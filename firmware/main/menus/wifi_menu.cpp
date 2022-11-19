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
#include <net/utilities.h>
#include <time.h>
#include "../config.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::XPT2046;
using libesp::Point2Ds;
using libesp::TouchNotification;
using libesp::RGBColor;
using libesp::System;

const char *WiFiMenu::WIFIAPSSID = "LEDClockSensor";
const char *WiFiMenu::LOGTAG = "WIFIMENU";
const char *WiFiMenu::MENUHEADER = "Connection Log";
const char *WiFiMenu::WIFISID    = "WIFISID";
const char *WiFiMenu::WIFIPASSWD = "WPASSWD";
const char *WiFiMenu::TZKEY      = "TZKEY";
const char *WiFiMenu::CLKNAME    = "My Sensor Clock";
static etl::vector<libesp::WiFiAPRecord,16> ScanResults;
static const uint32_t FILE_PATH_MAX = 128;

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
    , GET_TZ
    , SET_TZ
    , GET_SETTINGS
    , SET_SETTINGS
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
      case GET_TZ:
        return MyApp::get().getWiFiMenu()->handleGetTZ(r);
        break;
      case SET_TZ:
        return MyApp::get().getWiFiMenu()->handleSetTZ(r);
        break;
      case GET_SETTINGS:
        return MyApp::get().getWiFiMenu()->handleGetSettings(r);
        break;
      case SET_SETTINGS:
        return MyApp::get().getWiFiMenu()->handleSetSettings(r);
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
static RequestContextInfo GetTZCtx(RequestContextInfo::HandlerType::GET_TZ);
static RequestContextInfo SetTZCtx(RequestContextInfo::HandlerType::SET_TZ);
static RequestContextInfo GetSettingsCtx(RequestContextInfo::HandlerType::GET_SETTINGS);
static RequestContextInfo SetSettingsCtx(RequestContextInfo::HandlerType::SET_SETTINGS);

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
  ESP_LOGI(LOGTAG, "Contet Type set to %s", type);
  httpd_resp_set_type(req, type);
}

esp_err_t WiFiMenu::readHttp(httpd_req_t *req, char *buf, uint32_t bufLen) {
  int total_len = req->content_len;
  int cur_len = 0;
  int received = 0;
  if (total_len >= bufLen) {
    ESP_LOGE(LOGTAG,"content too long");
    return ESP_FAIL;
  }
  while (cur_len < total_len) {
    received = httpd_req_recv(req, buf + cur_len, total_len);
    if (received <= 0) {
      ESP_LOGE(LOGTAG,"Failed to post control value");
      return ESP_FAIL;
    }
    cur_len += received;
  }
  buf[total_len] = '\0';
  return ESP_OK;
}

esp_err_t WiFiMenu::handleSetSettings(httpd_req_t *req) {
  ESP_LOGI(LOGTAG,"handleSetSettings");
  ErrorType et = ESP_OK;
  char buf[128];
 
  et = readHttp(req, &buf[0], sizeof(buf));

  if(et.ok()) {
    ESP_LOGI(LOGTAG,"before decode: %s", &buf[0]);
    char decodeBuf[128];
    urlDecode(&buf[0], &decodeBuf[0], sizeof(decodeBuf));
    ESP_LOGI(LOGTAG,"after decode: %s", &decodeBuf[0]);
    char Name[64] = {'\0'};
    char strValue[16] = {'\0'};
    int32_t value = 0;
    if(ESP_OK==httpd_query_key_value(&decodeBuf[0],"name", &Name[0], sizeof(Name) )) {
      if(ESP_OK==httpd_query_key_value(&decodeBuf[0],"value", &strValue[0], sizeof(strValue) )) {
        et = MyApp::get().getConfig().setSetting(&Name[0],&strValue[0]);
        if(et.ok()) {
          static const char *pageData = "<html><head><title>Set Setting</title><meta http-equiv=\"refresh\" content=\"5;URL='/setting'\"/></head><body><p>Setting  Saved Successfully</p></body></html>";
          httpd_resp_sendstr(req, pageData);
        } else {
          ESP_LOGI(LOGTAG,"failed to save setting %s with value %d", &Name[0], value);
        }
      } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "value");
      }
    } else {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "name");
    }
  } else {
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed read data");
  }
  return et.getErrT();
}

