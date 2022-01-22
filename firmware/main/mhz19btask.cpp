
#include "mhz19btask.h"
#include "app.h"

static const char *LOGTAG = "MHZ19BTask";
  
bool MHZ19BMsg::handleMessage(MyApp *p) {
  return true;
}

MHZ19BTask::MHZ19BTask() : Task("MHZ19BTask"), co2(0), dev(), version(), range(0), autocal(false)  {
}


libesp::ErrorType MHZ19BTask::init(uart_port_t uartp, gpio_num_t tx, gpio_num_t rx) {
  libesp::ErrorType et = mhz19b_init(&dev, uartp, tx, rx);
  if(et.ok()) {
    while (!mhz19b_detect(&dev)) {
      ESP_LOGI(LOGTAG, "MHZ-19B not detected, waiting...");
      vTaskDelay(1000 / portTICK_RATE_MS);
    }

    mhz19b_get_version(&dev, version);
    ESP_LOGI(LOGTAG, "MHZ-19B firmware version: %s", version);
    ESP_LOGI(LOGTAG, "MHZ-19B set range and autocal");

    mhz19b_set_range(&dev, MHZ19B_RANGE_5000);
    mhz19b_set_auto_calibration(&dev, false);

    mhz19b_get_range(&dev, &range);
    ESP_LOGI(LOGTAG, "  range: %d", range);

    mhz19b_get_auto_calibration(&dev, &autocal);
    ESP_LOGI(LOGTAG, "  autocal: %s", autocal ? "ON" : "OFF");
  }
  return et;
}

void MHZ19BTask::run(void *data) {
	ESP_LOGI(LOGTAG,"MHZ19BTask::run");
  while(1) {
    while (mhz19b_is_warming_up(&dev)) {
        ESP_LOGI(LOGTAG, "MHZ-19B is warming up");
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    mhz19b_read_co2(&dev, &co2);
    ESP_LOGI(LOGTAG, "CO2: %d", co2);
    vTaskDelay(5000 / portTICK_RATE_MS);
  }
}

