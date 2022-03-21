#include "menu_state.h"
#include "../app.h"
#include "gui_list_processor.h"
#include "calibration_menu.h"
#include <app/display_message_state.h>
#include <esp_log.h>
#include <math/rectbbox.h>
#include "setting_menu.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::XPT2046;
using libesp::Point2Ds;
using libesp::TouchNotification;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[MenuState::QUEUE_SIZE*MenuState::MSG_SIZE] = {0};
static const char *LOGTAG = "MenuState";

static libesp::RectBBox2D TempBV(Point2Ds(30,40), 25, 15);
static libesp::Label TempLabel(uint16_t(0), (const char *)"Temp", &TempBV,RGBColor::BLUE, RGBColor::WHITE, RGBColor::BLACK, false);
static libesp::RectBBox2D HumBV(Point2Ds(100,40), 25, 15);
static libesp::Label HumLabel (uint16_t(0), (const char *)"Humidity", &HumBV,RGBColor::BLUE, RGBColor::WHITE, RGBColor::BLACK, false);

static libesp::RectBBox2D SettingRect(Point2Ds(110,85), 30, 15);
static libesp::Button SettingBtn((const char *)"Settings", uint16_t(2), &SettingRect, RGBColor::BLUE, RGBColor::WHITE);

static const int8_t NUM_INTERFACE_ITEMS = 3;
static libesp::Widget *InterfaceElements[NUM_INTERFACE_ITEMS] = {&TempLabel, &HumLabel, &SettingBtn};

MenuState::MenuState() :
	AppBaseMenu(),
	MyLayout(&InterfaceElements[0],NUM_INTERFACE_ITEMS, MyApp::get().getLastCanvasWidthPixel(), MyApp::get().getLastCanvasHeightPixel(), false){
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
	MyLayout.reset();
}

MenuState::~MenuState() {

}

ErrorType MenuState::onInit() {
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			delete pe;
		}
	}
	MyApp::get().getTouch().addObserver(InternalQueueHandler);
	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext MenuState::onRun() {
	BaseMenu *nextState = this;

	TouchNotification *pe = nullptr;
	Point2Ds TouchPosInBuf;
	libesp::Widget *widgetHit = nullptr;
	bool penUp = false;
	if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
		ESP_LOGI(LOGTAG,"que");
		Point2Ds screenPoint(pe->getX(),pe->getY());
		TouchPosInBuf = MyApp::get().getCalibrationMenu()->getPickPoint(screenPoint);
		ESP_LOGI(LOGTAG,"TouchPoint: X:%d Y:%d PD:%d", int32_t(TouchPosInBuf.getX()),
								 int32_t(TouchPosInBuf.getY()), pe->isPenDown()?1:0);
		penUp = !pe->isPenDown();
		delete pe;
		widgetHit = MyLayout.pick(TouchPosInBuf);
	}

  char buf[32];
  sprintf(&buf[0],"%2.1f",MyApp::get().getTemp());
  TempLabel.setDisplayText(&buf[0]);
  sprintf(&buf[0],"%2.1f",MyApp::get().getHumidity());
  HumLabel.setDisplayText(&buf[0]);

	MyLayout.draw(&MyApp::get().getDisplay());
    
  sprintf(&buf[0],"R: %u V: %u mV", MyApp::get().getLightSensorRaw(), MyApp::get().getLightCalcVoltage());
  MyApp::get().getDisplay().drawString(3,110,&buf[0],libesp::RGBColor::WHITE);
  sprintf(&buf[0],"CO2: %d", MyApp::get().getCO2());
  MyApp::get().getDisplay().drawString(3,130,&buf[0],libesp::RGBColor::WHITE);

	if(widgetHit) {
		ESP_LOGI(LOGTAG, "Widget %s hit\n", widgetHit->getName());
		switch(widgetHit->getWidgetID()) {
		case 2:
			nextState = MyApp::get().getSettingMenu();
			break;
		}
	}

	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType MenuState::onShutdown() {
	MyApp::get().getTouch().removeObserver(InternalQueueHandler);
	return ErrorType();
}

