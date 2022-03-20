/*
 * setting_state.h
 *
 *      Author: cmdc0de
 */

#ifndef SETTING_STATE_H_
#define SETTING_STATE_H_

#include "appbase_menu.h"
#include <device/display/layout.h>

namespace libesp {
	class TouchNotification;
}

class SettingMenu: public AppBaseMenu {
public:
  enum INTERNAL_STATE {
    SHOW_ALL = 0
    , AP_RUNNING
    , CALIBRATION
  };
public:
	SettingMenu();
	virtual ~SettingMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	QueueHandle_t TouchQueueHandle;
	libesp::StaticGridLayout MyLayout;
  INTERNAL_STATE InternalState;
public:
	static const int TOUCH_QUEUE_SIZE = 4;
	static const int TOUCH_MSG_SIZE = sizeof(libesp::TouchNotification*);
	static const char *LOGTAG;
};


#endif 
