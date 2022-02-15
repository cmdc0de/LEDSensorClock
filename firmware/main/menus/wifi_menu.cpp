#include "wifi_menu.h"
#include "../app.h"
#include "calibration_menu.h"
#include "gui_list_processor.h"
#include <app/display_message_state.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include "menu_state.h"
#include <math/rectbbox.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::XPT2046;
using libesp::Point2Ds;
using libesp::TouchNotification;
using libesp::RGBColor;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[WiFiMenu::QUEUE_SIZE*WiFiMenu::MSG_SIZE] = {0};
const char *WiFiMenu::LOGTAG = "WIFIMENU";
const char *WiFiMenu::MENUHEADER = "Connection Log";
const char *WiFiMenu::WIFISID = "WIFISID";
const char *WiFiMenu::WIFIPASSWD = "WIFIPASSWD";

static const int8_t NUM_INTERFACE_ITEMS = 1;
static libesp::AABBox2D Close(Point2Ds(185,7),6);
static libesp::Button CloseButton((const char *)"X", MyApp::CLOSE_BTN_ID, &Close,RGBColor::RED, RGBColor::BLUE);
static libesp::Widget *InterfaceElements[NUM_INTERFACE_ITEMS] = {&CloseButton};


void time_sync_cb(struct timeval *tv) {
    ESP_LOGI(WiFiMenu::LOGTAG, "Notification of a time synchronization event");
}

/* An HTTP GET handler */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "<h1>LEDSensor Clock Setup:</h1>", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = 0
    //.is_websocket = 0,
    //.handle_ws_control_frame =0,
    //.supported_subprotocol = 0
};

WiFiMenu::WiFiMenu() : AppBaseMenu(), WiFiEventHandler(), InternalQueueHandler(), MyWiFi()
  , NTPTime(), SSID(), Password(), Flags(0), ReTryCount(0), Items()
  , MenuList(MENUHEADER, Items, 0, 0, MyApp::get().getLastCanvasWidthPixel(), MyApp::get().getLastCanvasHeightPixel(), 0, ItemCount)
  , InternalState(INIT)
	, MyLayout(&InterfaceElements[0],NUM_INTERFACE_ITEMS, MyApp::get().getLastCanvasWidthPixel(), MyApp::get().getLastCanvasHeightPixel(), false)
  , WebServer() {

	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
	MyLayout.reset();
}


ErrorType WiFiMenu::hasWiFiBeenSetup() {
  char data[64] = {'\0'};
  uint32_t len = sizeof(data);
  ErrorType et = MyApp::get().getNVS().getValue(WIFISID, &data[0],len);
  if(et.ok()) {
    SSID = data;
    et = MyApp::get().getNVS().getValue(WIFIPASSWD, &data[0],len);
    Password = &data[0];
  } else {
    ESP_LOGI(LOGTAG,"failed to load wifisid: %d %s", et.getErrT(), et.toString()); 
  }
  return et;
}

ErrorType WiFiMenu::clearConnectData() {
  ErrorType et = MyApp::get().getNVS().eraseKey(WIFISID);
  if(!et.ok()) {
    ESP_LOGI(LOGTAG,"failed to erase key ssid: %d %s", et.getErrT(), et.toString()); 
  } 
  et = MyApp::get().getNVS().eraseKey(WIFIPASSWD);
  if(!et.ok()) {
    ESP_LOGI(LOGTAG,"failed to erase key password: %d %s", et.getErrT(), et.toString()); 
  }
  return et;
}

ErrorType WiFiMenu::initWiFi() {
  MyWiFi.setWifiEventHandler(this);
  ErrorType et = MyWiFi.init(WIFI_MODE_APSTA);
  if(et.ok()) {
    et = NTPTime.init(MyApp::get().getNVS(),true,time_sync_cb);
    if(et.ok()) {
      extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
      extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
      extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
      extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");

      et = WebServer.init(cacert_pem_start, (cacert_pem_end-cacert_pem_start), prvtkey_pem_start, (prvtkey_pem_end-prvtkey_pem_start));
    }
  }
  return et;
}

ErrorType WiFiMenu::connect() {
  ErrorType et = MyWiFi.connect(SSID,Password,WIFI_AUTH_OPEN);
  if(et.ok()) {
    Flags|=CONNECTING;
  }
  return et;
}

bool WiFiMenu::isConnected() {
  return (Flags&CONNECTED)!=0;
}


WiFiMenu::~WiFiMenu() {

}

static etl::vector<libesp::WiFiAPRecord,16> ScanResults;

ErrorType WiFiMenu::onInit() {
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			delete pe;
		}
	}
	MyApp::get().getTouch().addObserver(InternalQueueHandler);
  clearListBuffer();
  for(int i=0;i<ItemCount;++i) {
		Items[i].text = getRow(i);
		Items[i].id = i;
		Items[i].setShouldScroll();
	}
 
  ScanResults.clear();
	return ErrorType();
}

