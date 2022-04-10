#include "config.h"
#include <nvs_memory.h>
#include <cJSON.h>

using libesp::ErrorType;
using libesp::NVS;

const char *Config::SECONDS_ON_MAIN_SCREEN = "SecondsOnMainScreen";
const char *Config::SECONDS_ON_GAME_OF_LIFE = "SecondsInGameOfLife";
const char *Config::SECONDS_ON_3D = "SecondsIn3D";
const char *Config::SECONDS_SHOWING_DRAWING = "SecondsShowingDrawings";
const char *Config::TEMP_IN_C = "ShowTempInC";
const char *Config::SEC_START_COLOR = "SecStartColor";
const char *Config::SEC_END_COLOR = "SecEndColor";
const char *Config::MIN_START_COLOR = "MinStartColor";
const char *Config::MIN_END_COLOR = "MinEndColor";
const char *Config::HOUR_START_COLOR = "HourStartColor";
const char *Config::HOUR_END_COLOR = "HourEndColor";
const char *Config::MAX_BRIGHTNESS = "MaxBrightness";
const char *Config::LOGTAG = "Config";

Config::Config(libesp::NVS *nvs): SettingMap(), NVSStorage(nvs) {

}

Config::~Config() {

}

ErrorType Config::init() {
  ErrorType et;
  ConfigItem *p = new ConfigItem(ConfigItem::UINT, 300);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(SECONDS_ON_MAIN_SCREEN, p));
  p = new ConfigItem(ConfigItem::UINT, 300);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(SECONDS_ON_GAME_OF_LIFE, p));
  p = new ConfigItem(ConfigItem::UINT, 300);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(SECONDS_ON_3D, p));
  p = new ConfigItem(ConfigItem::UINT, 120);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(SECONDS_SHOWING_DRAWING, p));
  p = new ConfigItem(ConfigItem::INT, 0);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(TEMP_IN_C, p));
  p = new ConfigItem(ConfigItem::UINT, 0x0000FF);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(SEC_START_COLOR, p));
  p = new ConfigItem(ConfigItem::UINT, 0xFF0000);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(SEC_END_COLOR, p));
  p = new ConfigItem(ConfigItem::UINT, 0x00FF00);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(MIN_START_COLOR, p));
  p = new ConfigItem(ConfigItem::UINT, 0xFF0000);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(MIN_END_COLOR, p));
  p = new ConfigItem(ConfigItem::UINT, 0xFFFFFF);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(HOUR_START_COLOR, p));
  p = new ConfigItem(ConfigItem::UINT, 0xFF0000);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(HOUR_END_COLOR, p));
  p = new ConfigItem(ConfigItem::PERCENT, 50);
  SettingMap.insert(SettingMap.end(), ETL_OR_STD::make_pair(MAX_BRIGHTNESS, p));

  SettingMapTypeIT it = SettingMap.begin();
  while(it!=SettingMap.end()) {
    switch((*it).second->ItemType) {
      case ConfigItem::STRING: 
        {
          size_t len = sizeof((*it).second->Value.CharValue);
          et = NVSStorage->getValue((*it).first,&(*it).second->Value.CharValue[0], len);
          ESP_LOGI(LOGTAG,"RetVal: %d Loaded Value - Name: %s Value: %s", et.getErrT(), (*it).first, &(*it).second->Value.CharValue[0]);
        }
        break;
      case ConfigItem::INT:
        {
          et = NVSStorage->getValue((*it).first,(*it).second->Value.IntValue);
          ESP_LOGI(LOGTAG,"RetVal: %d Loaded Value - Name: %s Value: %d", et.getErrT(), (*it).first, (*it).second->Value.IntValue);
        }
        break;
      case ConfigItem::UINT:
      case ConfigItem::COLOR:
      case ConfigItem::PERCENT:
        {
          et = NVSStorage->getValue((*it).first,(*it).second->Value.UIntValue);
          ESP_LOGI(LOGTAG,"RetVal: %d Loaded Value - Name: %s Value: %u", et.getErrT(), (*it).first, (*it).second->Value.UIntValue);
        }
        break;
    }
    ++it;
  }

  return et;
}

bool Config::wantC() {
  return SettingMap[TEMP_IN_C]->Value.IntValue==1;
}
    
