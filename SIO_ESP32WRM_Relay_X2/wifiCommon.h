bool InAPMode = false;
bool APModeSuccess = false;
bool needReset = false;
bool doWiFiScan = false;
bool WifiInitialized = false;

TimeRelease ReconnectWiFi;
TimeRelease resetTimeDelay;


uint32_t getChipId(){
  uint32_t chipId = 0;
  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  
  return chipId;
}

WiFiUDP ntpUDP;
// By default 'pool.ntp.org' is used with 60 seconds update interval and no offset
//int TimeZoneOffsetHours = -5; Now in the settingsconfig struct//Eastern(US NY) standard time [This value will default to -5 but can be over ridden in the settings.d file
NTPClient timeClient(ntpUDP,"pool.ntp.org",(-5 * 3600/-1),120000);// -4 hours update every 2 minutes gets updated in setup.

struct WifiConfig
{
  char ssid[64];
  char password[64];
  char wifimode[12];
  char hostname[64];
  int attempts;
  int attemptdelay;
  char apPassword[64];
};

struct SettingsConfig
{
  char OPURI[128];
  int OPPort;
  char OPAK[34];
  int StatusIntervalSec;
  int TimeZoneOffsetHours;
};


SettingsConfig settingsConfig;
WifiConfig wifiConfig;
char* www_username = "admin";
WebServer webServer(80);
int lastWifiStatus = -1;


bool isWiFiConnected(bool debug=true){

  if(lastWifiStatus !=WiFi.status()){ // should only print out the status when it changes.
    lastWifiStatus =WiFi.status();
    #ifdef _GDebug
      debugMsgPrefx();Serial.print("WiFi Status: ");
      Serial.println(WiFi.status());
    #endif
  }
  return (WiFi.status() == WL_CONNECTED);
}


bool startWifiStation(){
  #ifdef _GDebug
  debugMsgPrefx();Serial.printf("[INFO]: Connecting to %s\n", wifiConfig.ssid);
  debugMsgPrefx();Serial.printf("Existing set WiFi.SSID:%s\n",WiFi.SSID().c_str());
  debugMsgPrefx();Serial.printf("wifiConfig.ssid:%s\n",wifiConfig.ssid);
  #endif
  if(String(wifiConfig.wifimode) == "WIFI_STA"){
    if ( String(WiFi.SSID()) != String(wifiConfig.ssid) || WifiInitialized == false)
    {
        #ifdef _GDebug
        debugMsg("initiallizing WiFi Connection");
        #endif
        if(isWiFiConnected()){
          WiFi.disconnect();
        }
        
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiConfig.ssid, wifiConfig.password);
        WifiInitialized = true;
        int attempts = wifiConfig.attempts;
        debugMsgPrefx();Serial.print("connecting ."); //dots per try "..."
        while (!isWiFiConnected())
        {
          if(attempts == 0) {
            WiFi.disconnect();
            Serial.println("");
            return false;
            
          }
          
          delay(wifiConfig.attemptdelay);
          Serial.print(".");
          attempts--;
       }
       Serial.println(""); //closes off the connection "... output"
       #ifdef _GDebugbug
       debugMsgPrefx();
       Serial.print("IP: ");
       Serial.println(WiFi.localIP());  
       #endif
    }else{
      if(WiFi.status() != WL_CONNECTED){
        WiFi.reconnect();
      }
    }
    WiFi.hostname(wifiConfig.hostname);
  }else{
    
    return false; //In AP mode.. should not be connecting.
  }
  #ifdef _GDebug
  debugMsgPrefx();
  Serial.print("WiFi.Status(): ");
  Serial.println(String(WiFi.status()));
  debugMsgPrefx();
  Serial.print("WL_CONNECTED: ");
  Serial.println(String(WL_CONNECTED));
  #endif
  //needMqttConnect = (WiFi.status() == WL_CONNECTED); //get the MQtt System to connect once wifi is connected.
  return (WiFi.status() == WL_CONNECTED);
}



