
#include "env_network.h"

#include <Preferences.h>  // WiFi storage

WiFiClient client;
const  char* rssiSSID;       // NO MORE hard coded set AP, all SmartConfig
const  char* password;
String PrefSSID, PrefPassword;  // used by preferences storage

int32_t rssi;           // store WiFi signal strength here
String getSsid;
String getPass;
bool reconfig_smart_config = false;

// SSID storage
Preferences preferences;  // declare class object
// END SSID storage

// Requires; #include <esp_wifi.h>
// Returns String NONE, ssid or pass arcording to request
// ie String var = getSsidPass( "pass" );
String getSsidPass( String s )
{
  String val = "NONE";  // return "NONE" if wrong key sent
  s.toUpperCase();
  if ( s.compareTo("SSID") == 0 )
  {
    wifi_config_t conf;
    esp_wifi_get_config( WIFI_IF_STA, &conf );
    val = String( reinterpret_cast<const char*>(conf.sta.ssid) );
  }
  if ( s.compareTo("PASS") == 0 )
  {
    wifi_config_t conf;
    esp_wifi_get_config( WIFI_IF_STA, &conf );
    val = String( reinterpret_cast<const char*>(conf.sta.password) );
  }
  return val;
}

// match WiFi IDs in NVS to Pref store,  assumes WiFi.mode(WIFI_AP_STA);  was executed
bool checkPrefsStore(void)
{
  bool val = false;
  String NVssid, NVpass, prefssid, prefpass;

  NVssid = getSsidPass( "ssid" );
  NVpass = getSsidPass( "pass" );

  // Open Preferences with my-app namespace. Namespace name is limited to 15 chars
  preferences.begin("wifi", false);
  prefssid  =  preferences.getString("ssid", "none");     //NVS key ssid
  prefpass  =  preferences.getString("password", "none"); //NVS key password
  preferences.end();

  if ( NVssid.equals(prefssid) && NVpass.equals(prefpass) )
  {
    val = true;
  }

  return val;
}
void setReconfigSmartConfig(){
  reconfig_smart_config = true;
}

// optionally call this function any way you want in your own code
// to remap WiFi to another AP using SmartConfig mode.   Button, condition etc..
void initSmartConfig()
{
  // start LED flasher
  int loopCounter = 0;

  WiFi.mode( WIFI_AP_STA );       //Init WiFi, start SmartConfig
  Serial.printf( "Entering SmartConfig\n" );

  WiFi.beginSmartConfig();

  while (!WiFi.smartConfigDone())
  {
    // flash led to indicate not configured
    Serial.printf( "." );
    if ( loopCounter >= 40 ) // keep from scrolling sideways forever
    {
      loopCounter = 0;
      Serial.printf( "\n" );
    }
    delay(600);
    ++loopCounter;
  }
  loopCounter = 0;

  // stopped flasher here

  Serial.printf("\nSmartConfig received.\n Waiting for WiFi\n\n");
  delay(2000 );

  while ( WiFi.status() != WL_CONNECTED )     // check till connected
  {
    delay(500);
  }
  netIpInfo();  // connected lets see IP info

  preferences.begin("wifi", false);      // put it in storage
  preferences.putString( "ssid"         , getSsid);
  preferences.putString( "password", getPass);
  preferences.end();

  delay(300);
}  // END SmartConfig()

void netWifiInit()  //
{
  WiFi.mode(WIFI_AP_STA);   // required to read NVR before WiFi.begin()

  // load credentials from NVR, a little RTOS code here
  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_STA, &conf);  // load wifi settings to struct comf
  rssiSSID = reinterpret_cast<const char*>(conf.sta.ssid);
  password = reinterpret_cast<const char*>(conf.sta.password);

  //  Serial.printf( "%s\n", rssiSSID );
  //  Serial.printf( "%s\n", password );

  // Open Preferences with "wifi" namespace. Namespace is limited to 15 chars
  preferences.begin("wifi", false);
  PrefSSID          =  preferences.getString("ssid", "none");      //NVS key ssid
  PrefPassword  =  preferences.getString("password", "none");  //NVS key password
  preferences.end();

  // keep from rewriting flash if not needed
  if ( !checkPrefsStore() || reconfig_smart_config)    // see is NV and Prefs are the same
  { // not the same, setup with SmartConfig
    if ( (PrefSSID == "none") ||reconfig_smart_config) // New...setup wifi
    {
      reconfig_smart_config = false;
      initSmartConfig();
      delay( 3000);
      ESP.restart();   // reboot with wifi configured
    }
  }

  // I flash LEDs while connecting here

  WiFi.begin( PrefSSID.c_str() , PrefPassword.c_str() );

  int WLcount = 0;
  while (WiFi.status() != WL_CONNECTED && WLcount < 200 ) // can take > 100 loops depending on router settings
  {
    delay( 100 );
    Serial.printf(".");
    ++WLcount;
  }
  delay( 3000 );

  //  stop the led flasher here

}  // END netWifiInit()

