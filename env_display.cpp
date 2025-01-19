#include "env_display.h"
static uint32_t ttf_off_time  = 60000; //1 minute 1*60 sec
static uint32_t next_display_sleep = 0;
static bool is_tft_on = true;

TFT_eSPI tft = TFT_eSPI();

void displayInit()
{
  displayOn(true);
  tft.init();             // -- Display
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_BLACK);
  tft.fillRect(0, 0, 240, 84, TFT_WHITE);
  displayMsg("Initialize ...");
  pinMode(4, OUTPUT); //tft
}

void displayMsg(const String m)
{
  Serial.println("> " + String(m));
  tft.setTextColor(TFT_BLACK);
  tft.fillRect(0, 0, 240, 84, TFT_WHITE);
  tft.setCursor(0, 20, 2);
  tft.println(String(m));
}
void displayMsgln(const String m)
{
  Serial.println("> " + String(m));
  tft.println(m);
}

void displayErrorMsg(String m)
{
  Serial.println(m);
}

void displaySensor(float t, float h, float p, int pmA10, int pmA25, int pmA1, int pmB10, int pmB25, int pmB1, String time_str)
{
  uint32_t bg = TFT_WHITE;
  if (pmA25 <0 ) bg = TFT_WHITE;
  if (pmA25 <= 25) bg = TFT_BLUE;
  if (pmA25 >= 26 && pmA25 <= 37) bg = TFT_GREEN;
  if (pmA25 >= 38 && pmA25 <= 49) bg = TFT_YELLOW;
  if (pmA25 >= 50 && pmA25 <= 90) bg = 0xFBE0; //Dark orange
  if (pmA25 >= 91) bg = TFT_RED;
  //Display big PM2.5
  tft.fillRect(0, 0, 240, 84, bg);
  tft.setTextColor(TFT_BLACK);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(String(pmA25), 159, 2, 8);

  //read sensor  time
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(160, 7, 4);
  tft.println(" PM2.5");
  String line; String sensor2;
  tft.setCursor(190, 27, 2); tft.println(time_str);

  if (pmB10 > 0) {
    sensor2 = "/" + String(pmB10);
  } else {
    sensor2 = "   ";
  }
  line = "10 =" + String(pmA10) + sensor2 ;
  tft.setCursor(160, 40, 2); tft.println(line);
  if (pmB25 > 0) {
    sensor2 = "/" + String(pmB25);
  } else {
    sensor2 = "   ";
  }
  line = "2.5=" + String(pmA25) + sensor2;
  tft.setCursor(160, 40 + 13, 2); tft.println(line);
  if (pmB1 > 0) {
    sensor2 = "/" + String(pmB1);
  } else {
    sensor2 = "   ";
  }
  line = "1  =" + String(pmA1) + sensor2;
  tft.setCursor(160, 40 + 13 + 13, 2); tft.println(line);

  Serial.print("T="); Serial.print(t); Serial.print(" H="); Serial.println(h);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString(String(t, 2) + String("c"), 235, 88, 2);
  //hum
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.drawString(String(h, 2) + String("%"), 235, 103, 2);
  //pressure
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(p, 1) + String("hpa"), 235, 118, 2);

}

void displaySetOffTimer(uint32_t t)
{
  ttf_off_time = t;
}

bool displayIsOn() {
  return is_tft_on;
}

void displayOn(bool isOn)
{
  is_tft_on = isOn;

  if (isOn) {
    next_display_sleep = millis() + ttf_off_time;
    Serial.println("Turn display On");
    digitalWrite(4, HIGH); //TFT_BL=4
  } else {
    digitalWrite(4, LOW); //TFT_BL=4
    Serial.println("Turn display Off");
  }
}

void displayToggle()
{
  displayOn(!is_tft_on);
}

void displayTime()
{
  struct tm timeinfo;
  time_t nowSecs = time(&nowSecs);
  localtime_r(&nowSecs, &timeinfo);
  //Serial.print(asctime(&timeinfo));
  uint8_t hh = timeinfo.tm_hour;
  uint8_t mm = timeinfo.tm_min;

  tft.setCursor(1, 87, 7);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  //if (!rtc_ok){tft.print("00:00");return;}
  if (hh < 10) tft.print(0);
  tft.print(hh);
  tft.print(":");
  if (mm < 10) tft.print(0);
  tft.print(mm);
}

void displayLoop()
{
  static uint32_t next_clock_update = 0;
  if (is_tft_on)
  {
    uint32_t timerNow = millis();
    if (next_display_sleep < timerNow)
    {
      //Serial.println(String(next_display_sleep)+":"+timerNow+":"+ttf_off_time);
      displayOn(false);
      return;
    }
    //displayTimer
    if (next_clock_update < timerNow ) {
      next_clock_update = timerNow + 60000; //update every 1 min
      displayTime();
    }
  }
}
