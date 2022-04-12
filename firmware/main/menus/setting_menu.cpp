/*
 * setting_state.cpp
 *
 *      Author: cmdc0de
 */

#include "setting_menu.h"
#include <device/display/display_device.h>
#include "menu_state.h"
#include "../app.h"
#include <device/touch/XPT2046.h>
#include <math/point.h>
#include <esp_log.h>
#include "calibration_menu.h"
#include "wifi_menu.h"
#include <math/rectbbox.h>
#include <net/webserver.h>
#include <app/display_message_state.h>
#include "game_of_life.h"
#include "menu3d.h"
#include <system.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::TouchNotification;
using libesp::Button;
using libesp::Point2Ds;

static StaticQueue_t TouchQueue;
static uint8_t TouchQueueBuffer[SettingMenu::TOUCH_QUEUE_SIZE*SettingMenu::TOUCH_MSG_SIZE] = {0};
const char *SettingMenu::LOGTAG = "SettingMenu";

static const char *START_AP = "Start AP";
static const char *STOP_AP = "Stop AP";
static libesp::RectBBox2D StartAP(Point2Ds(45,35), 40, 15);
static libesp::Button StartAPBtn(START_AP, uint16_t(0), &StartAP, RGBColor::BLUE, RGBColor::WHITE);
static libesp::RectBBox2D CalBV(Point2Ds(145,35), 40, 15);
static libesp::Button CalBtn((const char *)"Re-Calibrate", uint16_t(1), &CalBV, RGBColor::BLUE, RGBColor::WHITE);

static libesp::RectBBox2D GameOfLifeBV(Point2Ds(45,80), 40, 15);
static libesp::Button GOLBtn((const char *)"Game Of Life", uint16_t(2), &GameOfLifeBV, RGBColor::BLUE, RGBColor::WHITE);

static libesp::RectBBox2D Menu3DBV(Point2Ds(145,80), 40, 15);
static libesp::Button Menu3DBtn((const char *)"3D", uint16_t(3), &Menu3DBV, RGBColor::BLUE, RGBColor::WHITE);

static libesp::RectBBox2D ResetBV(Point2Ds(145,120), 40, 15);
static libesp::Button ResetBtn((const char *)"Reset", uint16_t(4), &ResetBV, RGBColor::BLUE, RGBColor::WHITE);

static const int8_t NUM_INTERFACE_ITEMS = 6;
static libesp::Widget *InterfaceElements[NUM_INTERFACE_ITEMS] = {&StartAPBtn, &GOLBtn, &CalBtn
  , &Menu3DBtn, &ResetBtn, &MyApp::get().getCloseButton()};

SettingMenu::SettingMenu() : AppBaseMenu(), TouchQueueHandle() 
	, MyLayout(&InterfaceElements[0],NUM_INTERFACE_ITEMS, MyApp::get().getLastCanvasWidthPixel(), MyApp::get().getLastCanvasHeightPixel(), false)
  , InternalState(INTERNAL_STATE::SHOW_ALL) {

	MyLayout.reset();
	TouchQueueHandle = xQueueCreateStatic(TOUCH_QUEUE_SIZE,TOUCH_MSG_SIZE,&TouchQueueBuffer[0],&TouchQueue);
}

SettingMenu::~SettingMenu() {

}

ErrorType SettingMenu::onInit() {
	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(TouchQueueHandle, &pe, 0)) {
			delete pe;
		}
	}
	MyApp::get().getTouch().addObserver(TouchQueueHandle);
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	return ErrorType();
}

BaseMenu::ReturnStateContext SettingMenu::onRun() {
	BaseMenu *nextState = this;
	TouchNotification *pe = nullptr;
	Point2Ds TouchPosInBuf;
	libesp::Widget *widgetHit = nullptr;
	bool penUp = false;
	if(xQueueReceive(TouchQueueHandle, &pe, 0)) {
		ESP_LOGI(LOGTAG,"que");
		Point2Ds screenPoint(pe->getX(),pe->getY());
		TouchPosInBuf = MyApp::get().getCalibrationMenu()->getPickPoint(screenPoint);
		ESP_LOGI(LOGTAG,"TouchPoint: X:%d Y:%d PD:%d", int32_t(TouchPosInBuf.getX()),
								 int32_t(TouchPosInBuf.getY()), pe->isPenDown()?1:0);
		penUp = !pe->isPenDown();
		delete pe;
		widgetHit = MyLayout.pick(TouchPosInBuf);
	  if(widgetHit && penUp) {
		  ESP_LOGI(LOGTAG, "Widget %s hit\n", widgetHit->getName());
	    MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
		  switch(widgetHit->getWidgetID()) {
    	case 0:
        if(InternalState!=AP_RUNNING) {
          StartAPBtn.setName(STOP_AP);
          MyApp::get().getWiFiMenu()->startAP();
          InternalState = AP_RUNNING;
        } else {
          StartAPBtn.setName(START_AP);
          MyApp::get().getWiFiMenu()->stopAP();
          InternalState = SHOW_ALL;
        }
			  break;
      case 1:
        MyApp::get().getWiFiMenu()->stopAP();
        nextState = MyApp::get().getCalibrationMenu();
        break;
      case 2:
        nextState = MyApp::get().getGameOfLife();
        break;
      case 3:
        nextState = MyApp::get().getMenu3D();
        break;
      case 4:
        MyApp::get().getWiFiMenu()->clearConnectData();
        MyApp::get().getCalibrationMenu()->eraseCalibration();
        libesp::System::get().restart();
        break;

      case MyApp::CLOSE_BTN_ID:
        nextState = MyApp::get().getMenuState();
        break;
		  }
	  }
	}

  if(InternalState==AP_RUNNING) {
    char strBuf[128];
    snprintf(&strBuf[0], sizeof(strBuf), "AP Running: SSID = %s", WiFiMenu::WIFIAPSSID);
    MyApp::get().getDisplay().drawString(10, 120, &strBuf[0]);
  } else {
    MyApp::get().getDisplay().drawString(10, 120, "AP Stopped");
  }

  MyLayout.draw(&MyApp::get().getDisplay());

	return ReturnStateContext(nextState);
}

ErrorType SettingMenu::onShutdown() {
	MyApp::get().getTouch().removeObserver(TouchQueueHandle);
  MyApp::get().getWiFiMenu()->stopAP();
	return ErrorType();
}


