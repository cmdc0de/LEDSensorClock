/*
 * setting_state.h
 *
 *      Author: cmdc0de
 */

#pragma once

#include "appbase_menu.h"
#include <device/display/layout.h>

namespace libesp {
	class TouchNotification;
}

class UserConfigMenu: public AppBaseMenu {
public:
	UserConfigMenu();
	virtual ~UserConfigMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	QueueHandle_t TouchQueueHandle;
	libesp::StaticGridLayout MyLayout;
public:
	static const int TOUCH_QUEUE_SIZE = 4;
	static const int TOUCH_MSG_SIZE = sizeof(libesp::TouchNotification*);
	static const char *LOGTAG;
};

