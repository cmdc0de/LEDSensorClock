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
#include "menus/game_of_life.h"
#include "menus/wifi_menu.h"
#include "menus/setting_menu.h"
#include "menus/menu3d.h"
#include "menus/user_config_menu.h"
#include <app/display_message_state.h>
#include "device/leds/apa102.h"
#include "spibus.h"
#include "freertos.h"
#include "fatfsvfs.h"
#include "pinconfig.h"
#include <device/sensor/dht11.h>
#include "appmsg.h"
#include <math/point.h>
#include <esp_spiffs.h>
#include <time.h>
#include <net/ota.h>

using libesp::ErrorType;
using libesp::System;
using libesp::FreeRTOS;
using libesp::RGBB;
using libesp::RGBColor;
using libesp::APA102c;
using libesp::SPIBus;
using libesp::TFTDisplay;
using libesp::XPT2046;
using libesp::GUI;
using libesp::DisplayMessageState;
using libesp::BaseMenu;
using libesp::Point2Ds;

const char *MyApp::LOGTAG = "AppTask";
const char *MyApp::sYES = "Yes";
const char *MyApp::sNO = "No";

#define START_ROT libesp::TFTDisplay::LANDSCAPE_TOP_LEFT
static const uint16_t PARALLEL_LINES = 1;

libesp::TFTDisplay Display(MyApp::DISPLAY_WIDTH,MyApp::DISPLAY_HEIGHT,START_ROT,
	PIN_NUM_DISPLAY_BACKLIGHT, PIN_NUM_DISPLAY_RESET, TFTDisplay::ILI9341);

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