void DoInAPMode(){
  if(InAPMode && !APModeSuccess){
    //Going into AP mode
    
    debugMsg("Entering AP mode.");
    
    
    String tempName = wifiConfig.hostname + String(getChipId()); 
    strlcpy(wifiConfig.hostname,tempName.c_str(),sizeof(wifiConfig.hostname));
    
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    delay(100);
    APModeSuccess = WiFi.softAP(wifiConfig.hostname, wifiConfig.apPassword);
    
    if (APModeSuccess){
      debugMsgPrefx();Serial.print("SoftAP IP Address: ");Serial.println(WiFi.softAPIP());
      ReconnectWiFi.set(75000); //75 seconds unless someone connects.
    }else{
      APModeSuccess = true; // not really but if we do not do this, It will come though here and reset the reset delay every loop. 
      
      debugMsg("SoftAP mode Failed Rebooting in 15 seconds.Try using Serial connection to update WiFi settings.");
      
      resetTimeDelay.set(15000UL); //trigger 15 sec
      needReset = true;
    }
  }
}

void CheckWifi(){
   //check connection status
   
  if(InAPMode && !APModeSuccess){
      DoInAPMode();
  }else{
    //only try to reconnect if no one has connected as an AP client.
    if(ReconnectWiFi.check() && WiFi.softAPgetStationNum() ==0){
        InAPMode = !startWifiStation(); //attempt to reconnect to the main wifi access point if it succeds, Set to false.
        APModeSuccess = false; //reset so it will do a full attempt to go into AP mode if called to
    }
  }
  
  webServer.handleClient(); 
  
  if(resetTimeDelay.check()){
    if(needReset){
      delay(10);
      ESP.restart();
    }
  }
}


String IpAddress2String(const IPAddress& ipAddress)
{
    return String(ipAddress[0]) + String(".") +
           String(ipAddress[1]) + String(".") +
           String(ipAddress[2]) + String(".") +
           String(ipAddress[3]);
}

void DebugwifiConfig(){
  debugMsgPrefx();Serial.print("wifiConfig.ssid: ");
  Serial.println(wifiConfig.ssid);
  debugMsgPrefx();Serial.print("wifiConfig.password: ");
  Serial.println(wifiConfig.password);
  debugMsgPrefx();Serial.print("wifiConfig.wifimode: ");
  Serial.println(wifiConfig.wifimode);
  debugMsgPrefx();Serial.print("wifiConfig.hostname: ");
  Serial.println(wifiConfig.hostname);
  debugMsgPrefx();Serial.print("wifiConfig.attempts: ");
  Serial.println(wifiConfig.attempts);
  debugMsgPrefx();Serial.print("wifiConfig.attemptdelay: ");
  Serial.println(wifiConfig.attemptdelay);
  debugMsgPrefx();Serial.print("wifiConfig.apPassword: ");
  Serial.println(wifiConfig.apPassword);
}

String wifiConfigFile = "/config/wifiConfig.json";
bool loadwifiConfig()
{
  #ifdef _GDebug
    debugMsgPrefx();Serial.print("wifiConfig file path: ");Serial.println(wifiConfigFile);
  #endif
  if (!SPIFFS.exists(wifiConfigFile))
  {
    #ifdef _GDebug
    debugMsg("[WARNING]: wifiConfig file not found! Loading Factory Defaults.");
    #endif
    strlcpy(wifiConfig.ssid,"", sizeof(wifiConfig.ssid));
    strlcpy(wifiConfig.password, "", sizeof(wifiConfig.password));
    
    strlcpy(wifiConfig.wifimode, "WIFI_AP", sizeof(wifiConfig.wifimode));
    strlcpy(wifiConfig.hostname, DEFAULT_HOSTS_NAME, sizeof(wifiConfig.hostname));
    strlcpy(wifiConfig.apPassword, "Password1", sizeof(wifiConfig.apPassword));
    wifiConfig.attempts = 10;
    wifiConfig.attemptdelay = 5000; //5 second delay
    return false;
  }
  
  File configfile = SPIFFS.open(wifiConfigFile,"r");

  DynamicJsonDocument doc(1024);

  DeserializationError error = deserializeJson(doc, configfile);

  strlcpy(wifiConfig.ssid, doc["ssid"] | "FAILED", sizeof(wifiConfig.ssid));
  strlcpy(wifiConfig.password, doc["password"] | "FAILED", sizeof(wifiConfig.password));
  strlcpy(wifiConfig.wifimode, doc["wifimode"] | "WIFI_STA", sizeof(wifiConfig.wifimode));
  strlcpy(wifiConfig.hostname, doc["hostname"] | DEFAULT_HOSTS_NAME, sizeof(wifiConfig.hostname));
  strlcpy(wifiConfig.apPassword, doc["apPassword"] | "Password1", sizeof(wifiConfig.apPassword));

  int attempts = doc["attempts"] | 10 ;
  wifiConfig.attempts = attempts;

  int attemptdelay = doc["attemptdelay"] | 5000 ;
  wifiConfig.attemptdelay = attemptdelay;

  configfile.close();

  if (error)
  {
    debugMsg("[ERROR]: deserializeJson() error in loadwifiConfig");
    debugMsg(error.c_str());
    return false;
  }else{
    #ifdef _GDebug
    debugMsg("wifiConfig was loaded successfully");
    DebugwifiConfig();
    #endif
  }
  

  return true;
}


