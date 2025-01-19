#ifndef ENV_SENSORS_H
#define ENV_SENSORS_H
#include "PMS.h"

void sensorBegin();
bool sensorsIsWakeUp();
void sensorsWakeUp(bool wake);
bool sensorsGetPMSDataA(PMS::DATA &pmsData);
bool sensorsGetPMSDataB(PMS::DATA &pmsData);
bool sensorBMEok();
bool sensorsGetBmeData(float &t, float &h, float &p );

#endif
