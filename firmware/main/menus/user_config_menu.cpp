/*
 * setting_state.cpp
 *
 *      Author: cmdc0de
 */

#include "user_config_menu.h"
#include <device/display/display_device.h>
#include "menu_state.h"
#include "../app.h"
#include <device/touch/XPT2046.h>
#include <math/point.h>
#include <esp_log.h>
#include "wifi_menu.h"
#include <math/rectbbox.h>
#include <net/webserver.h>
#include <app/display_message_state.h>
#include "game_of_life.h"
#include "menu3d.h"
#include "calibration_menu.h"
#include <system.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::TouchNotification;
using libesp::Button;
using libesp::Point2Ds;

static StaticQueue_t TouchQueue;
static uint8_t TouchQueueBuffer[UserConfigMenu::TOUCH_QUEUE_SIZE*UserConfigMenu::TOUCH_MSG_SIZE] = {0};
const char *UserConfigMenu::LOGTAG = "UserConfigMenu";

static const char *NORMAL_MODE = "Normal Mode";
static libesp::RectBBox2D NormalRec(Point2Ds(45,35), 40, 15);
static libesp::Button NormalBtn(NORMAL_MODE, uint16_t(0), &NormalRec, RGBColor::BLUE, RGBColor::WHITE);

static const int8_t NUM_INTERFACE_ITEMS = 1;
static libesp::Widget *InterfaceElements[NUM_INTERFACE_ITEMS] = {&NormalBtn};

UserConfigMenu::UserConfigMenu() : AppBaseMenu(), TouchQueueHandle() 
	, MyLayout(&InterfaceElements[0],NUM_INTERFACE_ITEMS, MyApp::get().getLastCanvasWidthPixel(), MyApp::get().getLastCanvasHeightPixel(), false) {

	MyLayout.reset();
	TouchQueueHandle = xQueueCreateStatic(TOUCH_QUEUE_SIZE,TOUCH_MSG_SIZE,&TouchQueueBuffer[0],&TouchQueue);
}

UserConfigMenu::~UserConfigMenu() {

}

ErrorType UserConfigMenu::onInit() {
	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(TouchQueueHandle, &pe, 0)) {
			delete pe;
		}
	}
	MyApp::get().getTouch().addObserver(TouchQueueHandle);
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
   //MyApp::get().getWiFiMenu()->startAP();
	return ErrorType();
}

BaseMenu::ReturnStateContext UserConfigMenu::onRun() {
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
               if(MyApp::get().getNVS().setValue(MyApp::CONFIG_MODE,int8_t(0)).ok()) {
                  libesp::System::get().restart();
               } else {
                  nextState = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState(),"Failed to save \n config mode setting", 2000);
               }
			   break;
            case MyApp::CLOSE_BTN_ID:
               nextState = MyApp::get().getMenuState();
            break;
         }
	   }
	}

   MyLayout.draw(&MyApp::get().getDisplay());
   MyApp::get().getDisplay().drawString(10,90,"AP is started: LEDClockSensor");
   MyApp::get().getDisplay().drawString(10,105,"http://192.168.4.1");

	return ReturnStateContext(nextState);
}

ErrorType UserConfigMenu::onShutdown() {
	MyApp::get().getTouch().removeObserver(TouchQueueHandle);
	return ErrorType();
}