bool savewifiConfig(WifiConfig newConfig)
{
  #ifdef _GDebug
    debugMsgPrefx();Serial.print("wifiConfig file path: ");Serial.println(wifiConfigFile);
  #endif
  SPIFFS.remove(wifiConfigFile);
  File file = SPIFFS.open(wifiConfigFile, "w");

  DynamicJsonDocument doc(1024);

  JsonObject wifiConfigobject = doc.to<JsonObject>();
  
  wifiConfigobject["ssid"] = newConfig.ssid;
  wifiConfigobject["password"] = newConfig.password;
  wifiConfigobject["wifimode"] = newConfig.wifimode;
  wifiConfigobject["hostname"] = newConfig.hostname;
  wifiConfigobject["attempts"] = newConfig.attempts;
  wifiConfigobject["attemptdelay"] = newConfig.attemptdelay;
  wifiConfigobject["apPassword"] = newConfig.apPassword;
  int chrW = serializeJsonPretty(doc, file);
  if (chrW == 0)
  {
    debugMsg("[WARNING]: Failed to write to webconfig file");
    return false;
  }else {
    wifiConfig = newConfig;
    #ifdef _GDebug
    debugMsgPrefx();Serial.print("Characters witten: ");Serial.println(chrW);
    debugMsg("wifiConfig was updated");
    DebugwifiConfig();
    #endif
  }
    
  file.close();
  return true;
}


void SetupWifi(){
  if(loadwifiConfig()){
    #ifdef _GDebug
    debugMsg("WiFi Config Loaded");
    #endif
    debugMsgPrefx();Serial.print("WifiMode:");
    Serial.println(wifiConfig.wifimode);
    if(wifiConfig.wifimode == "WIFI_AP"){
      InAPMode  = true;
      
    }else{
      InAPMode  = false;
    }
      
  }else{
    #ifdef _GDebug
    debugMsg("WiFi Config Failed to Load");
    #endif
    InAPMode  = true;
  }
  if(InAPMode){
    debugMsg("Doing InAPMode");
    DoInAPMode();
  }else{
    debugMsg("Doing startWifiStation");
    startWifiStation();
  }
  #ifdef _GDebug
  debugMsg("Wifi Setup complete");
  #endif
}

void PrintNetStat(){
  debugMsgPrefx();
  Serial.print("Is Wifi Connected: ");
  if (isWiFiConnected(false)){
    Serial.print("yes");
    debugMsgPrefx();Serial.print("IP:");
    Serial.println(WiFi.localIP());
    
    debugMsgPrefx();Serial.print("SubNet:");
    Serial.println(WiFi.subnetMask());

    debugMsgPrefx();Serial.print("Gateway:");
    Serial.println(WiFi.gatewayIP());
    //would like to add other info here. Things like gateway and dns server and Maybe MQTT info
  }else{
    Serial.println("No");
  }
}

