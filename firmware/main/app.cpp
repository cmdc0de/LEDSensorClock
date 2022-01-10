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
#include <app/display_message_state.h>
#include "device/leds/apa102.h"
#include "spibus.h"
#include "freertos.h"
#include "fatfsvfs.h"
#include "pinconfig.h"
#include <device/sensor/dht11.h>

using libesp::ErrorType;
using libesp::System;
using libesp::FreeRTOS;
using libesp::RGBB;
using libesp::APA102c;
using libesp::SPIBus;
using libesp::DisplayILI9341;
using libesp::XPT2046;
using libesp::GUI;
using libesp::DisplayMessageState;
using libesp::BaseMenu;


const char *MyApp::LOGTAG = "AppTask";
const char *MyApp::sYES = "Yes";
const char *MyApp::sNO = "No";

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


const char *MyErrorMap::toString(int32_t err) {
	return "TODO";
}

MyApp MyApp::mSelf;

MyApp &MyApp::get() {
	return mSelf;
}

MyApp::MyApp() : AppErrors(), CurrentMode(ONE), LastTime(0) ,DHT22T() {
	ErrorType::setAppDetail(&AppErrors);
}

MyApp::~MyApp() {

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

libesp::ErrorType MyApp::onInit() {
	ErrorType et;
	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

  et = APA102c::initAPA102c(PIN_NUM_LEDS_MOSI, PIN_NUM_LEDS_CLK, SPI2_HOST, SPI_DMA_CH1);
  if(!et.ok()) {
    return et;
  } else {
    ESP_LOGI(LOGTAG,"APA102c inited");
  }

	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	SPIBus *vbus = SPIBus::get(SPI2_HOST);
  et = LedControl.initDevice(vbus);

  et = MyCalibrationMenu.initNVS();
	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to init nvs for calibration");
		return et;
	}

  //NOT initializing XPT as we don't need to set up spi bus since we are sharing with display
	//et = XPT2046::initTouch(PIN_NUM_TOUCH_MISO, PIN_NUM_TOUCH_MOSI, PIN_NUM_TOUCH_CLK, SPI3_HOST, SPI_DMA_DISABLED);
	//if(!et.ok()) {
//		ESP_LOGE(LOGTAG,"failed to touch");
//		return et;
//	}

  //this will init the SPI bus and the display
  DisplayILI9341::initDisplay(PIN_NUM_DISPLAY_MISO, PIN_NUM_DISPLAY_MOSI,
    PIN_NUM_DISPLAY_SCK, SPI_DMA_CH2, PIN_NUM_DISPLAY_DATA_CMD, PIN_NUM_DISPLAY_RESET,
    PIN_NUM_DISPLAY_BACKLIGHT, SPI3_HOST);

  ESP_LOGI(LOGTAG,"After Display: Free: %u, Min %u", System::get().getFreeHeapSize()
    ,System::get().getMinimumFreeHeapSize());

  SPIBus *hbus = libesp::SPIBus::get(SPI3_HOST);
#define USE_TOUCH
#ifdef USE_TOUCH
  et = TouchTask.init(hbus,PIN_NUM_TOUCH_CS);

	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to touch SPI");
		return et;
	}
#endif

	FrameBuf.createInitDevice(hbus,PIN_NUM_DISPLAY_CS,PIN_NUM_DISPLAY_DATA_CMD);
	
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

  if(et.ok()) {
    for(int i=0;i<NumLEDs;++i) {
      leds[i].setBlue(255);
      leds[i].setBrightness(100);
    }
    LedControl.init(NumLEDs, &leds[0]);
    LedControl.send();
    ESP_LOGI(LOGTAG,"OnInit:After Send: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
  } else {
    ESP_LOGI(LOGTAG,"failed initDevice");
  }

  if(MyCalibrationMenu.hasBeenCalibrated()) {
		setCurrentMenu(getCalibrationMenu());
	} else {
		setCurrentMenu(getMenuState());
	}

	return et;
}

ErrorType MyApp::onRun() {
  ErrorType et;
	TouchTask.broadcast();
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
    switch(CurrentMode) {
    case ONE:
      {
        for(int i=0;i<NumLEDs;++i) {
          leds[i].setBlue(0);
          leds[i].setBrightness(50);
          leds[i].setGreen(255);
        }

        LedControl.init(NumLEDs, &leds[0]);
        LedControl.send();
        CurrentMode = TWO;
      }
      break;
    case TWO:
      {
        for(int i=0;i<NumLEDs;++i) {
          leds[i].setBrightness(10);
          leds[i].setGreen(0);
          leds[i].setRed(255);
        }
        LedControl.init(NumLEDs, &leds[0]);
        LedControl.send();
        CurrentMode = THREE;
      }
      break;
    case THREE:
      {
        for(int i=0;i<NumLEDs;++i) {
          leds[i].setBrightness(5);
          leds[i].setGreen(0);
          leds[i].setRed(0);
          leds[i].setBlue(255);
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

MenuState *MyApp::getMenuState() {
	return &MyMenuState;
}

CalibrationMenu *MyApp::getCalibrationMenu() {
	return &MyCalibrationMenu;
}

DisplayMessageState *MyApp::getDisplayMessageState(BaseMenu *bm, const char *msg, uint32_t msDisplay) {
	DMS.setMessage(msg);
	DMS.setNextState(bm);
	DMS.setTimeInState(msDisplay);
	DMS.setDisplay(&Display);
	return &DMS;
}

