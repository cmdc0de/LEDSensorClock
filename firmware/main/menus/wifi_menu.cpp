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
#include <cJSON.h>
#include <system.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::XPT2046;
using libesp::Point2Ds;
using libesp::TouchNotification;
using libesp::RGBColor;

const char *WiFiMenu::LOGTAG = "WIFIMENU";
const char *WiFiMenu::MENUHEADER = "Connection Log";
const char *WiFiMenu::WIFISID = "WIFISID";
const char *WiFiMenu::WIFIPASSWD = "WIFIPASSWD";
static etl::vector<libesp::WiFiAPRecord,16> ScanResults;

void time_sync_cb(struct timeval *tv) {
    ESP_LOGI(WiFiMenu::LOGTAG, "Notification of a time synchronization event");
}

struct RequestContextInfo {
  enum HandlerType {
    ROOT
    , SCAN
    , SET_CON_DATA
    , CALIBRATION
    , RESET_CALIBRATION
    , SYSTEM_INFO
  };
  HandlerType HType;
  RequestContextInfo(const HandlerType &ht) : HType(ht) {}
  esp_err_t go(httpd_req_t *r) {
    switch(HType) {
      case ROOT:
        return MyApp::get().getWiFiMenu()->handleRoot(r);
        break;
      case SCAN:
        return MyApp::get().getWiFiMenu()->handleScan(r);
        break;
        break;
      case SET_CON_DATA:
        return MyApp::get().getWiFiMenu()->handleSetConData(r);
        break;
      case CALIBRATION:
        return MyApp::get().getWiFiMenu()->handleCalibration(r);
        break;
      case RESET_CALIBRATION:
        return MyApp::get().getWiFiMenu()->handleResetCalibration(r);
        break;
      case SYSTEM_INFO:
        return MyApp::get().getWiFiMenu()->handleSystemInfo(r);
        break;
      default:
        return ESP_OK;
        break;
    }
  }
};

static RequestContextInfo RootCtx(RequestContextInfo::HandlerType::ROOT);
static RequestContextInfo ScanCtx(RequestContextInfo::HandlerType::SCAN);
static RequestContextInfo SetConCtx(RequestContextInfo::HandlerType::SET_CON_DATA);
static RequestContextInfo CalibrationCtx(RequestContextInfo::HandlerType::CALIBRATION);
static RequestContextInfo ResetCalCtx(RequestContextInfo::HandlerType::RESET_CALIBRATION);
static RequestContextInfo SystemInfoCtx(RequestContextInfo::HandlerType::SYSTEM_INFO);

static esp_err_t http_handler(httpd_req_t *req) {
  RequestContextInfo *rci = reinterpret_cast<RequestContextInfo *>(req->user_ctx);
  return rci->go(req);
}

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

void WiFiMenu::setContentTypeFromFile(httpd_req_t *req, const char *filepath) {
  const char *type = "text/plain";
  if (CHECK_FILE_EXTENSION(filepath, ".html")) {
    type = "text/html";
  } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
    type = "application/javascript";
  } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
    type = "text/css";
  } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
    type = "image/png";
  } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
    type = "image/x-icon";
  } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
    type = "text/xml";
  }
  httpd_resp_set_type(req, type);
}

static const uint32_t FILE_PATH_MAX = 128;

esp_err_t WiFiMenu::handleRoot(httpd_req_t *req) {
  char filepath[FILE_PATH_MAX];
  strlcpy(filepath, "www", sizeof(filepath));
  if (req->uri[strlen(req->uri) - 1] == '/') {
    strlcat(filepath, "/index.html", sizeof(filepath));
  } else {
    strlcat(filepath, req->uri, sizeof(filepath));
  }
  int fd = open(filepath, O_RDONLY, 0);
  if (fd == -1) {
    ESP_LOGE(LOGTAG, "Failed to open file : %s", filepath);
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
    return ESP_FAIL;
  }

  setContentTypeFromFile(req, filepath);
  char scratchBuffer[1024];

  char *chunk = &scratchBuffer[0];
  ssize_t read_bytes;
  do {
    /* Read file in chunks into the scratch buffer */
    read_bytes = read(fd, chunk, sizeof(scratchBuffer));
    if (read_bytes == -1) {
      ESP_LOGE(LOGTAG, "Failed to read file : %s", filepath);
    } else if (read_bytes > 0) {
      /* Send the buffer contents as HTTP response chunk */
      if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
        close(fd);
        ESP_LOGE(LOGTAG, "File sending failed!");
        /* Abort sending file */
        httpd_resp_sendstr_chunk(req, NULL);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
        return ESP_FAIL;
      }
    }
  } while (read_bytes > 0);
  /* Close file after sending complete */
  close(fd);
  ESP_LOGI(LOGTAG, "File sending complete");
  /* Respond with an empty chunk to signal HTTP response completion */
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