libesp::OTA CCOTA;

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
                 , NVSStorage("appdata","data",false), LSensorResult(), DisplayTouchSemaphore(nullptr)
                 , ConfigStore(&NVSStorage), LastConnectCheck(0), IsConfigMode(0) {
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

ErrorType MyApp::initFS() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/www",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(LOGTAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(LOGTAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(LOGTAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(LOGTAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(LOGTAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}

uint32_t LastMotionDetect = 0;

static void gpio_isr_handler(void* arg) {
  LastMotionDetect = FreeRTOS::getTimeSinceStart();
  printf("motion");
}

bool MyApp::wasMotion() {
  return ((FreeRTOS::getTimeSinceStart()-LastMotionDetect)<TIME_MOTION_DETECT);
}

libesp::ErrorType MyApp::onInit() {
	ErrorType et;

	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);

	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
   et = getConfig().init();
   if(!et.ok()) {
      ESP_LOGE(LOGTAG,"Failed to init config: %d %s", et.getErrT(), et.toString());
   }

   //const char *UPDATE_URL="https://s3.us-west-2.amazonaws.com/tech.cmdc0de.ledsensorclock/LEDSensorClock.bin";
   const char *UPDATE_URL="https://www.google.com";
   CCOTA.init(UPDATE_URL);
   CCOTA.logCurrentActiveParitionInfo();
   if(CCOTA.isUpdateAvailable()) {
      ESP_LOGI(LOGTAG,"*****UPDATE AVAILABLE!!!****");
      et = CCOTA.applyUpdate(true);
      if(et.ok()) {
         ESP_LOGI(LOGTAG,"UPDATE SUCCESSFUL to version %s",CCOTA.getCurrentApplicationVersion());
      } else {
         ESP_LOGI(LOGTAG,"UPDATE FAILED");
      }
   }

	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

  DisplayTouchSemaphore = xSemaphoreCreateMutexStatic(&xMutexDisplayTouchBuffer);
	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
  initFS();
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
  TFTDisplay::initDisplay(PIN_NUM_DISPLAY_MISO, PIN_NUM_DISPLAY_MOSI,
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
	et=Display.init(libesp::TFTDisplay::FORMAT_16_BIT, &Font_6x10, &FrameBuf);

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
    //ESP_LOGI(LOGTAG, "RAW: %u Voltage: %u", r.RawAvg, r.CalculatedVoltage);
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


  et = getNVS().getValue(MyApp::CONFIG_MODE,IsConfigMode);
  ESP_LOGI(LOGTAG, "value of config mode is: %d", int32_t(IsConfigMode));
   if(isConfigMode()) {
      //et = MyWiFiMenu.initWiFiForAP();
      MyApp::get().getWiFiMenu()->startAP();
      setCurrentMenu(getUserConfigMenu());
   } else {
      et = MyWiFiMenu.initWiFiForSTA();
      if(et.ok()) {
         ESP_LOGI(LOGTAG,"OnInit:After MyWiFiMenu::initWiFi: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
      } else {
         ESP_LOGE(LOGTAG,"Error Num :%d Msg: %s", et.getErrT(), et.toString());
      }

      if(!MyCalibrationMenu.hasBeenCalibrated()) {
         setCurrentMenu(getCalibrationMenu());
      } else {
         ESP_LOGI(LOGTAG,"***********************************************************");
         if(MyWiFiMenu.hasWiFiBeenSetup().ok()) {
            et = MyWiFiMenu.connect();
            setCurrentMenu(getMenuState());
         } else {
            setCurrentMenu(getSettingMenu());
         }
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

static uint16_t LightSensorCounter = 0;

static uint8_t MinIndex[60] = 
{
   4, 5 ,6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,
  34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59, 0, 1, 2, 3,  
};

static uint8_t SecIndex[60] = 
{
//   1,  2,  3,  4, 5, 6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    64, 65, 66, 67,68,69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 
    94, 95, 96, 97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119, 60, 61, 62, 63
};

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
  uint32_t connectCheckTime = FreeRTOS::getTimeSinceStart()-LastConnectCheck;
  if(connectCheckTime > TIME_BETWEEN_WIFI_CONNECTS && !isConfigMode()) {
    LastConnectCheck = FreeRTOS::getTimeSinceStart();
    if(!MyWiFiMenu.isConnected()) {
      ESP_LOGI(LOGTAG,"WifI not connected...reconnecting...");
      MyWiFiMenu.connect();
    }
  }

  if(timeSinceLast>=TIME_BETWEEN_PULSES) {
    LastTime = FreeRTOS::getTimeSinceStart();

    if(++LightSensorCounter>5) {
      LightSensor.acquireData(LSensorResult);
      LightSensorCounter=0;
    }

    //calculate Brightness based on light sensor
    uint8_t maxBrightness = getConfig().getMaxBrightness();
    uint32_t lightValue = getLightCalcVoltage();
    if(lightValue>=1000 && lightValue<2000) {
      maxBrightness*=0.75f;
    } else if(lightValue>=2000 && lightValue<3000) {
      maxBrightness*=0.5f;
    } else if(lightValue>=3000) {
      maxBrightness*=0.33f;
    }
    maxBrightness=maxBrightness<4?4:maxBrightness;

    switch(CurrentMode) {
    case ONE:
      {
        //ESP_LOGI(LOGTAG,"mode 1");
        for(int i=0;i<NumLEDs;++i) {
          leds[i].setBlue(0);
          leds[i].setBrightness(maxBrightness);
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
        //ESP_LOGI(LOGTAG,"mode 2");
        for(int i=0;i<NumLEDs;++i) {
          leds[i].setBrightness(maxBrightness>>1);
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
        //ESP_LOGI(LOGTAG,"mode 3");
        for(int i=0;i<NumLEDs;++i) {
          leds[i].setBrightness(maxBrightness>>2);
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
        //ESP_LOGI(LOGTAG,"clock mode");
        time_t now = 0;
        time(&now);
        struct tm timeinfo;
        memset(&timeinfo,0,sizeof(timeinfo));
        localtime_r(&now, &timeinfo);

        for(int i=0;i<NumLEDs;++i) {
          leds[i].setBrightness(0);
          leds[i].setGreen(0);
          leds[i].setRed(0);
          leds[i].setBlue(0);
        }
        libesp::RGBColor secColorS = getConfig().getSecondHandStartColor();
        libesp::RGBColor secColorE = getConfig().getSecondHandEndColor();
        float rShiftSec = float(secColorE.getR()-secColorS.getR())/60.0f;
        float gShiftSec = float(secColorE.getG()-secColorS.getG())/60.0f;
        float bShiftSec = float(secColorE.getB()-secColorS.getB())/60.0f;
        libesp::RGBColor minColorS = getConfig().getMinHandStartColor();
        libesp::RGBColor minColorE = getConfig().getMinHandEndColor();
        float rShiftMin = float(minColorE.getR()-minColorS.getR())/60.0f;
        float gShiftMin = float(minColorE.getG()-minColorS.getG())/60.0f;
        float bShiftMin = float(minColorE.getB()-minColorS.getB())/60.0f;
        for(int i=0;i<60;++i) {
          if(i<=timeinfo.tm_sec) {
            leds[SecIndex[i]].setBrightness(maxBrightness);
            leds[SecIndex[i]].setGreen(secColorS.getG()+uint8_t(gShiftSec*float(timeinfo.tm_sec-i)));
            leds[SecIndex[i]].setRed(secColorS.getR()+uint8_t(rShiftSec*float(timeinfo.tm_sec-i)));
            leds[SecIndex[i]].setBlue(secColorS.getB()+uint8_t(bShiftSec*float(timeinfo.tm_sec-i)));
          } else {
            leds[SecIndex[i]].setBrightness(0);
          }
          if(i<=timeinfo.tm_min) {
            leds[MinIndex[i]].setBrightness(maxBrightness);
            leds[MinIndex[i]].setGreen(minColorS.getG()+uint8_t(gShiftMin*float(timeinfo.tm_min-i)));
            leds[MinIndex[i]].setRed(minColorS.getR()+uint8_t(rShiftMin*float(timeinfo.tm_min-i)));
            leds[MinIndex[i]].setBlue(minColorS.getB()+uint8_t(bShiftMin*float(timeinfo.tm_min-i)));
          } else {
            leds[MinIndex[i]].setBrightness(0);
          }
        }
        float hrBAdjust = float(timeinfo.tm_min)/60.0f;
        float invHrBAdjust = 1.0f-hrBAdjust;
        uint8_t currentHrBright = uint8_t(invHrBAdjust*float(maxBrightness));
        currentHrBright = currentHrBright<4?4:currentHrBright;
        uint8_t nextHrBright = uint8_t(hrBAdjust*float(maxBrightness));
        libesp::RGBColor hrColorS = getConfig().getHourStartColor();
        libesp::RGBColor hrColorE = getConfig().getHourEndColor();
        float rShiftHr = float(hrColorE.getR()-hrColorS.getR())/60.0f;
        float gShiftHr = float(hrColorE.getG()-hrColorS.getG())/60.0f;
        float bShiftHr = float(hrColorE.getB()-hrColorS.getB())/60.0f;
        uint8_t currentHrR = hrColorS.getR()+uint8_t(rShiftHr*float(timeinfo.tm_min));
        uint8_t currentHrG = hrColorS.getG()+uint8_t(gShiftHr*float(timeinfo.tm_min));
        uint8_t currentHrB = hrColorS.getB()+uint8_t(bShiftHr*float(timeinfo.tm_min));
        uint8_t nextHrR = hrColorS.getR()+uint8_t(rShiftHr*float(60-timeinfo.tm_min));
        uint8_t nextHrG = hrColorS.getG()+uint8_t(gShiftHr*float(60-timeinfo.tm_min));
        uint8_t nextHrB = hrColorS.getB()+uint8_t(bShiftHr*float(60-timeinfo.tm_min));
        uint8_t startLED=0, endLED=0, endCurrentHourLED=0;
        bool bCommon = true;
        //ESP_LOGI(LOGTAG,"tm_hour %d", timeinfo.tm_hour);
        switch(timeinfo.tm_hour) {
          case 0: case 12:
            startLED = 120;
            endLED = 141;
            endCurrentHourLED = 136;
            break;
          case 1: case 13:
            startLED = 136;
            endLED = 152;
            endCurrentHourLED = 141;
            break;
          case 2: case 14:
            startLED = 141;
            endLED = 162;
            endCurrentHourLED = 152;
            break;
          case 3: case 15:
            startLED = 152;
            endLED = 170;
            endCurrentHourLED = 162;
            break;
          case 4: case 16:
            startLED = 162;
            endLED = 181;
            endCurrentHourLED = 170;
            break;
          case 5: case 17:
            startLED = 170;
            endLED = 193;
            endCurrentHourLED = 181;
            break;
          case 6: case 18:
            startLED = 181;
            endLED = 200;
            endCurrentHourLED = 193;
            break;
          case 7: case 19:
            startLED = 193;
            endLED = 213;
            endCurrentHourLED = 200;
            break;
          case 8: case 20:
            startLED = 200;
            endLED = 223;
            endCurrentHourLED = 213;
            break;
          case 9: case 21:
            startLED = 213;
            endLED = 240;
            endCurrentHourLED = 223;
            break;
          case 10: case 22:
            startLED = 223;
            endLED = 250;
            endCurrentHourLED = 240;
            break;
          case 11: case 23:
            {
              //ESP_LOGI(LOGTAG,"case 11/23");
              bCommon = false;
              for(int i=240;i<250;++i) {
                leds[i].setBrightness(currentHrBright);
                leds[i].setGreen(currentHrG);
                leds[i].setRed(currentHrR);
                leds[i].setBlue(currentHrB);
              }
              for(int i=120;i<136;++i) {
                leds[i].setBrightness(nextHrBright);
                leds[i].setGreen(nextHrG);
                leds[i].setRed(nextHrR);
                leds[i].setBlue(nextHrB);
              }
            }
            break;
        }
        if(bCommon) {
          //ESP_LOGI(LOGTAG,"cBright %d - nBright %d", currentHrBright, nextHrBright);
          for(int i=startLED;i<endLED;++i) {
            if(i<endCurrentHourLED) { 
              leds[i].setBrightness(currentHrBright);
              leds[i].setGreen(currentHrG);
              leds[i].setRed(currentHrR);
              leds[i].setBlue(currentHrB);
            } else {
              leds[i].setBrightness(nextHrBright);
              leds[i].setGreen(nextHrG);
              leds[i].setRed(nextHrR);
              leds[i].setBlue(nextHrB);
            }
          }
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
GameOfLife GOL;
UserConfigMenu UCM;


Menu3D Menu3DRender( uint8_t(float(MyApp::FRAME_BUFFER_WIDTH)*0.8f)
    , uint8_t(float(MyApp::FRAME_BUFFER_HEIGHT)*0.8f));

Menu3D *MyApp::getMenu3D() {
  return &Menu3DRender;
}

GameOfLife *MyApp::getGameOfLife() {
  return &GOL;
}

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

UserConfigMenu *MyApp::getUserConfigMenu() {
   return &UCM;
}

libesp::OTA &MyApp::getOTA() {
   return CCOTA;
}

DisplayMessageState *MyApp::getDisplayMessageState(BaseMenu *bm, const char *msg, uint32_t msDisplay) {
	DMS.setMessage(msg);
	DMS.setNextState(bm);
	DMS.setTimeInState(msDisplay);
	DMS.setDisplay(&Display);
	return &DMS;
}