esp_err_t WiFiMenu::handleGetSettings(httpd_req_t *req) {
  cJSON *root = cJSON_CreateArray();
  MyApp::get().getConfig().getAllSettings(root);
  const char *info = cJSON_Print(root);
  ESP_LOGI(LOGTAG, "%s", info);
  httpd_resp_sendstr(req, info);
  free((void *)info);
  cJSON_Delete(root);
  return ESP_OK;
}

esp_err_t WiFiMenu::handleRoot(httpd_req_t *req) {
  ESP_LOGI(LOGTAG, "HANDLE ROOT");
  char filepath[FILE_PATH_MAX];
  strlcpy(filepath, "/www", sizeof(filepath));
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
  } else {
    ESP_LOGI(LOGTAG, "File opened %s", filepath);
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
  , NTPTime(), SSID(), Password(), Flags(0), ReTryCount(0), WebServer(), TimeZone() {
  memset(&TimeZone[0],0,sizeof(TimeZone));
  strcpy(&TimeZone[0],"Etc/GMT");
}


ErrorType WiFiMenu::hasWiFiBeenSetup() {
  ESP_LOGI(LOGTAG,"hasWiFiBeenSetup");
  char data[128] = {'\0'};
  uint32_t len = sizeof(data);
  ErrorType et = MyApp::get().getNVS().getValue(WIFISID, &data[0], len);
  if(et.ok()) {
    SSID = data;
    len = sizeof(data);
    et = MyApp::get().getNVS().getValue(WIFIPASSWD, &data[0], len);
    if(et.ok()) {
      Password = &data[0];
      ESP_LOGI(LOGTAG, "ssid %s: pass: %s", SSID.c_str(), "not telling"); //Password.c_str());
    } else {
      ESP_LOGI(LOGTAG, "error getting password: %u %d %s", len, et.getErrT(), et.toString());
    }
  } else {
    ESP_LOGI(LOGTAG,"failed to load wifisid: %u %d %s", len, et.getErrT(), et.toString()); 
  }
  return et;
}

ErrorType WiFiMenu::setWiFiConnectionData(const char *ssid, const char *pass) {
  ESP_LOGI(LOGTAG,"con data %s %s", ssid,pass);
  libesp::NVSStackCommit nvssc(&MyApp::get().getNVS());
  ErrorType et = MyApp::get().getNVS().setValue(WIFISID, ssid);
  if(et.ok()) {
    et = MyApp::get().getNVS().setValue(WIFIPASSWD,pass);
  }
  return et;
}

ErrorType WiFiMenu::clearConnectData() {
  libesp::NVSStackCommit nvssc(&MyApp::get().getNVS());
  ErrorType et = MyApp::get().getNVS().eraseKey(WIFISID);
  if(!et.ok()) {
    ESP_LOGI(LOGTAG,"failed to erase key ssid: %d %s", et.getErrT(), et.toString()); 
  } 
  et = MyApp::get().getNVS().eraseKey(WIFIPASSWD);
  if(!et.ok()) {
    ESP_LOGI(LOGTAG,"failed to erase key password: %d %s", et.getErrT(), et.toString()); 
  }
  if(et.ok()) {
    ESP_LOGI(LOGTAG, "connection data cleared from NVS storage!");
  }
  return et;
}

void WiFiMenu::setTZ() {
  setenv("TZ", &TimeZone[0], 1);
  tzset();
  ESP_LOGI(LOGTAG,"Timezone set to: %s", &TimeZone[0]);
}
ErrorType WiFiMenu::initWiFiForSTA() {
   MyWiFi.setWifiEventHandler(this);
   ErrorType et = MyWiFi.init(WIFI_MODE_STA);
   if(et.ok()) {
      et = NTPTime.init(MyApp::get().getNVS(),true,time_sync_cb);
      if(et.ok()) {
         uint32_t len = sizeof(TimeZone); 
         et = MyApp::get().getNVS().getValue(TZKEY, &TimeZone[0], len);
         if(!et.ok()) {
            strcpy(&TimeZone[0],"Etc/GMT");
            ESP_LOGI(LOGTAG,"Timezone not set, defaulting to %s", &TimeZone[0]);
         } else {
            ESP_LOGI(LOGTAG,"Timezone loaded: %s", &TimeZone[0]);
         }
      } else {
         ESP_LOGE(LOGTAG,"failed to set up NTP");
      }
   } else {
      ESP_LOGE(LOGTAG,"failed to init for STA");
   }
   setTZ();
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
   ESP_LOGI(LOGTAG,"startAP called");
   MyWiFi.setWifiEventHandler(this);
   ErrorType et;
   extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
   extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
   extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
   extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");

   et = WebServer.init(cacert_pem_start, (cacert_pem_end-cacert_pem_start), prvtkey_pem_start, (prvtkey_pem_end-prvtkey_pem_start));
   ESP_LOGI(LOGTAG,"Web Server initialized but not started");
   if(et.ok()) {
      //TODO pass in SID and password to INIT so we can have the right name
      ErrorType et = MyWiFi.init(WIFI_MODE_APSTA); //must be APSTA otherwise we can't scan
      if(et.ok()) {
         ESP_LOGI(LOGTAG,"initwififorap call successfully");
         //et = MyWiFi.startAP(WIFIAPSSID, ""); //start AP
         if(et.ok()) {
            ESP_LOGI(LOGTAG,"************AP STARTED**************");
         } else {
            ESP_LOGE(LOGTAG,"Ap failed to start");
         }
      } else {
         ESP_LOGE(LOGTAG, "failed to init web server");
      }
   } else {
      ESP_LOGE(LOGTAG,"failed init wifi for ap: %s", et.toString());
   }
   return et;
}

bool WiFiMenu::stopWiFi() {
  return MyWiFi.stopWiFi() && MyWiFi.shutdown();
}

esp_err_t WiFiMenu::handleScan(httpd_req_t *req) {
  ESP_LOGI(LOGTAG,"handleScan");
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
  ESP_LOGI(LOGTAG,"handleSetConData");
  ErrorType et = ESP_OK;
  char buf[128];

  et = readHttp(req, buf, sizeof(buf));

  if(et.ok()) {
    ESP_LOGI(LOGTAG,"before decode: %s", &buf[0]);
    char decodeBuf[128];
    urlDecode(&buf[0], &decodeBuf[0], sizeof(decodeBuf));
    ESP_LOGI(LOGTAG,"after decode: %s", &decodeBuf[0]);
    char idVal[8] = {'\0'};
    int id = -1;
    char pass[64] = {'\0'};
    if((et=httpd_query_key_value(&decodeBuf[0],"id", &idVal[0], sizeof(idVal))).ok()) {
      id = atoi(&idVal[0]);
      if((et=httpd_query_key_value(&decodeBuf[0],"password", &pass[0], sizeof(pass))).ok()) {
        et = setWiFiConnectionData(ScanResults[id].getSSID().c_str(), &pass[0]);
        if(et.ok()) {
          static const char *pageData = "<html><head><title>TZ Set</title><meta http-equiv=\"refresh\" content=\"5;URL='/index.html'\"/></head><body><p>Connect Data  Saved Successfully</p></body></html>";
          httpd_resp_sendstr(req, pageData);
        }
      } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "invalid ID");
      }
    } else {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "invalid ID");
    }
  } else {
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read");
  }
  return et.getErrT();
}

