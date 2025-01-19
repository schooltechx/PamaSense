
#include "env_network.h"
#include "env_sensors.h"
#include "env_display.h"
#include "Button2.h"
#include "driver/rtc_io.h"
// #define BUILD_STRING "smart config 2020-11-10"
#define BUILD_DATE (char const[]) { __DATE__[0], __DATE__[1], __DATE__[2], __DATE__[3], (__DATE__[4] == ' ' ?  '0' : __DATE__[4]), __DATE__[5], __DATE__[6], __DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10], __DATE__[11] }
String build_ver = "PAMA Sense v1.0 " + String(BUILD_DATE);

int UpCount = 0;
int  WFstatus;
bool isOffLine = false;
//String ThingSpeakapiKey = "TVLIL2MN4QTK2ZZU";
String ThingSpeakapiKey = "E7NVDPVZO19KE0RC";
Button2 buttonA = Button2(35);
Button2 buttonB = Button2(0);

void setup() {
  String  MAC;
  Serial.begin(115200);
  displayInit();
  sensorBegin();
  buttonA.setTapHandler(BtHandler);
  displaySetOffTimer(2 * 60 * 1000); //2 min 2*60*1000
  delay(1000);
  displayMsg("  Offline: Hold button 3 sec");
  displayMsgln("  Reconfig network: Hole button");
  delay(3000);
  if ( digitalRead(35) == LOW ) // Offline or reconfig network
  {
    delay(3000);
    if ( digitalRead(35) == HIGH ) // Offline
    {
      isOffLine = true;
      displayMsgln("  Offline Mode ..");
      displaySetOffTimer(5 * 60 * 1000); //5 min 5*60*1000
      return;
    }
    displayMsg("  Reconfiging Network ... ");
    setReconfigSmartConfig();
    displayMsgln("  Smartconfig done. Restarting ...  ");
    delay(2000);
  }
  displayMsg("  " + build_ver);
  displayMsgln("  Connecting Wifi ...");

  netWifiInit();       // get WiFi connected
  netIpInfo();
  MAC = netGetMacAddr();
  netGetTime();
  IPAddress ip =  WiFi.localIP();
  String line1 = "  IP address: " + String(ip[0]) + String(ip[1]) + String(".") + String(ip[2]) + String(".") + String(ip[3]);
  displayMsgln(line1);
  displayMsgln("  SSID: " + WiFi.SSID() );
  delay(5000);
} //  END setup()

//Note : Don't delay inside loop if not necessary. use mili() to check time
void loop()
{
  displayLoop();
  buttonA.loop();
  buttonB.loop();
  if (isOffLine) {
    schedulerLoop();//Do read sensor and update display
    return; //not do network staff
  }
  if (netIsWifiConnect() )      // Main connected loop
  {
    // ANY MAIN LOOP CODE HERE
    schedulerLoop();//Do read sensor, display and publish task
  } // END Main connected loop()
  else
  { // WiFi DOWN
    //  wifi down start LED flasher here
    WFstatus = netWifiStatus();
    netWifiBegin();
    int WLcount = 0;
    while (!netIsWifiConnect() && WLcount < 200 )
    {
      delay( 100 );
      Serial.printf(".");
      if (UpCount >= 60)  // keep from scrolling sideways forever
      {
        UpCount = 0;
        Serial.printf("\n");
      }
      ++UpCount;
      ++WLcount;
    }
    WFstatus = netWifiStatus();
    if ( WFstatus == 3 )  //wifi returns
    {
      // stop LED flasher, wifi going up
    }
    delay( 1000 );
  }  // END WiFi down
} // END loop()

//https://rntlab.com/question/outputs-while-deepsleep/
void assignKeepStateDeepSleep(gpio_num_t  pin_MOSFET )
{
  rtc_gpio_init(pin_MOSFET);
  rtc_gpio_set_direction(pin_MOSFET, RTC_GPIO_MODE_OUTPUT_ONLY);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
}

void BtHandler(Button2& btn) {
  displayOn(true);
}

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

