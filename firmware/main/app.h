/*
 * app.h
 *
 * Author: cmdc0de
 */

#ifndef LIGHTBOX_APP_H
#define LIGHTBOX_APP_H

#include <app/app.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "dht22task.h"
#include "mhz19btask.h"
#include <nvs_memory.h>
#include <adc.h>
#include <freertos.h>
#include <device/display/layout.h>
#include "config.h"

namespace libesp {
class GUI;
class DisplayDevice;
class DisplayMessageState;
class XPT2046;
class OTA;
};

class CalibrationMenu;
class MenuState;
class WiFiMenu;
class SettingMenu;
class GameOfLife;
class Menu3D;
class UserConfigMenu;

enum ERRORS {
	APP_OK = libesp::ErrorType::APP_OK
	, OTA_INIT_FAIL = libesp::ErrorType::APP_BASE + 1
	, BT_INIT_FAIL
	, GAME_TASK_INIT_FAIL
	, EXPLOIT_TASK_INIT_FAIL
	, WIFI_TASK_INIT_FAIL
	, BUTTON_INIT_FAIL
	, TOP_BOARD_INIT_FAIL
};

class MyErrorMap : public libesp::IErrorDetail {
public:
	virtual const char *toString(int32_t err);
	virtual ~MyErrorMap() {}
};

class MyAppMsg;

class MyApp : public libesp::App {
public:
  enum MODE {
    ONE,
    TWO
    , THREE
    , FOUR
  };
public:
	static const char *LOGTAG;
	static const char *MENUHEADER;
   static constexpr const char *CONFIG_MODE="CONFIG_MODE";
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(MyAppMsg*);
	static const char *sYES;
	static const char *sNO;
   static const uint32_t TIME_BETWEEN_PULSES = 200;
   static const uint32_t TIME_BETWEEN_WIFI_CONNECTS = 60000;
   static const uint16_t DISPLAY_HEIGHT		= 240;
	static const uint16_t DISPLAY_WIDTH			= 320;
	//reminder ESP32 has 160KiB static and DRAM So a 1:1 buffer doesn't fit.
	static const uint16_t FRAME_BUFFER_HEIGHT	= 144;
	static const uint16_t FRAME_BUFFER_WIDTH	= 192;
   static const uint32_t TIME_MOTION_DETECT  = 3000;
   static const uint32_t CLOSE_BTN_ID = 1000; 
   static const uint32_t ESP_INTR_FLAG_DEFAULT= 0;

	static MyApp &get();
public:
   virtual ~MyApp();
   uint16_t getCanvasWidth();
	uint16_t getCanvasHeight();
	uint16_t getLastCanvasWidthPixel();
	uint16_t getLastCanvasHeightPixel();
	libesp::DisplayDevice &getDisplay();
	libesp::GUI &getGUI();
   MenuState *getMenuState();
   SettingMenu *getSettingMenu();
	CalibrationMenu *getCalibrationMenu();
   GameOfLife *getGameOfLife();
   Menu3D *getMenu3D();
	WiFiMenu *getWiFiMenu();
   UserConfigMenu *getUserConfigMenu();
	libesp::DisplayMessageState *getDisplayMessageState(libesp::BaseMenu *, const char *msg, uint32_t msDisplay);
	libesp::XPT2046 &getTouch();
	uint8_t *getBackBuffer();
	uint32_t getBackBufferSize();
   QueueHandle_t getMessageQueueHandle() {return InternalQueueHandler;}
   void setTempHumidity(float t, float h) {Temperature=t;Humidity=h;}
   float getTemp() {return Temperature;}
   float getHumidity() {return Humidity;}
   void setCO2(int16_t v) {CO2=v;}
   uint16_t getCO2() {return CO2;}
   libesp::NVS &getNVS() { return NVSStorage;}
   uint32_t getLightSensorRaw() { return LSensorResult.RawAvg;}
   uint32_t getLightCalcVoltage() { return LSensorResult.CalculatedVoltage;}
   libesp::Button &getCloseButton();
   Config &getConfig() {return ConfigStore;}
   libesp::ErrorType initFS();
   bool wasMotion();
   bool isConfigMode() {return IsConfigMode==1;}
   libesp::OTA &getOTA();
protected:
	MyApp();
   void handleMessages();
	virtual libesp::ErrorType onInit();
	virtual libesp::ErrorType onRun();
private:
	MyErrorMap AppErrors;
  MODE CurrentMode;
  uint32_t LastTime;
  DHT22Task DHT22T;
	QueueHandle_t InternalQueueHandler;
  float Temperature;
  float Humidity;
  MHZ19Task MHZ19T;
  int16_t CO2;
  libesp::NVS NVSStorage;
  libesp::ADC::Result LSensorResult;
  SemaphoreHandle_t DisplayTouchSemaphore;
  Config ConfigStore;
  uint32_t LastConnectCheck;
  int8_t IsConfigMode;
private:
	static MyApp mSelf;
};

#endif /* DC27_APP_H */