esp_err_t WiFiMenu::handleCalibration(httpd_req_t *req) {
  esp_err_t et = ESP_OK;
  httpd_resp_set_type(req, "application/json");
  cJSON *root = cJSON_CreateObject();
  MyApp::get().getCalibrationMenu()->calibrationData(root);
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
  httpd_resp_set_type(req, "text/html");
  const char *info = "<html><body><div><h2>ESP 32 is rebooting and will enter touch calibration mode</h2></div></body></html>";
  httpd_resp_sendstr(req, info);
	vTaskDelay(1000 / portTICK_RATE_MS);
  libesp::System::get().restart();
  return et;
}

esp_err_t WiFiMenu::handleGetTZ(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  uint32_t len = sizeof(TimeZone); 
  ErrorType et = MyApp::get().getNVS().getValue(TZKEY, &TimeZone[0], len);
  if(!et.ok()) {
    strcpy(&TimeZone[0],"Etc/GMT");
  }
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "TZ", &TimeZone[0]);
  const char *info = cJSON_Print(root);
  httpd_resp_sendstr(req, info);
  free((void *)info);
  cJSON_Delete(root);
  return ESP_OK;
}

esp_err_t WiFiMenu::handleSetTZ(httpd_req_t *req) {
  ESP_LOGI(LOGTAG,"handleSetTZ");
  ErrorType et = ESP_OK;
  char buf[128];
  
  et = readHttp(req, buf, sizeof(buf));
  if(et.ok()) {
    ESP_LOGI(LOGTAG,"Posted JSON: %s", &buf[0]);

    cJSON *root = cJSON_Parse(&buf[0]);
    int offset = cJSON_GetObjectItem(root,"offset")->valueint;
    offset*=-1;//not sure why I have to do this??? AZ is UTC - 7 but must be set as UTC+7
    sprintf(&TimeZone[0],"UTC%d",offset);
    et = MyApp::get().getNVS().setValue(TZKEY, &TimeZone[0]);
    cJSON_Delete(root);

    if(et.ok()) {
      setTZ();
      static const char *pageData = "{result: 'ok'}";
      httpd_resp_sendstr(req, pageData);
    } else {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to save TimeZone");
    }
  } else {
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read");
  }
  return et.getErrT();
}

