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

namespace libesp {
class GUI;
class DisplayDevice;
class DisplayMessageState;
class XPT2046;
};

class CalibrationMenu;
class MenuState;

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
  };
public:
	static const char *LOGTAG;
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(MyAppMsg*);
	static const char *sYES;
	static const char *sNO;
  static const uint32_t TIME_BETWEEN_PULSES = 1000;
  static const uint16_t DISPLAY_HEIGHT		= 240;
	static const uint16_t DISPLAY_WIDTH			= 320;
	//reminder ESP32 has 160KiB static and DRAM So a 1:1 buffer doesn't fit.
	static const uint16_t FRAME_BUFFER_HEIGHT	= 144;
	static const uint16_t FRAME_BUFFER_WIDTH	= 192;

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
	CalibrationMenu *getCalibrationMenu();
	libesp::DisplayMessageState *getDisplayMessageState(libesp::BaseMenu *, const char *msg, uint32_t msDisplay);
	libesp::XPT2046 &getTouch();
	uint8_t *getBackBuffer();
	uint32_t getBackBufferSize();
  QueueHandle_t getMessageQueueHandle() {return InternalQueueHandler;}
  void setTempHumidity(float t, float h) {Temperature=t;Humidity=h;}
  float getTemp() {return Temperature;}
  float getHumidity() {return Humidity;}
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
private:
	static MyApp mSelf;
};

#endif /* DC27_APP_H */
