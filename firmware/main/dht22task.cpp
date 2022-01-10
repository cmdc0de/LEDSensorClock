
#include "dht22task.h"
#include "app.h"

static const char *LOGTAG = "DHT22Task";
  
bool DHT22Msg::handleMessage(MyApp *p) {
  p->setTempHumidity(THData.temperature, THData.humidity);
  return true;
}

DHT22Task::DHT22Task() : Task("DHT22Task"), Sensor() {
}


libesp::ErrorType DHT22Task::init(gpio_num_t dht11Pin) {
  return Sensor.init(dht11Pin);
}

void DHT22Task::run(void *data) {
	ESP_LOGI(LOGTAG,"DHT22Task::run");
  while(1) {
    libesp::DHT22::Data d;
    libesp::ErrorType et = Sensor.read(d);
    if(et.ok()) {
      ESP_LOGI(LOGTAG,"Temp: %f, Humidity %f",d.temperature, d.humidity);
      DHT22Msg *msg = new DHT22Msg(d);
      if(errQUEUE_FULL==xQueueSend(MyApp::get().getMessageQueueHandle(), &msg, 0)) {
        delete msg;
      }
    } else {
      ESP_LOGE(LOGTAG,"Failed to read Sensor: %s", et.toString());
    }
	  vTaskDelay(5000 / portTICK_RATE_MS);
  }
}

