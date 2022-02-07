/*
 * virtual_key_board.h
 *
 */

#ifndef VIRTUAL_KEY_BOARD_H_
#define VIRTUAL_KEY_BOARD_H_

#include <device/display/display_device.h>
#include <etl/string.h>
#include <math/point.h>

class VirtualKeyBoard {
public:
	class InputHandleContext {
	public:
		InputHandleContext(char *b, uint8_t s) : Buf(b), Size(s), CurrentPos(0) {}
		void set(char *b, uint8_t s) { Buf=b; Size=s; CurrentPos=0;}
    void clear();
		void addChar(char b);
		void backspace();
	private:
		char *Buf;
		uint8_t Size;
		uint8_t CurrentPos;
	};
public:
	static const char *STDKBLowerCase;
	static const char *STDKBNames;
	static const char *STDCAPS;
public:
	VirtualKeyBoard();
	void init(const char *vkb, InputHandleContext *ic, int16_t xdisplayPos, int16_t xEndDisplay, int16_t yDisplayPos, const libesp::RGBColor &fontColor,
			const libesp::RGBColor &backgroundColor, const libesp::RGBColor &cursorColor, const char cursorChar = '_');
	void process();
  void processTouch(const libesp::Point2Ds &touchPos);
	uint8_t getCursorX();
	uint8_t getCursorY();
	uint8_t getVKBIndex();
	char getSelectedChar();
private:
	const char *VKB;
	uint16_t SizeOfKeyboard;
	int16_t XDisplayPos;
	int16_t XEndDisplayPos;
	int16_t YDisplayPos;
	libesp::RGBColor FontColor;
	libesp::RGBColor BackGround;
	libesp::RGBColor CursorColor;
	char CursorChar;
	int16_t CursorPos;
	int16_t CharsPerRow;
	InputHandleContext *InputContext;
};



#endif 
