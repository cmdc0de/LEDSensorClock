
#include "mhz19btask.h"
#include "app.h"

static const char *LOGTAG = "MHZ19Task";
  
bool MHZ19Msg::handleMessage(MyApp *p) {
  p->setCO2(CO2);
  return true;
}

MHZ19Task::MHZ19Task() : Task("MHZ19BTask"), CO2Sensor()  {
}


libesp::ErrorType MHZ19Task::init(uart_port_t uartp, gpio_num_t tx, gpio_num_t rx) {
  libesp::ErrorType et = CO2Sensor.init(uartp, tx, rx);
  if(et.ok()) {
    uint8_t count = 0;
    while (!CO2Sensor.detect() && count++<5) {
      ESP_LOGI(LOGTAG, "MHZ-19B not detected, waiting...");
      vTaskDelay(1000 / portTICK_RATE_MS);
    }

    if(et.ok() && count<=5) {
      ESP_LOGI(LOGTAG, "MHZ-19 firmware version: %s", CO2Sensor.getVersion());

      et = CO2Sensor.setRange(libesp::MHZ19::MHZ19_RANGE::MHZ19_RANGE_5000);
      if(et.ok()) {
        et = CO2Sensor.setAutoCalibration(true);
        if(et.ok()) {
          uint16_t range;
          et = CO2Sensor.getRange(range);
          if(et.ok()) {
            ESP_LOGI(LOGTAG, "  range: %d", range);

            bool autocal = false;
            et = CO2Sensor.getAutoCalibration(autocal);
            if(et.ok()) {
              ESP_LOGI(LOGTAG, "  autocal: %s", autocal ? "ON" : "OFF");
            } else {
              ESP_LOGE(LOGTAG, "error getting auto cal %s", et.toString());
            }
          } else {
            ESP_LOGE(LOGTAG, "error getting range: %s", et.toString());
          }
        } else {
          ESP_LOGE(LOGTAG, "error getting setAutoCal: %s", et.toString());
        }
      } else {
        ESP_LOGE(LOGTAG, "erorr set Range: %s", et.toString());
      }
    } else {
      et = libesp::ErrorType::TIMEOUT_ERROR; 
    }
  }
  return et;
}

void MHZ19Task::run(void *data) {
	ESP_LOGI(LOGTAG,"MHZ19BTask::run");
  while(1) {
    int16_t co2;
    CO2Sensor.readCO2(co2);
    ESP_LOGI(LOGTAG, "CO2: %d", co2);
      MHZ19Msg *msg = new MHZ19Msg(co2);
      if(errQUEUE_FULL==xQueueSend(MyApp::get().getMessageQueueHandle(), &msg, 0)) {
        delete msg;
      }
    vTaskDelay(15000 / portTICK_RATE_MS);
  }
}

