
#ifndef ENV_DISPLAY_H
#define ENV_DISPLAY_H
#include <TFT_eSPI.h>
#include <SPI.h>

#include "driver/rtc_io.h"
void displayInit();
void displayLoop();
void displayOn(bool isOn);
void displayToggle();
bool displayIsOn();
void displaySetOffTimer(uint32_t t);
void displayMsg(const String  m);
void displayMsgln(const String m);
void displayErrorMsg(const String& m);
void displaySensor(float t, float h, float p, int pmA10, int pmA25, int pmA1, int pmB10, int pmB25, int pmB1,String time_str);

#endif