void DebugSettingsConfig(){
  
  debugMsgPrefx();Serial.print("settingsConfig.OPURI: ");
  Serial.println(settingsConfig.OPURI);
  
  debugMsgPrefx();Serial.print("settingsConfig.OPPort: ");
  Serial.println(settingsConfig.OPPort);
  
  debugMsgPrefx();Serial.print("settingsConfig.OPAK: ");
  Serial.println(settingsConfig.OPAK);

  debugMsgPrefx();Serial.print("settingsConfig.StatusIntervalSec: ");
  Serial.println(settingsConfig.StatusIntervalSec);

  debugMsgPrefx();Serial.print("settingsConfig.TimeZoneOffsetHours: ");
  Serial.println(settingsConfig.TimeZoneOffsetHours);
}

String settingsConfigFile = "/config/settingsConfig.json";
bool loadSettings(){
  #ifdef _GDebug
  debugMsgPrefx();Serial.print("SettingsConfig file path: ");Serial.println(settingsConfigFile);
  #endif
  
  if (!SPIFFS.exists(settingsConfigFile))
  {
    #ifdef _GDebug
    debugMsg("[WARNING]: SettingsConfig file not found! Loading Factory Defaults.");
    #endif
    
    strlcpy(settingsConfig.OPURI,"", sizeof(settingsConfig.OPURI));
    settingsConfig.OPPort = 5000 ; //set to install default (maybe this should default to 80)
    strlcpy(settingsConfig.OPAK, "", sizeof(settingsConfig.OPAK));
    settingsConfig.StatusIntervalSec = 5;
    settingsConfig.TimeZoneOffsetHours = -5; //eastern(NewYork) time 
    return false;
  }
  
  File configfile = SPIFFS.open(settingsConfigFile,"r");
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, configfile);

  strlcpy(settingsConfig.OPURI, doc["OPURI"] | "FAILED", sizeof(settingsConfig.OPURI));
  
  int OPPort = doc["OPPort"] | 5000 ;
  
  settingsConfig.OPPort = OPPort;
  
  
  strlcpy(settingsConfig.OPAK, doc["OPAK"] | "FAILED", sizeof(settingsConfig.OPAK));

  int StatusIntervalSec = doc["StatusIntervalSec"] | 15 ;
  settingsConfig.StatusIntervalSec = StatusIntervalSec;

  int TimeZoneOffsetHours = doc["TimeZoneOffsetHours"] | -5 ;
  settingsConfig.TimeZoneOffsetHours = TimeZoneOffsetHours;

  configfile.close();

  if (error)
  {
    debugMsg("[ERROR]: deserializeJson() error in loadSettings");
    debugMsg(error.c_str());
    return false;
  }else{
    #ifdef _GDebug
    debugMsg("SettingsConfig was loaded successfully");
    DebugSettingsConfig();
    #endif
  }
  

  return true;  
}

bool saveSettings(SettingsConfig newSettings){
  #ifdef _GDebug
    debugMsgPrefx();Serial.print("settingsConfig file path: ");Serial.println(settingsConfigFile);
  #endif
  
  SPIFFS.remove(settingsConfigFile);
  File file = SPIFFS.open(settingsConfigFile, "w");

  DynamicJsonDocument doc(512);

  JsonObject Configobject = doc.to<JsonObject>();
  
  Configobject["OPURI"] = newSettings.OPURI;
  Configobject["OPPort"] = newSettings.OPPort;
  Configobject["OPAK"] = newSettings.OPAK;
  Configobject["StatusIntervalSec"] = newSettings.StatusIntervalSec;
  Configobject["TimeZoneOffsetHours"] = newSettings.TimeZoneOffsetHours;
  
  int chrW = serializeJsonPretty(doc, file);
  if (chrW == 0)
  {
    debugMsg("[WARNING]: Failed to write to settingsConfi file");
    return false;
  }else {
    settingsConfig = newSettings;
    debugMsgPrefx();Serial.print("Characters witten: ");Serial.println(chrW);
    debugMsg("settingsConfig was updated");
    DebugSettingsConfig();
  }
    
  file.close();
  return true;
}
