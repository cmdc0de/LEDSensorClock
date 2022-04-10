#pragma once

#include <nvs_memory.h>
#include <error_type.h>
#include <etl/map.h>
#include <device/display/color.h>

class cJSON;

struct ConfigItem {
  enum TYPE {
    STRING
      , INT
      , UINT
      , COLOR
      , PERCENT
  };
  ConfigItem::TYPE ItemType;
  union {
    char CharValue[16];
    int32_t IntValue;
    uint32_t UIntValue;
  } Value;
  ConfigItem(ConfigItem::TYPE it, const char *strValue) : ItemType(it), Value() {
    strncpy(&Value.CharValue[0],strValue, sizeof(Value.CharValue));
  }
  ConfigItem(ConfigItem::TYPE it, int32_t iv) : ItemType(it), Value() {
    Value.IntValue = iv;
  }
  ConfigItem(ConfigItem::TYPE it, uint32_t iv) : ItemType(it), Value() {
    Value.IntValue = iv;
  }
};

class Config {
  public:
    static const char *SECONDS_ON_MAIN_SCREEN;
    static const char *SECONDS_ON_GAME_OF_LIFE;
    static const char *SECONDS_ON_3D;
    static const char *SECONDS_SHOWING_DRAWING;
    static const char *TEMP_IN_C;
    static const char *SEC_START_COLOR;
    static const char *SEC_END_COLOR;
    static const char *MIN_START_COLOR;
    static const char *MIN_END_COLOR;
    static const char *HOUR_START_COLOR;
    static const char *HOUR_END_COLOR;
    static const char *MAX_BRIGHTNESS;
    static const char *LOGTAG;
  public:
    Config(libesp::NVS *nvs);
    ~Config();
    libesp::ErrorType init();
    libesp::ErrorType getAllSettings(cJSON *r);
    libesp::ErrorType setSetting(const char *name, const char *value);
    bool wantC();
    uint32_t getSecondsOnMainScreen();
    uint32_t getSecondsOnGameOfLife();
    uint32_t getSecondsOn3D();
    libesp::RGBColor getSecondHandStartColor();
    libesp::RGBColor getSecondHandEndColor();
    libesp::RGBColor getMinHandStartColor();
    libesp::RGBColor getMinHandEndColor();
    libesp::RGBColor getHourStartColor();
    libesp::RGBColor getHourEndColor();
    uint8_t getMaxBrightness();
  private:
    typedef etl::map<const char *, ConfigItem *, 12> SettingMapType;
    typedef SettingMapType::iterator SettingMapTypeIT;
    SettingMapType SettingMap;
    libesp::NVS *NVSStorage;
};


