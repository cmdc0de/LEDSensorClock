
#include "dht11task.h"

static const char *LOGTAG = "DHT11Task";

DHT11Task::DHT11Task() : Task("DHT11Task"), Sensor() {
}


libesp::ErrorType DHT11Task::init(gpio_num_t dht11Pin) {
  return Sensor.init(dht11Pin);
}

void DHT11Task::run(void *data) {
	ESP_LOGI(LOGTAG,"DHT11Task::run");
  while(1) {
    libesp::DHT11::Data d;
    libesp::ErrorType et = Sensor.read(d);
    if(et.ok()) {
      ESP_LOGI(LOGTAG,"Temp: %d, Humidity %d",d.temperature, d.humidity);
    } else {
      ESP_LOGE(LOGTAG,"Failed to read Sensor: %s", et.toString());
    }
	  vTaskDelay(5000 / portTICK_RATE_MS);
  }
}