esp_err_t WiFiMenu::handleSystemInfo(httpd_req_t *req) {
  ESP_LOGI(LOGTAG,"handleSystemInfo");
  httpd_resp_set_type(req, "application/json");
  esp_chip_info_t ChipInfo;
  esp_chip_info(&ChipInfo);
  cJSON *root = cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "Free HeapSize", System::get().getFreeHeapSize());
  cJSON_AddNumberToObject(root, "Free Min HeapSize",System::get().getMinimumFreeHeapSize());
  cJSON_AddNumberToObject(root, "Free 32 Bit HeapSize",heap_caps_get_free_size(MALLOC_CAP_32BIT));
  cJSON_AddNumberToObject(root, "Free 32 Bit Min HeapSize",heap_caps_get_minimum_free_size(MALLOC_CAP_32BIT));
  cJSON_AddNumberToObject(root, "Free DMA HeapSize",heap_caps_get_free_size(MALLOC_CAP_DMA));
  cJSON_AddNumberToObject(root, "Free DMA Min HeapSize",heap_caps_get_minimum_free_size(MALLOC_CAP_DMA));
  cJSON_AddNumberToObject(root, "Free Internal HeapSize",heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  cJSON_AddNumberToObject(root, "Free Internal Min HeapSize",heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
  cJSON_AddNumberToObject(root, "Free Default HeapSize",heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
  cJSON_AddNumberToObject(root, "Free Default Min HeapSize",heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
  cJSON_AddNumberToObject(root, "Free Exec",heap_caps_get_free_size(MALLOC_CAP_EXEC));
  cJSON_AddNumberToObject(root, "Free Exec Min",heap_caps_get_minimum_free_size(MALLOC_CAP_EXEC));
  cJSON_AddNumberToObject(root, "Model", ChipInfo.model);
  cJSON_AddNumberToObject(root, "Features", ChipInfo.features);
  cJSON_AddNumberToObject(root, "EMB_FLASH", (ChipInfo.features&CHIP_FEATURE_EMB_FLASH)!=0);
  cJSON_AddNumberToObject(root, "WIFI_BGN", (ChipInfo.features&CHIP_FEATURE_WIFI_BGN)!=0);
  cJSON_AddNumberToObject(root, "BLE", (ChipInfo.features&CHIP_FEATURE_BLE)!=0);
  cJSON_AddNumberToObject(root, "BT", (ChipInfo.features&CHIP_FEATURE_BT)!=0);
  cJSON_AddNumberToObject(root, "Cores", (int)ChipInfo.cores);
  cJSON_AddNumberToObject(root, "Revision", (int)ChipInfo.revision);
  cJSON_AddStringToObject(root, "IDF Version", ::esp_get_idf_version());
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
  ESP_LOGI(LOGTAG,"wifiReady");
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

const httpd_uri_t scan = {
  .uri       = "/wifiscan",
  .method    = HTTP_GET,
  .handler   = http_handler,
  .user_ctx  = &ScanCtx
};
const httpd_uri_t conData = {
  .uri       = "/setcon",
  .method    = HTTP_POST,
  .handler   = http_handler,
  .user_ctx  = &SetConCtx
};
const httpd_uri_t cal = {
  .uri       = "/calibration",
  .method    = HTTP_GET,
  .handler   = http_handler,
  .user_ctx  = &CalibrationCtx
};
const httpd_uri_t resetcal = {
  .uri       = "/resetcal",
  .method    = HTTP_POST,
  .handler   = http_handler,
  .user_ctx  = &ResetCalCtx
};
const httpd_uri_t SysInfo = {
  .uri       = "/systeminfo",
  .method    = HTTP_GET,
  .handler   = http_handler,
  .user_ctx  = &SystemInfoCtx
};
const httpd_uri_t GetTZURI = {
  .uri       = "/tz",
  .method    = HTTP_GET,
  .handler   = http_handler,
  .user_ctx  = &GetTZCtx
};
const httpd_uri_t SetTZURI = {
  .uri       = "/settz",
  .method    = HTTP_POST,
  .handler   = http_handler,
  .user_ctx  = &SetTZCtx
};
const httpd_uri_t GetSettings = {
  .uri       = "/settings",
  .method    = HTTP_GET,
  .handler   = http_handler,
  .user_ctx  = &GetSettingsCtx
};
const httpd_uri_t SetSettings = {
  .uri       = "/setsetting",
  .method    = HTTP_POST,
  .handler   = http_handler,
  .user_ctx  = &SetSettingsCtx
};
static const httpd_uri_t root = {
  .uri       = "/",
  .method    = HTTP_GET,
  .handler   = http_handler,
  .user_ctx  = &RootCtx
};
static const httpd_uri_t st = {
  .uri       = "/static/*",
  .method    = HTTP_GET,
  .handler   = http_handler,
  .user_ctx  = &RootCtx
};

ErrorType WiFiMenu::apStart() {
  ErrorType et;
  Flags|=AP_START;
  ESP_LOGI(LOGTAG,"AP Started");
  et = WebServer.start();
  if(et.ok()) {
    et = WebServer.registerHandle(scan);
    if(et.ok())  et = WebServer.registerHandle(conData);
    else ESP_LOGI(LOGTAG,"registering handle: %d: %s", et.getErrT(), et.toString());

    if(et.ok())  et = WebServer.registerHandle(cal);
    else ESP_LOGI(LOGTAG,"registering handle: %d: %s", et.getErrT(), et.toString());

    if(et.ok()) et = WebServer.registerHandle(resetcal);
    else ESP_LOGI(LOGTAG,"registering handle: %d: %s", et.getErrT(), et.toString());
    
    if(et.ok())  et = WebServer.registerHandle(SysInfo);
    else ESP_LOGI(LOGTAG,"registering handle: %d: %s", et.getErrT(), et.toString());
    
    if(et.ok()) et = WebServer.registerHandle(GetTZURI);
    else ESP_LOGI(LOGTAG,"registering handle: %d: %s", et.getErrT(), et.toString());
    
    if(et.ok())  et = WebServer.registerHandle(SetTZURI);
    else ESP_LOGI(LOGTAG,"registering handle: %d: %s", et.getErrT(), et.toString());
    
    if(et.ok())  et = WebServer.registerHandle(GetSettings);
    else ESP_LOGI(LOGTAG,"registering handle: %d: %s", et.getErrT(), et.toString());
    
    if(et.ok())  et = WebServer.registerHandle(SetSettings);
    else ESP_LOGI(LOGTAG,"registering handle: %d: %s", et.getErrT(), et.toString());
   
    if(et.ok())  et = WebServer.registerHandle(root);
    else ESP_LOGI(LOGTAG,"registering handle: %d: %s", et.getErrT(), et.toString());

    if(et.ok())  et = WebServer.registerHandle(st);
    else ESP_LOGI(LOGTAG,"registering handle: %d: %s", et.getErrT(), et.toString());
  } else {
    ESP_LOGI(LOGTAG,"Error starting web server: %d: %s", et.getErrT(), et.toString());
  }
  return et;
}

ErrorType WiFiMenu::apStop() {
  ErrorType et = WebServer.stop();
  Flags=Flags&~AP_START;
  ESP_LOGI(LOGTAG,"AP Stopped");
  return et;
}

ErrorType WiFiMenu::staConnected(system_event_sta_connected_t *info) {
  ErrorType et;
  Flags|=CONNECTED;
  Flags=(Flags&~CONNECTING);
  ReTryCount = 0;
  ESP_LOGI(LOGTAG,"staConnected");
  NTPTime.start();
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

ErrorType WiFiMenu::staGotIp(system_event_sta_got_ip_t *info) {
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

