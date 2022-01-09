#ifndef DHT11TASK_H
#define DHT11TASK_H
#pragma once

#include <task.h>
#include <device/sensor/dht11.h>

class DHT11Task : public Task {
public:
  DHT11Task();
  libesp::ErrorType init(gpio_num_t dht11Pin);
	virtual void run(void *data) override;
private:
  libesp::DHT11 Sensor;
};

#endif
