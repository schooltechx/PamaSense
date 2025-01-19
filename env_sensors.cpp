#include "env_sensors.h"
#include "HardwareSerial.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


#define RX_A 17 // ต่อ TX ของเซนเซอร์
#define TX_A 13 // ต่อ RX ของเซนเซอร์

#define RX_B 26 // ต่อ TX ของเซนเซอร์
#define TX_B 27 // ต่อ RX ของเซนเซอร์
#define WAKE_PIN_A 15 //WAKE AND SLEEP PIN
/*

#define RESET_PIN_A 33 //RESET PIN
#define RX_A 25 // ต่อ TX ของเซนเซอร์
#define TX_A 26 // ต่อ RX ของเซนเซอร์
#define WAKE_PIN_A 27 //WAKE AND SLEEP PIN

#define RESET_PIN_B 33 //RESET PIN
#define RX_B 32 // ต่อ TX ของเซนเซอร์
#define TX_B 39 // ต่อ RX ของเซนเซอร์
#define WAKE_PIN_B 27 //WAKE AND SLEEP PIN
*/


PMS pmsA(Serial1); // ต่อเซนเซอร์ที่ Serial2
PMS pmsB(Serial2); // ต่อเซนเซอร์ที่ Serial3

bool isPmsReady = false;
Adafruit_BME280 bme;
static bool bme_ok = false;
static bool is_sensors_wakeup = false;

void sensorBegin()
{
  Serial1.begin(9600, SERIAL_8N1, RX_A, TX_A);
  Serial2.begin(9600, SERIAL_8N1, RX_B, TX_B);

  pinMode(WAKE_PIN_A, OUTPUT);
  pmsA.passiveMode();    // ใช้โหมด passive mode ต้อง requestRead
  pmsB.passiveMode();    // ใช้โหมด passive mode ต้อง requestRead

  bme_ok = bme.begin();
  if (!bme_ok) {
    Serial.println("Invalid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(), 16);
  }
}

bool sensorsIsWakeUp() {
  return is_sensors_wakeup;
}
//wakeup all sensor
void sensorsWakeUp(bool wake)
{
  //Serial.println("sensorsWakeUp "+String(wake));
  is_sensors_wakeup = wake;
  if (wake) {
    while (Serial1.available()) {
      Serial1.read();
    }
    while (Serial2.available()) {
      Serial2.read();
    }
    pmsA.wakeUp();
    pmsB.wakeUp();

    Serial.println("PMS wake up ");
    digitalWrite(WAKE_PIN_A, HIGH);
  } else {
    pmsA.sleep();
    pmsB.sleep();
    Serial.println("PMS sleep ");
    digitalWrite(WAKE_PIN_A, LOW);
  }
}
bool sensorsGetBmeData(float &t, float &h, float &p )
{
  if (bme_ok) {
    t = bme.readTemperature();
    h = bme.readHumidity();
    p = bme.readPressure() / 100.0F ;
  } else {
    t = 0; h = 0; p = 0;
  }
  return bme_ok;

}
bool sensorsGetPMSDataA(PMS::DATA &pmsData)
{
  pmsA.requestRead();
  return pmsA.readUntil(pmsData);
}
bool sensorsGetPMSDataB(PMS::DATA &pmsData)
{
  pmsB.requestRead();
  return pmsB.readUntil(pmsData);
}

bool sensorBMEok() {
  return bme_ok;
}
/*
  void handleSensor()
  {
  pms.wakeUp(); //ปลุกเซนเซอร์ขึ้นมาทำงาน

  if (pms.readUntil(data))
  {
    Serial.print("PM 1.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_1_0);

    Serial.print("PM 2.5 (ug/m3): ");
    Serial.println(data.PM_AE_UG_2_5);

    Serial.print("PM 10.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_10_0);
  }
  else
  {
    Serial.println("No data.");
  }
  pms.sleep(); //พักการทำงานของเซนเซอร์
  }
*/