uint32_t Config::getSecondsOnMainScreen() {
  return SettingMap[SECONDS_ON_MAIN_SCREEN]->Value.UIntValue;
}

uint32_t Config::getSecondsOnGameOfLife() {
  return SettingMap[SECONDS_ON_GAME_OF_LIFE]->Value.UIntValue;
}

uint32_t Config::getSecondsOn3D() {
  return SettingMap[SECONDS_ON_3D]->Value.UIntValue;
}

uint8_t Config::getMaxBrightness() {
  return SettingMap[MAX_BRIGHTNESS]->Value.UIntValue;
}

libesp::RGBColor Config::getSecondHandStartColor() {
  uint32_t v = SettingMap[SEC_START_COLOR]->Value.UIntValue;
  return libesp::RGBColor((v>>16)&0xFF, (v>>8)&0xFF,v&0xFF);
}

libesp::RGBColor Config::getSecondHandEndColor() {
  uint32_t v = SettingMap[SEC_END_COLOR]->Value.UIntValue;
  return libesp::RGBColor((v>>16)&0xFF, (v>>8)&0xFF,v&0xFF);
}

libesp::RGBColor Config::getMinHandStartColor() {
  uint32_t v = SettingMap[MIN_START_COLOR]->Value.UIntValue;
  return libesp::RGBColor((v>>16)&0xFF, (v>>8)&0xFF,v&0xFF);
}

libesp::RGBColor Config::getMinHandEndColor() {
  uint32_t v = SettingMap[MIN_END_COLOR]->Value.UIntValue;
  return libesp::RGBColor((v>>16)&0xFF, (v>>8)&0xFF,v&0xFF);
}

libesp::RGBColor Config::getHourStartColor() {
  uint32_t v = SettingMap[HOUR_START_COLOR]->Value.UIntValue;
  return libesp::RGBColor((v>>16)&0xFF, (v>>8)&0xFF,v&0xFF);
}

libesp::RGBColor Config::getHourEndColor() {
  uint32_t v = SettingMap[HOUR_END_COLOR]->Value.UIntValue;
  return libesp::RGBColor((v>>16)&0xFF, (v>>8)&0xFF,v&0xFF);
}

libesp::ErrorType Config::getAllSettings(cJSON *r) {
  ErrorType et;
  SettingMapTypeIT it = SettingMap.begin();
  for(;it!=SettingMap.end();++it) {
    cJSON *sr = cJSON_CreateObject();
    switch((*it).second->ItemType) {
      case ConfigItem::STRING: 
        {
          cJSON_AddStringToObject(sr, "name", (*it).first);
          cJSON_AddStringToObject(sr, "value", &(*it).second->Value.CharValue[0]);
        }
        break;
      case ConfigItem::INT:
        {
          cJSON_AddStringToObject(sr, "name", (*it).first);
          cJSON_AddNumberToObject(sr, "value", (*it).second->Value.IntValue);
        }
        break;
      case ConfigItem::UINT:
      case ConfigItem::COLOR:
      case ConfigItem::PERCENT:
        {
          cJSON_AddStringToObject(sr, "name", (*it).first);
          cJSON_AddNumberToObject(sr, "value", (*it).second->Value.UIntValue);
        }
        break;
    }
    cJSON_AddItemToArray(r,sr);
  }
  return et;
}

libesp::ErrorType Config::setSetting(const char *name, const char *value) {
  ErrorType et;
  ConfigItem *ci = SettingMap[name];
  if(ci) {
    switch(ci->ItemType) {
      case ConfigItem::STRING: 
        {
          et = NVSStorage->setValue(name,value);
        }
        break;
      case ConfigItem::INT:
        {
          int v = atoi(value);
          et = NVSStorage->setValue(name,v);
        }
        break;
      case ConfigItem::UINT:
      case ConfigItem::COLOR:
        {
          uint32_t v = strtoul(value, 0L, 10);
          et = NVSStorage->setValue(name,v);
        }
        break;
      case ConfigItem::PERCENT:
        {
          uint32_t v = strtoul(value, 0L, 10);
          v = v>100?100:v;
          et = NVSStorage->setValue(name,v);
        }
        break;
    }
  } else {
    ESP_LOGE(LOGTAG,"settting not found %s", name);
  }
  return et;
}

