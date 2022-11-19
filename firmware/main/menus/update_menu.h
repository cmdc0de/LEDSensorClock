/*
 *
 *      Author: cmdc0de
 */

#pragma once

#include "appbase_menu.h"

namespace libesp {
	class TouchNotification;
}

class UpdateMenu: public AppBaseMenu {
public:
	UpdateMenu();
	virtual ~UpdateMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	QueueHandle_t TouchQueueHandle;
public:
	static const int QUEUE_SIZE = 4;
	static const int TOUCH_MSG_SIZE = sizeof(libesp::TouchNotification*);
	static const char *LOGTAG;
};

