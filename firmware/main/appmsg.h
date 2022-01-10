#ifndef APP_MESSAGE_H
#define APP_MESSAGE_H

#pragma once

class MyApp;

class MyAppMsg {
public:
  virtual bool handleMessage(MyApp *pApp) = 0;
  virtual ~MyAppMsg() {}
};


#endif