// Return RSSI or 0 if target SSID not found
// const char* SSID = "YOUR_SSID";  // declare in GLOBAL space
// call:  int32_t rssi = getRSSI( SSID );
int32_t getRSSI( const char* target_ssid )
{
  byte available_networks = WiFi.scanNetworks();

  for (int network = 0; network < available_networks; network++)
  {
    if ( strcmp(  WiFi.SSID( network).c_str(), target_ssid ) == 0)
    {
      return WiFi.RSSI( network );
    }
  }
  return 0;
} //  END  getRSSI()

void netIpInfo()
{
  String  MAC;
  getSsid = WiFi.SSID();
  getPass = WiFi.psk();
  rssi = getRSSI(  rssiSSID );
  netWifiStatus();
  MAC = netGetMacAddr();

  Serial.printf( "\n\n\tSSID\t%s, ", getSsid.c_str() );
  Serial.print( rssi);  Serial.printf(" dBm\n" );  // printf??
  Serial.printf( "\tPass:\t %s\n", getPass.c_str() );
  Serial.print( "\n\n\tIP address:\t" );  Serial.print(WiFi.localIP() );
  Serial.print( " / " );
  Serial.println( WiFi.subnetMask() );
  Serial.print( "\tGateway IP:\t" );  Serial.println( WiFi.gatewayIP() );
  Serial.print( "\t1st DNS:\t" );     Serial.println( WiFi.dnsIP() );
  Serial.printf( "\tMAC:\t\t%s\n", MAC.c_str() );
}

int netWifiStatus()
{
  int WiFiStatus = 0;
  WiFiStatus = WiFi.status();
  Serial.printf("\tStatus %d",  WiFiStatus );
  switch ( WiFiStatus )
  {
    case WL_IDLE_STATUS :                         // WL_IDLE_STATUS     = 0,
      Serial.printf(", WiFi IDLE \n");
      break;
    case WL_NO_SSID_AVAIL:                        // WL_NO_SSID_AVAIL   = 1,
      Serial.printf(", NO SSID AVAIL \n");
      break;
    case WL_SCAN_COMPLETED:                       // WL_SCAN_COMPLETED  = 2,
      Serial.printf(", WiFi SCAN_COMPLETED \n");
      break;
    case WL_CONNECTED:                            // WL_CONNECTED       = 3,
      Serial.printf(", WiFi CONNECTED \n");
      break;
    case WL_CONNECT_FAILED:                       // WL_CONNECT_FAILED  = 4,
      Serial.printf(", WiFi WL_CONNECT FAILED\n");
      break;
    case WL_CONNECTION_LOST:                      // WL_CONNECTION_LOST = 5,
      Serial.printf(", WiFi CONNECTION LOST\n");
      WiFi.persistent(false);                 // don't write FLASH
      break;
    case WL_DISCONNECTED:                         // WL_DISCONNECTED    = 6
      Serial.printf(", WiFi DISCONNECTED ==\n");
      WiFi.persistent(false);                 // don't write FLASH when reconnecting
      break;
  }
  return WiFiStatus;
}
// END getWifiStatus()

// Get the station interface MAC address.
// @return String MAC
String netGetMacAddr(void)
{
  WiFi.mode(WIFI_AP_STA);                    // required to read NVR before WiFi.begin()
  uint8_t baseMac[6];
  esp_read_mac( baseMac, ESP_MAC_WIFI_STA ); // Get MAC address for WiFi station
  char macStr[18] = { 0 };
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  return String(macStr);
}
// END getMacAddress()