//Do read sensor and publish task
void schedulerLoop()
{
  static uint32_t next_read_sensors = 0;
  static uint32_t next_warm_sensors = 0;
  static bool sensor_ready = false;

  uint32_t timerNow = millis();
  if (next_read_sensors < timerNow) // always publsh when restart
  {
    if (next_warm_sensors == 0) { //start warmup
      next_warm_sensors = timerNow + 30000; //update every 30sec
      sensorsWakeUp(true);
    }
    if (next_warm_sensors < timerNow) { //sensor ready
      PMS::DATA pmsDataA;
      PMS::DATA pmsDataB;
      struct tm timeinfo;
      time_t nowSecs = time(&nowSecs);
      localtime_r(&nowSecs, &timeinfo);
      if (next_read_sensors == 0) { //first read sensor
        //find good time to read sensor. every 30min
        int sec_diff ;
        if (timeinfo.tm_min < 30)
        {
          sec_diff = (30 * 60) - (timeinfo.tm_min * 60) + timeinfo.tm_sec;
        } else
        {
          sec_diff = (60 * 60) - (timeinfo.tm_min * 60) + timeinfo.tm_sec;
        }
        if (sec_diff < 60) {
          sec_diff += (30 * 60); //advance 30min
        }
        Serial.println("next Read sensor in " + String(sec_diff) + " Second");
        next_read_sensors = timerNow + (sec_diff * 1000);
      } else {
        next_read_sensors = timerNow + (30 * 60 * 1000); //30 * 60*1000 publish every half hour
      }
      next_warm_sensors = 0;
      int pmA25 =-1; int pmA10 = -1; int pmA1 = -1;
      int pmB25 = -1; int pmB10 = -1; int pmB1 = -1;

      if (sensorsGetPMSDataA(pmsDataA)) {
        Serial.println("-- PMS7003 A --");
        Serial.print("PM 1.0 (ug/m3): "); Serial.println(pmsDataA.PM_AE_UG_1_0);
        Serial.print("PM 2.5 (ug/m3): "); Serial.println(pmsDataA.PM_AE_UG_2_5);
        Serial.print("PM 10.0 (ug/m3): "); Serial.println(pmsDataA.PM_AE_UG_10_0);
        pmA25 = pmsDataA.PM_AE_UG_2_5 ;
        pmA10 = pmsDataA.PM_AE_UG_10_0;
        pmA1 = pmsDataA.PM_AE_UG_1_0;

      } else {
        Serial.println("PMS A Nodata");
      }
      if (sensorsGetPMSDataB(pmsDataB)) {
        Serial.println("-- PMS7003 B --");
        Serial.print("PM 1.0 (ug/m3): "); Serial.println(pmsDataB.PM_AE_UG_1_0);
        Serial.print("PM 2.5 (ug/m3): "); Serial.println(pmsDataB.PM_AE_UG_2_5);
        Serial.print("PM 10.0 (ug/m3): "); Serial.println(pmsDataB.PM_AE_UG_10_0);
        pmB25 = pmsDataB.PM_AE_UG_2_5 ;
        pmB10 = pmsDataB.PM_AE_UG_10_0;
        pmB1 = pmsDataB.PM_AE_UG_1_0;

      } else {
        Serial.println("PMS B Nodata");
      }
      float temp_data = 0; float humi_data = 0; float pres_data = 0;

      sensorsGetBmeData(temp_data, humi_data, pres_data);
      displayOn(true); //ready to update sensor display
      uint8_t hh = timeinfo.tm_hour;
      uint8_t mm = timeinfo.tm_min;
      String time_str = "";
      if (hh < 10)time_str += "0";
      time_str += String(hh);
      time_str += ":";
      if (mm < 10)time_str += "0";
      time_str += String(mm);
      displaySensor(temp_data, humi_data, pres_data,
                    pmA10, pmA25, pmA1,
                    pmB10, pmB25, pmB1,
                    time_str);
      //note: Online station will reboot every day.
      if (!isOffLine && (pmA25>-1) && (pmB25>-1) ) {
        if (timerNow > (24 * 60 * 60 * 1000)) { //reboot every 24 hour
          Serial.println("Daily reboot");
          Serial.flush();
          delay(200);
          ESP.restart();
        }

        netPublishThinkSpeak( ThingSpeakapiKey, temp_data, humi_data, pres_data, pmA10, pmA25 , pmA1, pmB10, pmB25, pmB1 );
        netPublishPaMa(       netGetMacAddr() , temp_data, humi_data, pres_data, pmA10, pmA25 , pmA1, pmB10, pmB25, pmB1 );
        //netPublishPaMa(netGetMacAddr() + "-A", pmA25 );
        //netPublishPaMa(netGetMacAddr() + "-B", pmB25 );
      }
      Serial.print(asctime(&timeinfo));
      sensorsWakeUp(false);
      return;
    }
    //Do nothing wait schedule
  }

}
