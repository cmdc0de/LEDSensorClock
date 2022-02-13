/*
 * app.cpp
 *
 * Author: cmdc0de
 */

#include "app.h"
#include <esp_log.h>
#include <system.h>
#include <spibus.h>

#include <driver/uart.h>
#include <driver/gpio.h>
#include <device/display/frame_buffer.h>
#include <device/display/display_device.h>
#include <device/display/fonts.h>
#include <device/display/gui.h>
#include <device/touch/XPT2046.h>
#include "menus/menu_state.h"
#include "menus/calibration_menu.h"
#include "menus/wifi_menu.h"
#include "menus/setting_menu.h"
#include <app/display_message_state.h>
#include "device/leds/apa102.h"
#include "spibus.h"
#include "freertos.h"
#include "fatfsvfs.h"
#include "pinconfig.h"
#include <device/sensor/dht11.h>
#include "appmsg.h"
#include <math/point.h>

using libesp::ErrorType;
using libesp::System;
using libesp::FreeRTOS;
using libesp::RGBB;
using libesp::RGBColor;
using libesp::APA102c;
using libesp::SPIBus;
using libesp::DisplayILI9341;
using libesp::XPT2046;
using libesp::GUI;
using libesp::DisplayMessageState;
using libesp::BaseMenu;
using libesp::Point2Ds;

const char *MyApp::LOGTAG = "AppTask";
const char *MyApp::sYES = "Yes";
const char *MyApp::sNO = "No";
//const uint16_t MyApp::CLOSE_BTN_ID = 1000;

#define START_ROT libesp::DisplayILI9341::LANDSCAPE_TOP_LEFT
static const uint16_t PARALLEL_LINES = 1;

libesp::DisplayILI9341 Display(MyApp::DISPLAY_WIDTH,MyApp::DISPLAY_HEIGHT,START_ROT,
	PIN_NUM_DISPLAY_BACKLIGHT, PIN_NUM_DISPLAY_RESET);

static uint16_t BkBuffer[MyApp::FRAME_BUFFER_WIDTH*MyApp::FRAME_BUFFER_HEIGHT];
static uint16_t *BackBuffer = &BkBuffer[0];

uint16_t ParallelLinesBuffer[MyApp::DISPLAY_WIDTH*PARALLEL_LINES] = {0};

libesp::ScalingBuffer FrameBuf(&Display, MyApp::FRAME_BUFFER_WIDTH, MyApp::FRAME_BUFFER_HEIGHT, uint8_t(16), MyApp::DISPLAY_WIDTH
    ,MyApp::DISPLAY_HEIGHT, PARALLEL_LINES, (uint8_t*)&BackBuffer[0],(uint8_t*)&ParallelLinesBuffer[0]);

static GUI MyGui(&Display);
static XPT2046 TouchTask(PIN_NUM_TOUCH_IRQ,true);
static CalibrationMenu MyCalibrationMenu("nvs");
static WiFiMenu MyWiFiMenu;

static libesp::AABBox2D Close(Point2Ds(185,7),6);
static libesp::Button CloseButton((const char *)"X", MyApp::CLOSE_BTN_ID, &Close,RGBColor::RED, RGBColor::BLUE);

const char *MyErrorMap::toString(int32_t err) {
	return "TODO";
}

MyApp MyApp::mSelf;
static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[MyApp::QUEUE_SIZE*MyApp::MSG_SIZE] = {0};
static StaticSemaphore_t xMutexDisplayTouchBuffer;

MyApp &MyApp::get() {
	return mSelf;
}

MyApp::MyApp() : AppErrors(), CurrentMode(ONE), LastTime(0) ,DHT22T()
                 , InternalQueueHandler(0), Temperature(0.0f), Humidity(0.0f), MHZ19T(), CO2(0)
                 , NVSStorage("appdata","data",false), LSensorResult(), DisplayTouchSemaphore(nullptr) {
	ErrorType::setAppDetail(&AppErrors);
}

MyApp::~MyApp() {

}

libesp::Button &MyApp::getCloseButton() {
  return CloseButton;
}

uint8_t *MyApp::getBackBuffer() {
	return (uint8_t *)&BackBuffer[0];
}

