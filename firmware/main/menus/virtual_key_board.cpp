/*
 * virtual_key_board.cpp
 *
 *  Created on: Feb 5, 2022
 *      Author: libesp
 */

#include "virtual_key_board.h"
#include <string>
#include "../app.h"
#include <freertos.h>

using libesp::RGBColor;
using libesp::FreeRTOS;

const char *VirtualKeyBoard::STDKBLowerCase = "abcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()-";
const char *VirtualKeyBoard::STDKBNames = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()-_";
const char *VirtualKeyBoard::STDCAPS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void VirtualKeyBoard::InputHandleContext::addChar(char b) {
	if(Buf!=0) {
		if(CurrentPos<(Size-1)) {
			Buf[CurrentPos] = b;
			++CurrentPos;
		} else {
			//do nothing
		}
	}
}

void VirtualKeyBoard::InputHandleContext::clear() {
  CurrentPos = 0;
  memset(&Buf[0],0,Size);
}

//Remember CurrentPos here means the current position in the input buffer not the position in the keyboard like below.
void VirtualKeyBoard::InputHandleContext::backspace() {
	Buf[CurrentPos] = '\0';
	--CurrentPos;
}

////////

VirtualKeyBoard::VirtualKeyBoard(): VKB(0), SizeOfKeyboard(0), XDisplayPos(0)
	, XEndDisplayPos(MyApp::get().getLastCanvasWidthPixel()), YDisplayPos(0), FontColor(RGBColor::WHITE), BackGround(RGBColor::BLACK)
	, CursorColor(RGBColor::BLUE), CursorChar('_'), CursorPos(0), CharsPerRow(0), InputContext(0) {


}

void VirtualKeyBoard::init(const char *vkb, InputHandleContext *ic, int16_t xdisplayPos, int16_t xEndDisplay, int16_t yDisplayPos, const libesp::RGBColor &fontColor,
		const libesp::RGBColor &backgroundColor, const libesp::RGBColor &cursorColor, char cursorChar) {
	VKB = vkb;
	SizeOfKeyboard = strlen(VKB);
	XDisplayPos = xdisplayPos;
	XEndDisplayPos = xEndDisplay;
	YDisplayPos = yDisplayPos;
	FontColor = fontColor;
	BackGround = backgroundColor;
	CursorColor = cursorColor;
	CursorChar = cursorChar;
	CursorPos = 0;
	uint8_t FontPixelWidth = MyApp::get().getDisplay().getFont()->FontWidth;
	CharsPerRow = (XEndDisplayPos-XDisplayPos)/FontPixelWidth;
	InputContext = ic;
}

void VirtualKeyBoard::processTouch(const libesp::Point2Ds &touchPos) {
  if(touchPos.getY()>=YDisplayPos && touchPos.getX()>=XDisplayPos && touchPos.getX()<=XEndDisplayPos) {
	  uint8_t FontPixelHeight = MyApp::get().getDisplay().getFont()->FontHeight;
  	uint8_t FontPixelWidth = MyApp::get().getDisplay().getFont()->FontWidth;
	  for(int i=0,y=0;i<SizeOfKeyboard && y < (MyApp::get().getCanvasHeight()-(y*FontPixelHeight));i+=CharsPerRow, ++y) {
      if(touchPos.getY()>=(YDisplayPos+(y*FontPixelHeight)) && touchPos.getY()<=(YDisplayPos+((y+1)*FontPixelHeight))) {
        uint32_t arrayPos = y*CharsPerRow+(touchPos.getX()/FontPixelWidth);
        InputContext->addChar(VKB[arrayPos]);
      }
		}
  }
}

void VirtualKeyBoard::process() {
	uint16_t y = 0;
	const char *ptr = VKB;
	uint8_t FontPixelHeight = MyApp::get().getDisplay().getFont()->FontHeight;
	uint8_t FontPixelWidth = MyApp::get().getDisplay().getFont()->FontWidth;
	uint8_t cursorRow = getCursorY();
	uint8_t curosrColumn = getCursorX();
	for(int i=0;i<SizeOfKeyboard && y < (MyApp::get().getCanvasHeight()-(y*FontPixelHeight));i+=CharsPerRow, ++y) {
		MyApp::get().getDisplay().drawString(XDisplayPos, uint16_t(YDisplayPos+(y*FontPixelHeight)), ptr, FontColor, BackGround, 1, false, CharsPerRow);
		if(y==cursorRow) {
			if((FreeRTOS::getTimeSinceStart()%1000)<500) {
				MyApp::get().getDisplay().drawString(XDisplayPos+(curosrColumn*FontPixelWidth), YDisplayPos+(y*FontPixelHeight), (const char *)"_", CursorColor, BackGround, 1, false);
			}
		}
		ptr = ptr + CharsPerRow;
	}
}


uint8_t VirtualKeyBoard::getCursorX() {
	return CursorPos%CharsPerRow;
}


uint8_t VirtualKeyBoard::getCursorY() {
	return CursorPos/CharsPerRow;

}

uint8_t VirtualKeyBoard::getVKBIndex() {
	return CursorPos;
}


char VirtualKeyBoard::getSelectedChar() {
	return VKB[CursorPos];
}


