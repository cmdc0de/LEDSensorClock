/*
 * update_menu.cpp
 *
 *      Author: cmdc0de
 */

#include "update_menu.h"
#include <device/display/display_device.h>
#include "device/display/color.h"
#include "device/touch/XPT2046.h"
#include "menu_state.h"
#include "../app.h"
#include <esp_log.h>
#include <system.h>
#include <net/ota.h>
#include <app/display_message_state.h>
#include "../app.h"
#include <system.h>
#include "esp_http_client.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::TouchNotification;
using libesp::System;

static StaticQueue_t TouchQueue;
static uint8_t TouchQueueBuffer[UpdateMenu::QUEUE_SIZE*UpdateMenu::TOUCH_MSG_SIZE] = {0};
const char *UpdateMenu::LOGTAG = "UpdateMenu";

class UpdateProgress : public libesp::OTAProgress {
public:
   UpdateProgress() : libesp::OTAProgress() {}
protected:
   virtual void onUpdate(const PROGRESS &p) {
      char buffer[64] = {'\0'};
      char buffer1[64] = {'\0'};
      char buffer2[64] = {'\0'};
	   MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
      uint32_t delayTime = 1000;
      switch(p) {
         case INIT:
            sprintf(&buffer[0], "INIT");
            break;
         case CONNECTED:
            sprintf(&buffer[0], "CONNECTED");
            break;
         case HTTP_READ:
            sprintf(&buffer[0], "HTTP_READ");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            delayTime = 200;
            break;
         case HTTP_READ_ERROR:
            sprintf(&buffer[0], "HTTP_READ_ERROR");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            break;
         case NO_NEW_VERSION_AVAIL:
            sprintf(&buffer[0], "NO_NEW_VERSION_AVAIL");
            sprintf(&buffer1[0], "Current: %s", this->getCurrentVersion());
            sprintf(&buffer2[0], "Downloaded: %s", this->getDownloadVersion());
            break;
         case IMAGE_HEADER_CHECK_SUCCESSFUL:
            sprintf(&buffer[0], "IMAGE_HEADER_CHECK_SUCCESSFUL");
            sprintf(&buffer1[0], "Current: %s", this->getCurrentVersion());
            sprintf(&buffer2[0], "Downloaded: %s", this->getDownloadVersion());
            break;
         case OTA_BEGIN_FAILED:
            sprintf(&buffer[0], "OTA_BEGIN_FAILED");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            break;
         case OTA_WRITE_UPDATE_START:
            sprintf(&buffer[0], "OTA_WRITE_UPDATE_START");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            break;
         case OTA_FAILED_WRITE:
            sprintf(&buffer[0], "OTA_FAILED_WRITE");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            break;
         case COMPLETE:
            sprintf(&buffer[0], "COMPLETE");
            sprintf(&buffer1[0], "Bytes: %d", this->getSingleReadBytes());
            sprintf(&buffer2[0], "Total Bytes: %d", this->getTotalBytes());
            break;
         case PARITION_SWAP_COMPLETE:
            sprintf(&buffer[0], "PARITION_SWAP_COMPLETE");
            sprintf(&buffer1[0], "Current: %s", this->getCurrentVersion());
            sprintf(&buffer2[0], "Downloaded: %s", this->getDownloadVersion());
            break;
      }
      MyApp::get().getDisplay().drawString(2,20,&buffer[0]);
      MyApp::get().getDisplay().drawString(2,40,&buffer1[0]);
      MyApp::get().getDisplay().drawString(2,60,&buffer2[0]);
      MyApp::get().getDisplay().swap();
	   vTaskDelay(delayTime / portTICK_RATE_MS);
   }
};



UpdateMenu::UpdateMenu() : AppBaseMenu(), TouchQueueHandle() {
	
	TouchQueueHandle = xQueueCreateStatic(QUEUE_SIZE,TOUCH_MSG_SIZE,&TouchQueueBuffer[0],&TouchQueue);
}

UpdateMenu::~UpdateMenu() {

}


ErrorType UpdateMenu::onInit() {
	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(TouchQueueHandle, &pe, 0)) {
			delete pe;
		}
	}
	MyApp::get().getTouch().addObserver(TouchQueueHandle);
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	return ErrorType();
}
#if 0
static void http_native_request()
{
   static const char *TAG="TEST";
#define MAX_HTTP_OUTPUT_BUFFER 1024
    char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};   // Buffer to store response of http request
    int content_length = 0;
    esp_http_client_config_t config = {
        .url = "http://httpbin.org/get",
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // GET Request
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    } else {
        content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
            ESP_LOGE(TAG, "HTTP client fetch headers failed");
        } else {
            int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
            if (data_read >= 0) {
                ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
                ESP_LOG_BUFFER_HEX(TAG, output_buffer, data_read);
            } else {
                ESP_LOGE(TAG, "Failed to read response");
            }
        }
    }
    esp_http_client_close(client);
}
#endif

BaseMenu::ReturnStateContext UpdateMenu::onRun() {
	BaseMenu *nextState = MyApp::get().getMenuState();
	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(TouchQueueHandle, &pe, 0)) {
			delete pe;
		}
	}
   //ESP_LOGI(LOGTAG,"test connection");
   //http_native_request();
   //ESP_LOGI(LOGTAG,"end test connection");

	ESP_LOGI(LOGTAG,"UpdateMenu: Free: %u, Min %u",System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
   UpdateProgress UP;
   ErrorType et = MyApp::get().getOTA().run(&UP);
   if(!et.ok()) {
      char buf[64];
      sprintf(&buf[0],"Error: %s", et.toString());
	   MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
      MyApp::get().getDisplay().drawString(2,90,&buf[0]);
      MyApp::get().getDisplay().swap();
	   vTaskDelay(3000 / portTICK_RATE_MS);
   }

	return ReturnStateContext(nextState);
}

ErrorType UpdateMenu::onShutdown() {
	MyApp::get().getTouch().removeObserver(TouchQueueHandle);
	return ErrorType();
}


