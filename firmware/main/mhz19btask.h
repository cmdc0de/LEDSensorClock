#ifndef MHZ19TASK_H
#define MHZ19TASK_H
#pragma once

#include <task.h>
#include <device/sensor/mhz19b.h>
#include <error_type.h>
#include "appmsg.h"

class MHZ19Msg : public MyAppMsg {
public:
  MHZ19Msg(int16_t v) : MyAppMsg(), CO2(v) {}
  virtual bool handleMessage(MyApp *);
  virtual ~MHZ19Msg() {}
private:
  int16_t CO2;
};

class MHZ19Task : public Task {
public:
  MHZ19Task();
  libesp::ErrorType init(uart_port_t uartp, gpio_num_t tx, gpio_num_t rx);
	virtual void run(void *data) override;
private:
  libesp::MHZ19 CO2Sensor;
};

#endif