void netWifiBegin()
{
  WiFi.begin( PrefSSID.c_str() , PrefPassword.c_str() );
}
bool netIsWifiConnect()
{
  return WiFi.status() == WL_CONNECTED;
}

void netGetTime() 
{
  //25200 = gmt+7
  configTime(25200, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(&nowSecs);

  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(&nowSecs);
  }
  //setenv("TZ", "EST+7", 1);
  //tzset(); // save the TZ variable
  Serial.println();
  struct tm timeinfo;
  localtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}
// Send message to Pama System send two time
void netPublishPaMa(String client_name, float temp, float hum, float pre,uint16_t pmA10, uint16_t pmA25, uint16_t pmA1,uint16_t pmB10, uint16_t pmB25, uint16_t pmB1 )
{
  //https://pamasmell-service-cumzzqlena-as.a.run.app/api/iotreport?name=1234&pm25=50&pm10=50&temperature=29&humidity=50
  const char* server = "pamasmell-service-cumzzqlena-as.a.run.app";
  //sensor 1  
  if (client.connect(server, 80)){
    Serial.println("PaMa  connected");
    // We now create a URI for the request
    String url = "/api/iotreport";
    url += "?name=";
    url += client_name;
    url += "&pm25=";
    url += String(pmA25);
    url += "&pm10=";
    url += String(pmA10);
    url += "&temperature=";
    url += String(temp);
    url += "&humidity=";
    url += String(hum);
    Serial.print("Requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 20000) {
        Serial.println(">>> PaMa respond timeout !");
        client.stop();
        return;
      }
    }
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
    
  }else {
    Serial.println("PaMa connection fail");
  }
  Serial.println("");
  client.stop();
  
  //sensor 2
  if (client.connect(server, 80)){
    Serial.println("PaMa  connected");
    // We now create a URI for the request
    String url = "/api/iotreport";
    url += "?name=";
    url += client_name;
    url += "&pm25=";
    url += String(pmB25);
    url += "&pm10=";
    url += String(pmB10);
    Serial.print("Requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 20000) {
        Serial.println(">>> PaMa respond timeout !");
        client.stop();
        return;
      }
    }
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
    
  }else {
    Serial.println("PaMa connection fail");
  }
  Serial.println("");
  client.stop();

}
void netPublishPaMaOld(String client_name, uint16_t pm25 )
{
  //https://pamasmell.herokuapp.com/api/iotreport?name=test-00&aqi=37
  const char* server = "pamasmell.herokuapp.com";
  if (client.connect(server, 80))
  {
    Serial.println("PaMa  connected");
    // We now create a URI for the request
    String url = "/api/iotreport";
    url += "?name=";
    url += client_name;
    url += "&aqi=";
    url += pm25;

    Serial.print("Requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 20000) {
        Serial.println(">>> PaMa respond timeout !");
        client.stop();
        return;
      }
    }
    while (client.available()) {
      String line = client.readStringUntil('\r');
      //Serial.print(line);
    }
  } else {
    Serial.println("PaMa connection fail");
  }
  Serial.println("");
  client.stop();
}

void netPublishThinkSpeak(String ThingSpeakapiKey, float temp, float hum, float pre,uint16_t pmA10, uint16_t pmA25, uint16_t pmA1,uint16_t pmB10, uint16_t pmB25, uint16_t pmB1 )
{
  const char* server = "api.thingspeak.com";

  if (client.connect(server, 80))
  {
    Serial.println("Thingspeak  connected");
    String postStr = ThingSpeakapiKey;
    postStr += "&field1=";postStr += String(temp);
    postStr += "&field2=";postStr += String(hum);
    postStr += "&field3=";postStr += String(pre);
    postStr += "&field4=";postStr += String(pmA10);
    postStr += "&field5=";postStr += String(pmA25);
    postStr += "&field6=";postStr += String(pmB1);
    postStr += "&field7=";postStr += String(pmB10);
    postStr += "&field8=";postStr += String(pmB25);

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + ThingSpeakapiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    Serial.println("%. Send to Thingspeak.");
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 20000) {
        Serial.println(">>> Thing Speak respond timeout !");
        client.stop();
        return;
      }
    }
    while (client.available()) {
      String line = client.readStringUntil('\r');
      //Serial.print(line);
    }
  } else {
    Serial.println("Thingspeak  connect fail");
  }
  Serial.println("");
  client.stop();

}
