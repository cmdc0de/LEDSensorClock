#ifndef WIFI_STATE_H
#define WIFI_STATE_H

#include <device/touch/XPT2046.h>
#include <net/wifi.h>
#include <net/networktimeprotocol.h>
#include <net/wifieventhandler.h>
#include <device/display/layout.h>
#include <net/webserver.h>

class WiFiMenu: public libesp::WiFiEventHandler {
public:
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(libesp::TouchNotification*);
  static const char *WIFIAPSSID;
  typedef libesp::WiFi::SSIDTYPE SSIDTYPE;
  typedef libesp::WiFi::PASSWDTYPE PASSWDTYPE;
  static const char *LOGTAG;
  static const char *MENUHEADER;
  static const char *WIFISID;
  static const char *WIFIPASSWD;
  static const char *TZKEY;
  static const char *CLKNAME;
  static const uint32_t MAX_RETRY_CONNECT_COUNT = 10;

  static const uint32_t NOSTATE = 0;
  static const uint32_t CONNECTING = 1<<0;
  static const uint32_t CONNECTED = 1<<1;
  static const uint32_t WIFI_READY = 1<<2;
  static const uint32_t HAS_IP = 1<<3;
  static const uint32_t SCAN_COMPLETE = 1<<4;
  static const uint32_t AP_START = 1<<5;


  enum INTERNAL_STATE {
    INIT = 0
    , CONFIG_CONNECTION
    , AWAITING_AP
    , SCAN_RESULTS
    , DISPLAY_SINGLE_SSID
  };
public:
	WiFiMenu();
	virtual ~WiFiMenu();
  libesp::ErrorType hasWiFiBeenSetup();
  libesp::ErrorType connect();
  bool isConnected();
  libesp::ErrorType initWiFiForSTA();
  libesp::ErrorType clearConnectData();
  libesp::ErrorType startAP();
  bool stopWiFi();
  esp_err_t handleRoot(httpd_req_t *req);
  esp_err_t handleScan(httpd_req_t *req);
  esp_err_t handleSetConData(httpd_req_t *req);
  esp_err_t handleCalibration(httpd_req_t *req);
  esp_err_t handleResetCalibration(httpd_req_t *req);
  esp_err_t handleSystemInfo(httpd_req_t *req);
  esp_err_t handleGetTZ(httpd_req_t *req);
  esp_err_t handleSetTZ(httpd_req_t *req);
  esp_err_t handleGetSettings(httpd_req_t *req);
  esp_err_t handleSetSettings(httpd_req_t *req);
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
	virtual libesp::ErrorType staGotIp(system_event_sta_got_ip_t *info);
	virtual libesp::ErrorType staScanDone(system_event_sta_scan_done_t *info);
	virtual libesp::ErrorType staAuthChange(system_event_sta_authmode_change_t *info);
   libesp::ErrorType setWiFiConnectionData(const char *ssid, const char *pass);
protected:
  bool isFlagSet(uint32_t f) {return ((f&Flags)!=0);}
  void setContentTypeFromFile(httpd_req_t *req, const char *filepath);
  void setTZ();
  esp_err_t readHttp(httpd_req_t *req, char *buf, uint32_t bufLen);
private:
  libesp::WiFi MyWiFi;
  libesp::NTP NTPTime;
  SSIDTYPE    SSID;
  PASSWDTYPE  Password;
  uint32_t    Flags;
  uint16_t    ReTryCount;
  libesp::HTTPWebServer WebServer;
  char        TimeZone[32];
};

#endif
