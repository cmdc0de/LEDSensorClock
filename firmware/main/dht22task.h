#ifndef DHT22TASK_H
#define DHT22TASK_H
#pragma once

#include <task.h>
#include <device/sensor/dht22.h>

class DHT22Task : public Task {
public:
  DHT22Task();
  libesp::ErrorType init(gpio_num_t dht11Pin);
	virtual void run(void *data) override;
private:
  libesp::DHT22 Sensor;
};

#endif
