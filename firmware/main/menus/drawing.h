#ifndef DRAWING_MENU_H
#define DRAWING_MENU_H

#include "appbase_menu.h"
#include <math/point.h>

namespace libesp {
	class TouchNotification;
}

class DrawingMenu: public AppBaseMenu {
public:
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(libesp::TouchNotification*);
	static const char *LOGTAG;
public:
	DrawingMenu();
	virtual ~DrawingMenu();
	libesp::ErrorType initStorage();
	void clearStorage();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
protected:
	void drawVirtualButtons();
private:
	QueueHandle_t InternalQueueHandler;
	uint8_t ColorIndex;	
	int8_t LastSaveIndex;
};
#endif
