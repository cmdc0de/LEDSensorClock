#ifndef MHZ19BTASK_H
#define MHZ19BTASK_H
#pragma once

#include <task.h>
#include <device/sensor/mhz19b.h>
#include <error_type.h>
#include "appmsg.h"

class MHZ19BMsg : public MyAppMsg {
public:
  MHZ19BMsg() {}
  virtual bool handleMessage(MyApp *);
  virtual ~MHZ19BMsg() {}
private:
};

class MHZ19BTask : public Task {
public:
  MHZ19BTask();
  libesp::ErrorType init(uart_port_t uartp, gpio_num_t tx, gpio_num_t rx);
	virtual void run(void *data) override;
private:
  int16_t co2;
  mhz19b_dev_t dev;
  char version[6];
  uint16_t range;
  bool autocal;
};

#endif
