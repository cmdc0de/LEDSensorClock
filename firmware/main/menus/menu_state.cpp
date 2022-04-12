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

static libesp::RectBBox2D TempBV(Point2Ds(30,37), 20, 15);
static libesp::Label TempLabel(uint16_t(0), (const char *)"Temp", &TempBV,RGBColor::BLUE, RGBColor::WHITE, RGBColor::BLACK, false);
static libesp::RectBBox2D HumBV(Point2Ds(85,37), 20, 15);
static libesp::Label HumLabel (uint16_t(0), (const char *)"Humidity", &HumBV,RGBColor::BLUE, RGBColor::WHITE, RGBColor::BLACK, false);

static libesp::RectBBox2D CO2BV(Point2Ds(140,37), 20, 15);
static libesp::Label CO2Label (uint16_t(0), (const char *)"CO2", &CO2BV,RGBColor::BLUE, RGBColor::WHITE, RGBColor::BLACK, false);

static libesp::RectBBox2D LSBV(Point2Ds(30,120), 20, 15);
static libesp::Label LSLabel (uint16_t(0), (const char *)"Light", &LSBV,RGBColor::BLUE, RGBColor::WHITE, RGBColor::BLACK, false);

static libesp::RectBBox2D OzoneBV(Point2Ds(85,120), 20, 15);
static libesp::Label OzoneLabel (uint16_t(0), (const char *)"Ozone", &OzoneBV,RGBColor::BLUE, RGBColor::WHITE, RGBColor::BLACK, false);

static libesp::RectBBox2D SettingRect(Point2Ds(150,120), 28, 15);
static libesp::Button SettingBtn((const char *)"Settings", uint16_t(2), &SettingRect, RGBColor::BLUE, RGBColor::WHITE);

static const int8_t NUM_INTERFACE_ITEMS = 6;
static libesp::Widget *InterfaceElements[NUM_INTERFACE_ITEMS] = {&TempLabel, &HumLabel, &CO2Label, &LSLabel, &OzoneLabel, &SettingBtn};

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
  float f = MyApp::get().getTemp();
  if(!MyApp::get().getConfig().wantC()) {
    f = ((f*9.0f)/5.0f) + 32;
  }
  sprintf(&buf[0],"%2.1f",f);
  TempLabel.setDisplayText(&buf[0]);
  sprintf(&buf[0],"%2.1f",MyApp::get().getHumidity());
  HumLabel.setDisplayText(&buf[0]);
  sprintf(&buf[0],"%d", MyApp::get().getCO2());
  CO2Label.setDisplayText(&buf[0]);
  sprintf(&buf[0],"%u", MyApp::get().getLightCalcVoltage());
  LSLabel.setDisplayText(&buf[0]);

  OzoneLabel.setDisplayText("TODO");

	MyLayout.draw(&MyApp::get().getDisplay());
    
  time_t now = 0;
  time(&now);
  struct tm timeinfo;
  memset(&timeinfo,0,sizeof(timeinfo));
  localtime_r(&now, &timeinfo);
  strftime(buf, sizeof(buf), "%c", &timeinfo);
  MyApp::get().getDisplay().drawString(3,70,&buf[0],(MyApp::get().wasMotion()?libesp::RGBColor::BLUE:libesp::RGBColor::WHITE));

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

