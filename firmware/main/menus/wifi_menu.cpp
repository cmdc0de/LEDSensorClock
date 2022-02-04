#include "wifi_menu.h"
#include "../app.h"
#include "calibration_menu.h"
#include "gui_list_processor.h"
#include <app/display_message_state.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include "menu_state.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::XPT2046;
using libesp::Point2Ds;
using libesp::TouchNotification;
using libesp::RGBColor;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[WiFiMenu::QUEUE_SIZE*WiFiMenu::MSG_SIZE] = {0};
const char *WiFiMenu::LOGTAG = "SSID                  RSSI  CH";
const char *WiFiMenu::WIFISID = "WIFISID";
const char *WiFiMenu::WIFIPASSWD = "WIFIPASSWD";

void time_sync_cb(struct timeval *tv) {
    ESP_LOGI(WiFiMenu::LOGTAG, "Notification of a time synchronization event");
}

WiFiMenu::WiFiMenu() : AppBaseMenu(), WiFiEventHandler(), InternalQueueHandler(), MyWiFi()
  , NTPTime(), SSID(), Password(), Flags(0), ReTryCount(0), Items()
  , MenuList(LOGTAG, Items, 0, 0, MyApp::get().getLastCanvasWidthPixel(), MyApp::get().getLastCanvasHeightPixel(), 0, ItemCount), InternalState(INIT) {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
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
  ErrorType et = MyWiFi.init(WIFI_MODE_STA);
  if(et.ok()) {
    et = NTPTime.init(MyApp::get().getNVS(),true,time_sync_cb);
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
  InternalState = INIT;
	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext WiFiMenu::onRun() {
	BaseMenu *nextState = this;

  if(ScanResults.empty()) {
    clearListBuffer();
    ErrorType et = MyWiFi.scan(ScanResults,false);
    for(uint32_t i = 0;i<ScanResults.size() && i< NumRows;++i) {
      snprintf(getRow(i),AppBaseMenu::RowLength,"%-19.18s  %4d  %3d"
          , ScanResults[i].getSSID().c_str(), ScanResults[i].getRSSI(), ScanResults[i].getPrimary());
    }
    InternalState = SCAN_RESULTS;
  }

	//Point2Ds TouchPosInBuf;
	//libesp::Widget *widgetHit = nullptr;
	//if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
//		Point2Ds screenPoint(pe->getX(),pe->getY());
//		TouchPosInBuf = MyApp::get().getCalibrationMenu()->getPickPoint(screenPoint);
//		ESP_LOGI(LOGTAG,"TouchPoint: X:%d Y:%d PD:%d", int32_t(TouchPosInBuf.getX()),
//								 int32_t(TouchPosInBuf.getY()), pe->isPenDown()?1:0);
 //   penUp = !pe->isPenDown();
	//	delete pe;
		//widgetHit = MyLayout.pick(TouchPosInBuf);
	//}

	bool penUp = false;
  bool hdrHit = false;
  bool pe = processTouch(InternalQueueHandler, MenuList, ItemCount, penUp, hdrHit);
  if(penUp) {
    uint32_t selectedItem = MenuList.selectedItem;
    if(SCAN_RESULTS==InternalState) {
      if(hdrHit) {
        nextState = MyApp::get().getMenuState();
      } else {
        InternalState = DISPLAY_SINGLE_SSID;
        clearListBuffer();
        snprintf(getRow(0),AppBaseMenu::RowLength,"SSID:  %-22.21s", ScanResults[selectedItem].getSSID().c_str());
        snprintf(getRow(1),AppBaseMenu::RowLength,"Auth:  %s", ScanResults[selectedItem].getAuthModeString());
        snprintf(getRow(2),AppBaseMenu::RowLength,"RSSI:  %d", static_cast<int32_t>(ScanResults[selectedItem].getRSSI()));
        snprintf(getRow(3),AppBaseMenu::RowLength,"Channel:  %d", static_cast<int32_t>(ScanResults[selectedItem].getPrimary()));
        snprintf(getRow(4),AppBaseMenu::RowLength,"Capabilites:  B:%s N:%s G:%s LR:%s"
          ,ScanResults[selectedItem].isWirelessB()?"Y":"N", ScanResults[selectedItem].isWirelessN()?"Y":"N"
          ,ScanResults[selectedItem].isWirelessG()?"Y":"N", ScanResults[selectedItem].isWirelessLR()?"Y":"N");
        snprintf(getRow(7),AppBaseMenu::RowLength,"Connect");
        snprintf(getRow(9),AppBaseMenu::RowLength,"Back");
      }
    } else {
      snprintf(getRow(5),AppBaseMenu::RowLength,"Password:  %s", "XXXXX");
      if(selectedItem==7) {
        ESP_LOGI(LOGTAG,"CONNECT!!!!!!");
      } else if (9==selectedItem) {
        InternalState=INIT;
        ScanResults.clear();
      }
    }
  }

  MyApp::get().getGUI().drawList(&MenuList);

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
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

ErrorType WiFiMenu::apStaDisconnected(wifi_event_ap_stadisconnected_t *info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

ErrorType WiFiMenu::apStart() {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

ErrorType WiFiMenu::apStop() {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

ErrorType WiFiMenu::staConnected(system_event_sta_connected_t *info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  Flags|=CONNECTED;
  Flags=(Flags&~CONNECTING);
  return et;
}

ErrorType WiFiMenu::staDisconnected(system_event_sta_disconnected_t *info) {
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  Flags=(Flags&~(CONNECTED|HAS_IP));
  NTPTime.stop();
  if(++ReTryCount<MAX_RETRY_CONNECT_COUNT) {
    return connect();
  }
  return ErrorType(ErrorType::MAX_RETRIES);
}

ErrorType WiFiMenu::staGotIp(system_event_sta_got_ip_t info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  Flags|=HAS_IP;
  ReTryCount = 0;
  NTPTime.start();
  return et;
}

ErrorType WiFiMenu::staScanDone(system_event_sta_scan_done_t *info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  Flags|=SCAN_COMPLETE;
  return et;
}

ErrorType WiFiMenu::staAuthChange(system_event_sta_authmode_change_t *info) {
  ErrorType et;
  //ESP_LOGI(LOGTAG,__FUNCTION__);
  return et;
}