ErrorType WiFiMenu::startAP() {
  InternalState = CONFIG_CONNECTION;
  return MyWiFi.startAP("LEDClockSensor", ""); //start AP
}

bool WiFiMenu::stopAP() {
  return MyWiFi.stopWiFi();
}

void WiFiMenu::handleAP() {
  if(isFlagSet(AP_START)) {
    //if web server started ()
  }
}

libesp::BaseMenu::ReturnStateContext WiFiMenu::onRun() {
	BaseMenu *nextState = this;


  Point2Ds TouchPosInBuf;
  libesp::Widget *widgetHit = nullptr;
  TouchNotification *pe = nullptr;
  if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
    Point2Ds screenPoint(pe->getX(),pe->getY());
    TouchPosInBuf = MyApp::get().getCalibrationMenu()->getPickPoint(screenPoint);
    ESP_LOGI(LOGTAG,"TouchPoint: X:%d Y:%d PD:%d", int32_t(TouchPosInBuf.getX()), int32_t(TouchPosInBuf.getY()), pe->isPenDown()?1:0);
    bool penUp = !pe->isPenDown();
    delete pe;
    widgetHit = MyLayout.pick(TouchPosInBuf);
    if(widgetHit && penUp) {
      ESP_LOGI(LOGTAG, "Widget %s hit\n", widgetHit->getName());
      switch(widgetHit->getWidgetID()) {
        case MyApp::CLOSE_BTN_ID:
          nextState = MyApp::get().getMenuState();
          break;
        default:
          break;
      } 
    }
  } 
  MyLayout.draw(&MyApp::get().getDisplay());
  switch(InternalState) {
    case CONFIG_CONNECTION:
      handleAP();
      break;
    case AWAITING_AP:
      break;
    default:
      break;
  }
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType WiFiMenu::onShutdown() {
	MyApp::get().getTouch().removeObserver(InternalQueueHandler);
	return ErrorType();
}

// wifi handler
ErrorType WiFiMenu::staStart() {
  ErrorType et;
  //ESP_LOGI(LOGTAG, __PRETTY_FUNCTION__ );
  return et;
}

ErrorType WiFiMenu::staStop() {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

ErrorType WiFiMenu::wifiReady() {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  Flags|=WIFI_READY;
  return et;
}

ErrorType WiFiMenu::apStaConnected(wifi_event_ap_staconnected_t *info) {
  ErrorType et;
  Flags|=CONNECTED;
  Flags=(Flags&~CONNECTING);
  ESP_LOGI(LOGTAG,"apstaConnected");
  return et;
}

ErrorType WiFiMenu::apStaDisconnected(wifi_event_ap_stadisconnected_t *info) {
  ErrorType et;
  Flags=(Flags&~(CONNECTED|HAS_IP));
  ESP_LOGI(LOGTAG,"apstaDisconnected");
  NTPTime.stop();
  if(++ReTryCount<MAX_RETRY_CONNECT_COUNT) {
    return connect();
  }
  return ErrorType(ErrorType::MAX_RETRIES);
}

ErrorType WiFiMenu::apStart() {
  ErrorType et;
  Flags|=AP_START;
  ESP_LOGI(LOGTAG,"AP Started");
  et = WebServer.start();
  et = WebServer.registerHandle(root);
  return et;
}

ErrorType WiFiMenu::apStop() {
  ErrorType et;
  Flags=Flags&~AP_START;
  ESP_LOGI(LOGTAG,"AP Stopped");
  //stop web server
  return et;
}

ErrorType WiFiMenu::staConnected(system_event_sta_connected_t *info) {
  ErrorType et;
  Flags|=CONNECTED;
  Flags=(Flags&~CONNECTING);
  ReTryCount = 0;
  ESP_LOGI(LOGTAG,"staConnected");
  return et;
}

ErrorType WiFiMenu::staDisconnected(system_event_sta_disconnected_t *info) {
  ESP_LOGI(LOGTAG,"staDisconnected");
  Flags=(Flags&~(CONNECTED|HAS_IP));
  NTPTime.stop();
  if(++ReTryCount<MAX_RETRY_CONNECT_COUNT) {
    return connect();
  }
  return ErrorType(ErrorType::MAX_RETRIES);
}

ErrorType WiFiMenu::staGotIp(system_event_sta_got_ip_t info) {
  ErrorType et;
  ESP_LOGI(LOGTAG,"Have IP");
  Flags|=HAS_IP;
  ReTryCount = 0;
  NTPTime.start();
  return et;
}

ErrorType WiFiMenu::staScanDone(system_event_sta_scan_done_t *info) {
  ErrorType et;
  ESP_LOGI(LOGTAG,"Scan Complete");
  Flags|=SCAN_COMPLETE;
  return et;
}

ErrorType WiFiMenu::staAuthChange(system_event_sta_authmode_change_t *info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