uint32_t MyApp::getBackBufferSize() {
	return MyApp::FRAME_BUFFER_WIDTH*MyApp::FRAME_BUFFER_HEIGHT*2;
}
  
static RGBB leds[256];
static size_t NumLEDs = sizeof(leds)/sizeof(leds[0]);
libesp::APA102c LedControl;
libesp::ADC LightSensor;

libesp::ErrorType MyApp::onInit() {
	ErrorType et;

	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);

	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

  DisplayTouchSemaphore = xSemaphoreCreateMutexStatic(&xMutexDisplayTouchBuffer);

	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
	et = NVSStorage.init();
	if(!et.ok()) {
		ESP_LOGI(LOGTAG, "1st InitNVS failed bc %s\n", et.toString());
		et = NVSStorage.initStorage();
		if(et.ok()) {
      ESP_LOGI(LOGTAG, "initStorage succeeded");
			et = NVSStorage.init();
      if(et.ok()) {
        ESP_LOGI(LOGTAG, "NVSSTorage init successful");
      } else {
		    ESP_LOGE(LOGTAG, "2nd InitNVS failed bc %s\nTHIS IS A PROBLEM\n", et.toString());
      }
		} else {
			ESP_LOGI(LOGTAG, "initStorage failed %s\n", et.toString());
		}
	}
	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

  et = APA102c::initAPA102c(PIN_NUM_LEDS_MOSI, PIN_NUM_LEDS_CLK, SPI2_HOST, SPI_DMA_CH1);
  if(!et.ok()) {
    return et;
  } else {
    ESP_LOGI(LOGTAG,"APA102c inited");
  }

	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
	SPIBus *vbus = SPIBus::get(SPI2_HOST);
  et = LedControl.initDevice(vbus);

  et = MyCalibrationMenu.initNVS();
	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to init nvs for calibration");
		return et;
	}

  //NOT initializing XPT as we don't need to set up spi bus since we are sharing with display
	et = XPT2046::initTouch(PIN_NUM_TOUCH_MISO, PIN_NUM_TOUCH_MOSI, PIN_NUM_TOUCH_CLK, SPI3_HOST, SPI_DMA_DISABLED);
	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to touch");
		return et;
	}

  //this will init the SPI bus and the display
  DisplayILI9341::initDisplay(PIN_NUM_DISPLAY_MISO, PIN_NUM_DISPLAY_MOSI,
    PIN_NUM_DISPLAY_SCK, SPI_DMA_CH2, PIN_NUM_DISPLAY_DATA_CMD, PIN_NUM_DISPLAY_RESET,
    PIN_NUM_DISPLAY_BACKLIGHT, SPI3_HOST);

  ESP_LOGI(LOGTAG,"After Display: Free: %u, Min %u", System::get().getFreeHeapSize()
    ,System::get().getMinimumFreeHeapSize());

  SPIBus *hbus = libesp::SPIBus::get(SPI3_HOST);
  et = TouchTask.init(hbus,PIN_NUM_TOUCH_CS,DisplayTouchSemaphore);

	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to touch SPI");
		return et;
	}

	FrameBuf.createInitDevice(hbus,PIN_NUM_DISPLAY_CS,PIN_NUM_DISPLAY_DATA_CMD, DisplayTouchSemaphore);
	
	ESP_LOGI(LOGTAG,"After FrameBuf: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	ESP_LOGI(LOGTAG,"start display init");
	et=Display.init(libesp::DisplayILI9341::FORMAT_16_BIT, &Font_6x10, &FrameBuf);

  if(et.ok()) {
		ESP_LOGI(LOGTAG,"display init OK");
		Display.fillScreen(libesp::RGBColor::BLACK);
		Display.swap();
		ESP_LOGI(LOGTAG,"fill black done");
		Display.fillRec(0,0,FRAME_BUFFER_WIDTH/4,10,libesp::RGBColor::RED);
		Display.swap();
		vTaskDelay(500 / portTICK_RATE_MS);
		Display.fillRec(0,15,FRAME_BUFFER_WIDTH/2,10,libesp::RGBColor::WHITE);
		Display.swap();
		vTaskDelay(500 / portTICK_RATE_MS);
		Display.fillRec(0,30,FRAME_BUFFER_WIDTH,10,libesp::RGBColor::BLUE);
		Display.swap();
		vTaskDelay(500 / portTICK_RATE_MS);
		Display.drawRec(0,60,FRAME_BUFFER_WIDTH/2,20, libesp::RGBColor::GREEN);
		Display.drawString(15,110,"Color Validation.",libesp::RGBColor::RED);
		Display.drawString(30,120,"Sensor Clock",libesp::RGBColor::BLUE, libesp::RGBColor::WHITE,1,false);
		Display.swap();

		vTaskDelay(1000 / portTICK_RATE_MS);
		ESP_LOGI(LOGTAG,"After Display swap:Free: %u, Min %u",System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	} else {
		ESP_LOGE(LOGTAG,"failed display init");
	}

	TouchTask.start();
	ESP_LOGI(LOGTAG,"After Touch Task starts: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

  if(DHT22T.init(PIN_NUM_DHT_DATA).ok()) {
    DHT22T.start();
	  ESP_LOGI(LOGTAG,"After DHT22 Task starts: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
  } else {
    ESP_LOGE(LOGTAG,"Failed to initialize DHT22 Task");
  }

  //IO35
  if(!LightSensor.init(ADC1_CHANNEL_7, ADC2_CHANNEL_MAX, ADC_WIDTH_BIT_12, ADC_ATTEN_DB_11, 30).ok()) {
    ESP_LOGE(LOGTAG,"Failed to initialize LightSensor ");
  } else {
    libesp::ADC::Result r;
    LightSensor.acquireData(r);
    ESP_LOGI(LOGTAG, "RAW: %u Voltage: %u", r.RawAvg, r.CalculatedVoltage);
  }

  //init MHZ19
  //PIN_NUM_ESP_TX_MHZ19B_RX
  //PIN_NUM_ESP_RX_MHZ19B_TX
  if(!MHZ19T.init(UART_NUM_2, PIN_NUM_ESP_RX_MHZ19B_TX, PIN_NUM_ESP_TX_MHZ19B_RX).ok()) {
    ESP_LOGE(LOGTAG,"Failed to initialize MHZ19B ");
  } else {
    ESP_LOGI(LOGTAG,"MHZ19 initialized");
    MHZ19T.start();
  }

  if(et.ok()) {
    for(int i=0;i<NumLEDs;++i) {
      leds[i].setBlue(255);
      leds[i].setBrightness(16);
    }
    LedControl.init(NumLEDs, &leds[0]);
    LedControl.send();
    ESP_LOGI(LOGTAG,"OnInit:After Send: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
  } else {
    ESP_LOGI(LOGTAG,"failed initDevice");
  }

  et = MyWiFiMenu.initWiFi();
  if(et.ok()) {
    ESP_LOGI(LOGTAG,"OnInit:After Send: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
  } else {
    ESP_LOGE(LOGTAG,"Error Num :%d Msg: %s", et.getErrT(), et.toString());
  }

  if(!MyCalibrationMenu.hasBeenCalibrated()) {
		setCurrentMenu(getCalibrationMenu());
	} else {
    if(MyWiFiMenu.hasWiFiBeenSetup().ok()) {
      et = MyWiFiMenu.connect();
  		setCurrentMenu(getMenuState());
    } else {
      setCurrentMenu(getSettingMenu());
    }
	}
	return et;
}

void MyApp::handleMessages() {
  MyAppMsg *msg = nullptr;
  int counter = 0;
	while(counter++<5 && xQueueReceive(InternalQueueHandler, &msg, 0)) {
    if(msg!=nullptr) {
      if(msg->handleMessage(this)) {
  		  delete msg;
      }
    }
  }
}

static uint32_t SecondCount = 60;
static uint32_t MinCount = 0;
static uint16_t LightSensorCounter = 0;

ErrorType MyApp::onRun() {
  ErrorType et;
	TouchTask.broadcast();
  handleMessages();
	libesp::BaseMenu::ReturnStateContext rsc = getCurrentMenu()->run();
	Display.swap();

	if (rsc.Err.ok()) {
		if (getCurrentMenu() != rsc.NextMenuToRun) {
			setCurrentMenu(rsc.NextMenuToRun);
			ESP_LOGI(LOGTAG,"on Menu swap: Free: %u, Min %u",
				System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
		} else {
		}
	} 

  uint32_t timeSinceLast = FreeRTOS::getTimeSinceStart()-LastTime;
  if(timeSinceLast>=TIME_BETWEEN_PULSES) {
    LastTime = FreeRTOS::getTimeSinceStart();

    if(++LightSensorCounter>3) {
      LightSensor.acquireData(LSensorResult);
      LightSensorCounter=0;
    }
    //ESP_LOGI(LOGTAG, "RAW: %u Voltage: %u", r.RawAvg, r.CalculatedVoltage);
    //char buf[32];
    //sprintf(&buf[0],"R: %u V: %u mV", r.RawAvg, r.CalculatedVoltage);
    //Display.drawString(3,110,&buf[0],libesp::RGBColor::WHITE);
    //sprintf(&buf[0],"CO2: %d", CO2);
    //Display.drawString(3,130,&buf[0],libesp::RGBColor::WHITE);

    switch(CurrentMode) {
    case ONE:
      {
        for(int i=0;i<NumLEDs;++i) {
          leds[i].setBlue(0);
          leds[i].setBrightness(12);
          leds[i].setGreen(255);
          leds[i].setRed(0);
        }

        LedControl.init(NumLEDs, &leds[0]);
        LedControl.send();
        CurrentMode = TWO;
      }
      break;
    case TWO:
      {
        for(int i=0;i<NumLEDs;++i) {
          leds[i].setBrightness(8);
          leds[i].setGreen(0);
          leds[i].setRed(255);
          leds[i].setBlue(0);
        }
        LedControl.init(NumLEDs, &leds[0]);
        LedControl.send();
        CurrentMode = THREE;
      }
      break;
    case THREE:
      {
        for(int i=0;i<NumLEDs;++i) {
          leds[i].setBrightness(4);
          leds[i].setGreen(0);
          leds[i].setRed(0);
          leds[i].setBlue(255);
        }
        LedControl.init(NumLEDs, &leds[0]);
        LedControl.send();
        CurrentMode = FOUR;
      }
      break;
    case FOUR:
      {
        for(int i=0;i<NumLEDs;++i) {
          if(i>59 && i<=SecondCount) {
            leds[i].setBrightness(8);
            leds[i].setGreen(32);
            leds[i].setRed(0);
            leds[i].setBlue(0);
          } else if(i<=MinCount) {
            leds[i].setBrightness(8);
            leds[i].setGreen(0);
            leds[i].setRed(0);
            leds[i].setBlue(64);
          } else {
            leds[i].setBrightness(0);
          }
        }
        if(++SecondCount>119) {
          SecondCount = 60;
          MinCount++;
        }
        LedControl.init(NumLEDs, &leds[0]);
        LedControl.send();
      }
      break;
    }
  }
	return et;
}

XPT2046 &MyApp::getTouch() {
	return TouchTask;
}

uint16_t MyApp::getCanvasWidth() {
	return FrameBuf.getBufferWidth(); 
}

uint16_t MyApp::getCanvasHeight() {
	return FrameBuf.getBufferHeight();
}

uint16_t MyApp::getLastCanvasWidthPixel() {
	return getCanvasWidth()-1;
}

uint16_t MyApp::getLastCanvasHeightPixel() {
	return getCanvasHeight()-1;
}

libesp::DisplayDevice &MyApp::getDisplay() {
	return Display;
}

libesp::GUI &MyApp::getGUI() {
	return MyGui;
}

MenuState MyMenuState;
libesp::DisplayMessageState DMS;
SettingMenu MySettingMenu;

MenuState *MyApp::getMenuState() {
	return &MyMenuState;
}

SettingMenu *MyApp::getSettingMenu() {
	return &MySettingMenu;
}

CalibrationMenu *MyApp::getCalibrationMenu() {
	return &MyCalibrationMenu;
}

WiFiMenu *MyApp::getWiFiMenu() {
  return &MyWiFiMenu;
}

DisplayMessageState *MyApp::getDisplayMessageState(BaseMenu *bm, const char *msg, uint32_t msDisplay) {
	DMS.setMessage(msg);
	DMS.setNextState(bm);
	DMS.setTimeInState(msDisplay);
	DMS.setDisplay(&Display);
	return &DMS;
}