WiFiMenu::WiFiMenu() : WiFiEventHandler(), MyWiFi()
  , NTPTime(), SSID(), Password(), Flags(0), ReTryCount(0), WebServer() {

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

ErrorType WiFiMenu::setWiFiConnectionData(const char *ssid, const char *pass) {
  ErrorType et = MyApp::get().getNVS().setValue(WIFISID, ssid);
  if(et.ok()) {
    et = MyApp::get().getNVS().setValue(WIFIPASSWD,pass);
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


ErrorType WiFiMenu::startAP() {
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

esp_err_t WiFiMenu::handleScan(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  cJSON *root = cJSON_CreateArray();
  ErrorType et = MyWiFi.scan(ScanResults,false);
  for(uint32_t i = 0;i<ScanResults.size();++i) {
    cJSON *sr = cJSON_CreateObject();
    cJSON_AddNumberToObject(sr, "id", i);
    cJSON_AddStringToObject(sr, "ssid", ScanResults[i].getSSID().c_str());
    cJSON_AddNumberToObject(sr, "rssi", ScanResults[i].getRSSI());
    cJSON_AddNumberToObject(sr, "channel", ScanResults[i].getPrimary());
    cJSON_AddStringToObject(sr, "authMode", ScanResults[i].getAuthModeString());
    cJSON_AddItemToArray(root,sr);
  }
  const char *info = cJSON_Print(root);
  ESP_LOGI(LOGTAG, "%s", info);
  httpd_resp_sendstr(req, info);
  free((void *)info);
  cJSON_Delete(root);
  return et.getErrT();
}

esp_err_t WiFiMenu::handleSetConData(httpd_req_t *req) {
  ErrorType et = ESP_OK;
  char buf[256];

  int total_len = req->content_len;
  int cur_len = 0;
  int received = 0;
  if (total_len >= sizeof(buf)) {
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
    return ESP_FAIL;
  }
  while (cur_len < total_len) {
    received = httpd_req_recv(req, buf + cur_len, total_len);
    if (received <= 0) {
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
      return ESP_FAIL;
    }
    cur_len += received;
  }
  buf[total_len] = '\0';

  cJSON *root = cJSON_Parse(buf);
  int id = cJSON_GetObjectItem(root, "id")->valueint;
  const char *passwd = cJSON_GetObjectItem(root, "password")->string;
  if(id>0 && id<ScanResults.size()) {
    et = setWiFiConnectionData(ScanResults[id].getSSID().c_str(), passwd);
  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "invalid ID");
  }
  cJSON_Delete(root);
  httpd_resp_sendstr(req, "Post control value successfully");
  return et.getErrT();
}

esp_err_t WiFiMenu::handleCalibration(httpd_req_t *req) {
  esp_err_t et = ESP_OK;
  httpd_resp_set_type(req, "application/json");
  cJSON *root = cJSON_CreateArray();
  cJSON *sr = cJSON_CreateObject();
  MyApp::get().getCalibrationMenu()->calibrationData(sr);
  cJSON_AddItemToArray(root,sr);
  const char *info = cJSON_Print(root);
  ESP_LOGI(LOGTAG, "%s", info);
  httpd_resp_sendstr(req, info);
  free((void *)info);
  cJSON_Delete(root);
  return et;
}

esp_err_t WiFiMenu::handleResetCalibration(httpd_req_t *req) {
  esp_err_t et = ESP_OK;
  MyApp::get().getCalibrationMenu()->eraseCalibration();
  libesp::System::get().restart();
  return et;
}

esp_err_t WiFiMenu::handleSystemInfo(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  cJSON *root = cJSON_CreateObject();
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  cJSON_AddStringToObject(root, "version", IDF_VER);
  cJSON_AddNumberToObject(root, "cores", chip_info.cores);
  const char *sys_info = cJSON_Print(root);
  httpd_resp_sendstr(req, sys_info);
  free((void *)sys_info);
  cJSON_Delete(root);
  return ESP_OK;
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
  //ok to make these local as data is copied
  const httpd_uri_t scan = {
    .uri       = "/scan",
    .method    = HTTP_GET,
    .handler   = http_handler,
    .user_ctx  = &ScanCtx
  };
  et = WebServer.registerHandle(scan);
  const httpd_uri_t conData = {
    .uri       = "/setcon",
    .method    = HTTP_POST,
    .handler   = http_handler,
    .user_ctx  = &SetConCtx
  };
  et = WebServer.registerHandle(conData);
  const httpd_uri_t cal = {
    .uri       = "/calibration",
    .method    = HTTP_GET,
    .handler   = http_handler,
    .user_ctx  = &CalibrationCtx
  };
  et = WebServer.registerHandle(cal);
  const httpd_uri_t resetcal = {
    .uri       = "/resetcal",
    .method    = HTTP_POST,
    .handler   = http_handler,
    .user_ctx  = &ResetCalCtx
  };
  et = WebServer.registerHandle(resetcal);
  const httpd_uri_t SysInfo = {
    .uri       = "/systeminfo",
    .method    = HTTP_GET,
    .handler   = http_handler,
    .user_ctx  = &SystemInfoCtx
  };
  et = WebServer.registerHandle(SysInfo);
  static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = http_handler,
    .user_ctx  = &RootCtx
  };
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

