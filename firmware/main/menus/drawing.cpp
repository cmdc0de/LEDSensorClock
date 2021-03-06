#include "drawing.h"
#include "menu_state.h"
#include "../app.h"
#include <math/point.h>
#include "calibration_menu.h"
#include "device/display/color.h"
#include "app/display_message_state.h"
#include <nvs_memory.h>
#include <esp_log.h>

using libesp::ErrorType;
using libesp::RGBColor;
using libesp::Point2Ds;
using libesp::TouchNotification;
using libesp::BaseMenu;
using libesp::NVS;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[DrawingMenu::QUEUE_SIZE*DrawingMenu::MSG_SIZE] = {0};
const char *DrawingMenu::LOGTAG = "DrawingMenu";

static const RGBColor Colors[] = {
	RGBColor::WHITE,
	RGBColor(32,0,0),
	RGBColor(64,0,0),
	RGBColor(127,0,0),
	RGBColor(192,0,0),
	RGBColor(224,0,0),
	RGBColor::RED,
	RGBColor(0,32,0),
	RGBColor(0,64,0),
	RGBColor(0,127,0),
	RGBColor(0,192,0),
	RGBColor(0,224,0),
	RGBColor::GREEN,
	RGBColor(0,0,32),
	RGBColor(0,0,64),
	RGBColor(0,0,127),
	RGBColor(0,0,192),
	RGBColor(0,0,224),
	RGBColor::BLUE,
	RGBColor(224,224,224),
	RGBColor(192,192,192),
	RGBColor(127,127,127),
	RGBColor(64,64,64),
	RGBColor(32,32,32),
};

static const uint32_t MAX_COLORS = sizeof(Colors)/sizeof(Colors[0]);
static NVS NVSDrawing("drawing","draw", false);

DrawingMenu::DrawingMenu() : AppBaseMenu(), InternalQueueHandler(), ColorIndex(0) {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

DrawingMenu::~DrawingMenu() {

}

ErrorType DrawingMenu::initStorage() {
	ErrorType et = NVSDrawing.initStorage();
	if(et.ok()) {
		et = NVSDrawing.init();
		if(et.ok()) {
			NVSDrawing.logInfo();
		}
	}
	return et;
}

void DrawingMenu::clearStorage() {
	NVSDrawing.wipe();
}

ErrorType DrawingMenu::onInit() {
	ErrorType et;
	clearListBuffer();
	initStorage();

	ColorIndex = 0;

	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			delete pe;
		}
	}
	MyApp::get().getTouch().addObserver(InternalQueueHandler);
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	uint32_t size = MyApp::get().getBackBufferSize();
	et = NVSDrawing.getBlob("s1", MyApp::get().getBackBuffer() , size);
	ESP_LOGI(LOGTAG,"storage size: %u",size);
	if(!et.ok()) {
		if(et != ((esp_err_t)ESP_ERR_NVS_NOT_FOUND)) {
			ESP_LOGI(LOGTAG,"Clearing storage due to error: %s", et.toString());
			clearStorage();
		} 
	} 
	drawVirtualButtons();
	return ErrorType();
}
	
void DrawingMenu::drawVirtualButtons() {
	MyApp::get().getDisplay().fillRec(0,14,14,14, libesp::RGBColor::WHITE);
	//Save
	MyApp::get().getDisplay().drawString(4,16,"S",RGBColor::BLACK);
	//Delete
	MyApp::get().getDisplay().fillRec(0,40,14,14, libesp::RGBColor::WHITE);
	MyApp::get().getDisplay().drawString(4,42,"D",RGBColor::BLACK);
	//Color
	MyApp::get().getDisplay().fillRec(0,66,15,15, Colors[ColorIndex]);
}
/*
BaseMenu *DrawingMenu::processVirtualButtons(Point2Ds &touchPoint) {
	BaseMenu *retVal = this;
	if(touchPoint.getY()>=14&&touchPoint.getY()<=28) {
		if(NVSDrawing.setBlob("s1",MyApp::get().getBackBuffer(),MyApp::get().getBackBufferSize()).ok()) {
				NVSDrawing.commit();
				retVal = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState(), (const char *)"Save Successful", 2000);
		} else {
			retVal = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState(), (const char *)"Save Failed", 2000);
		}
	} else if(touchPoint.getY()>=40&&touchPoint.getY()<=54) {
		retVal = MyApp::get().getDisplayMessageState(MyApp::get().getMenuState(), (const char *)"Abandoning Drawing...\nWas it that bad???", 2000);
		NVSDrawing.eraseKey("s1");
	} else if(touchPoint.getY()>=66&&touchPoint.getY()<=80) {
		if(ColorIndex==MAX_COLORS-1) {
			ColorIndex = 0;
		} else {
			++ColorIndex;
		}
	}
	return retVal;
}
*/
/*
BaseMenu *DrawingMenu::processButtons() {
	BaseMenu *retVal = this;
	if(upAction()) {
		if(ColorIndex==MAX_COLORS-1) {
			ColorIndex = 0;
		} else {
			++ColorIndex;
		}
	} else if(downAction()) {
		if(ColorIndex==0) {
			ColorIndex = MAX_COLORS-1;
		} else {
			--ColorIndex;
		}
	} else if(selectAction()) {
		retVal = MyApp::get().getMenuState();
	}
	return this;
}
*/
libesp::BaseMenu::ReturnStateContext DrawingMenu::onRun() {
	BaseMenu *nextState = this;
	drawVirtualButtons();
	for(int i=0;i<10;++i) {
		TouchNotification *pe = nullptr;
		bool isPenDown = false;
		uint16_t penX = 0;
		uint16_t penY = 0;
		uint16_t penZ = 0;
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			penX = pe->getX();
			penY = pe->getY();
			penZ = pe->getZ();
			isPenDown = pe->isPenDown();
			delete pe;
			Point2Ds screenPoint(penX,penY);
			Point2Ds TouchPosInBuf = MyApp::get().getCalibrationMenu()->getPickPoint(screenPoint);
			if(isPenDown) {
				if(TouchPosInBuf.getX()>15) {
					MyApp::get().getDisplay().drawPixel(TouchPosInBuf.getX(),TouchPosInBuf.getY(),Colors[ColorIndex]);
				}
			} else {
				if(TouchPosInBuf.getX()<15) {
					//nextState = processVirtualButtons(TouchPosInBuf);
				}
			}
		}
	}
	if(nextState==this) {
	//	nextState = processButtons();
	}
	
	return libesp::BaseMenu::ReturnStateContext(nextState);
}

ErrorType DrawingMenu::onShutdown() {
	MyApp::get().getTouch().removeObserver(InternalQueueHandler);
	//NVSDrawing.close();
	return ErrorType();
}

