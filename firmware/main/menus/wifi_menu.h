#ifndef WIFI_STATE_H
#define WIFI_STATE_H

#include "appbase_menu.h"
#include <device/touch/XPT2046.h>
#include <net/wifi.h>
#include <net/networktimeprotocol.h>
#include <net/wifieventhandler.h>
//#include <device/display/layout.h>

class WiFiMenu: public AppBaseMenu, libesp::WiFiEventHandler {
public:
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(libesp::TouchNotification*);
  typedef libesp::WiFi::SSIDTYPE SSIDTYPE;
  typedef libesp::WiFi::PASSWDTYPE PASSWDTYPE;
  static const char *LOGTAG;
  static const char *MENUHEADER;
  static const char *WIFISID;
  static const char *WIFIPASSWD;
	WiFiMenu();
	virtual ~WiFiMenu();
  static const uint32_t MAX_RETRY_CONNECT_COUNT = 10;
  static const uint32_t NOSTATE = 0;
  static const uint32_t CONNECTING = 1<<0;
  static const uint32_t CONNECTED = 1<<1;
  static const uint32_t WIFI_READY = 1<<2;
  static const uint32_t HAS_IP = 1<<3;
  static const uint32_t SCAN_COMPLETE = 1<<4;
  enum INTERNAL_STATE {
    INIT = 0
    , SCAN_RESULTS
    , DISPLAY_SINGLE_SSID
  };
public:
  libesp::ErrorType hasWiFiBeenSetup();
  libesp::ErrorType connect();
  bool isConnected();
  libesp::ErrorType initWiFi();
  libesp::ErrorType clearConnectData();
public:
  virtual libesp::ErrorType staStart();
  virtual libesp::ErrorType staStop();
  virtual libesp::ErrorType wifiReady();
	virtual libesp::ErrorType apStaConnected(wifi_event_ap_staconnected_t *info);
	virtual libesp::ErrorType apStaDisconnected(wifi_event_ap_stadisconnected_t *info);
	virtual libesp::ErrorType apStart();
	virtual libesp::ErrorType apStop();
	virtual libesp::ErrorType staConnected(system_event_sta_connected_t *info);
	virtual libesp::ErrorType staDisconnected(system_event_sta_disconnected_t *info);
	virtual libesp::ErrorType staGotIp(system_event_sta_got_ip_t info);
	virtual libesp::ErrorType staScanDone(system_event_sta_scan_done_t *info);
	virtual libesp::ErrorType staAuthChange(system_event_sta_authmode_change_t *info);
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	QueueHandle_t InternalQueueHandler;
	//libesp::StaticGridLayout MyLayout;
  libesp::WiFi MyWiFi;
  libesp::NTP NTPTime;
  SSIDTYPE    SSID;
  PASSWDTYPE  Password;
  uint32_t    Flags;
  uint16_t    ReTryCount;
	libesp::GUIListItemData Items[AppBaseMenu::NumRows];
	libesp::GUIListData MenuList;
  INTERNAL_STATE InternalState;
	static const uint16_t ItemCount = uint16_t(sizeof(Items) / sizeof(Items[0]));
};

#endif
