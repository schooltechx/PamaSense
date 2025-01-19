#ifndef ENV_NETWORK_H
#define ENV_NETWORK_H
#include <WiFi.h>
#include "FS.h"
#include "esp_system.h"
#include <esp_wifi.h>
#include "esp_mac.h"
#include <string.h>
#include <WebServer.h>

void netWifiInit();
void netIpInfo();
int netWifiStatus();
String netGetMacAddr(void);
void netWifiBegin();
bool netIsWifiConnect();
void netPublishThinkSpeak(String ThingSpeakapiKey, float temp, float hum, float pre,uint16_t pmA10, uint16_t pmA25, uint16_t pmA1,uint16_t pmB10, uint16_t pmB25, uint16_t pmB1 );
void netPublishPaMa(String client_name, float temp, float hum, float pre,uint16_t pmA10, uint16_t pmA25, uint16_t pmA1,uint16_t pmB10, uint16_t pmB25, uint16_t pmB1 );
void netPublishPaMaOld(String client_name,uint16_t pm25 );
void netGetTime();
void initSmartConfig();
void setReconfigSmartConfig();
#endif
