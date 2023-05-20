bool InAPMode = false;
bool APModeSuccess = false;
bool needReset = false;
bool doWiFiScan = false;
bool WifiInitialized = false;

TimeRelease ReconnectWiFi;
TimeRelease resetTimeDelay;


uint32_t getChipId(){
  return ESP.getChipId();
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
ESP8266WebServer webServer(80);
int lastWifiStatus = -1;


bool isWiFiConnected(bool debug=true){

  if(lastWifiStatus !=WiFi.status()){ // should only print out the status when it changes.
    lastWifiStatus =WiFi.status();
    #ifdef _GDebug
      Serial.print("WiFi Status: ");
      Serial.println(WiFi.status());
    #endif
  }
  return (WiFi.status() == WL_CONNECTED);
}


bool startWifiStation(){
  #ifdef _GDebug
  Serial.printf("[INFO]: Connecting to %s\n", wifiConfig.ssid);
  Serial.printf("Existing set WiFi.SSID:%s\n",WiFi.SSID().c_str());
  Serial.printf("wifiConfig.ssid:%s\n",wifiConfig.ssid);
  #endif
  if(String(wifiConfig.wifimode) == "WIFI_STA"){
    if ( String(WiFi.SSID()) != String(wifiConfig.ssid) || WifiInitialized == false)
    {
        #ifdef _GDebug
        Serial.println("initiallizing WiFi Connection");
        #endif
        if(isWiFiConnected()){
          WiFi.disconnect();
        }
        
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiConfig.ssid, wifiConfig.password);
        WifiInitialized = true;
        uint8_t attempts = wifiConfig.attempts;
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
       #ifdef _GDebugbug
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
  Serial.print("WiFi.Status(): ");
  Serial.println(String(WiFi.status()));
  Serial.print("WL_CONNECTED: ");
  Serial.println(String(WL_CONNECTED));
  #endif
  //needMqttConnect = (WiFi.status() == WL_CONNECTED); //get the MQtt System to connect once wifi is connected.
  return (WiFi.status() == WL_CONNECTED);
}



void DoInAPMode(){
  if(InAPMode && !APModeSuccess){
    //Going into AP mode
    #ifdef _GDebug
    Serial.println("Entering AP mode.");
    #endif
    if(wifiConfig.hostname == DEFAULT_HOSTS_NAME){
      String tempName = wifiConfig.hostname + String(getChipId()); 
      strlcpy(wifiConfig.hostname,tempName.c_str(),sizeof(wifiConfig.hostname));
    }
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    delay(100);
    APModeSuccess = WiFi.softAP(wifiConfig.hostname, wifiConfig.apPassword);
    if (APModeSuccess){
      #ifdef _GDebug
      Serial.print("SoftAP IP Address: ");
      Serial.println(WiFi.softAPIP());
      #endif
      ReconnectWiFi.set(15000); //15 seconds
    }else{
      APModeSuccess = true; // not really but if we do not do this, It will come though here and reset the reset delay every loop. 
      #ifdef _GDebug
      Serial.println("SoftAP mode Failed Rebooting in 15 seconds.");
      #endif
      resetTimeDelay.set(15000UL); //trigger 15 sec
      needReset = true;
    }
  }
}

void CheckWifi(){
   //check connection status
  if(!InAPMode || InAPMode && !APModeSuccess){
    
    if(!isWiFiConnected()){
      InAPMode = true; 
      DoInAPMode();
							 
							   
    }
    
  }else{
    //only try to reconnect if no one has connected as an AP client.
    if(ReconnectWiFi.check() && WiFi.softAPgetStationNum() ==0){
        InAPMode = !startWifiStation(); //attempt to reconnect to the main wifi access point if it succeds, Set to false.
        APModeSuccess = false; //reset so it will do a full attempt to go into AP mode if called to
    }else{
      
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
  Serial.print("wifiConfig.ssid: ");
  Serial.println(wifiConfig.ssid);
  Serial.print("wifiConfig.password: ");
  Serial.println(wifiConfig.password);
  Serial.print("wifiConfig.wifimode: ");
  Serial.println(wifiConfig.wifimode);
  Serial.print("wifiConfig.hostname: ");
  Serial.println(wifiConfig.hostname);
  Serial.print("wifiConfig.attempts: ");
  Serial.println(wifiConfig.attempts);
  Serial.print("wifiConfig.attemptdelay: ");
  Serial.println(wifiConfig.attemptdelay);
  Serial.print("wifiConfig.apPassword: ");
  Serial.println(wifiConfig.apPassword);
}

String wifiConfigFile = "/config/wifiConfig.json";
bool loadwifiConfig()
{
  #ifdef _GDebug
    Serial.print("wifiConfig file path: ");Serial.println(wifiConfigFile);
  #endif
  if (!SPIFFS.exists(wifiConfigFile))
  {
    #ifdef _GDebug
    Serial.println("[WARNING]: wifiConfig file not found! Loading Factory Defaults.");
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
    Serial.println("[ERROR]: deserializeJson() error in loadwifiConfig");
    Serial.println(error.c_str());
    return false;
  }else{
    #ifdef _GDebug
    Serial.println("wifiConfig was loaded successfully");
    DebugwifiConfig();
    #endif
  }
  

  return true;
}


bool savewifiConfig(WifiConfig newConfig)
{
  #ifdef _GDebug
    Serial.print("wifiConfig file path: ");Serial.println(wifiConfigFile);
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
    Serial.println("[WARNING]: Failed to write to webconfig file");
    return false;
  }else {
    wifiConfig = newConfig;
    #ifdef _GDebug
    Serial.print("Characters witten: ");Serial.println(chrW);
    Serial.println("wifiConfig was updated");
    DebugwifiConfig();
    #endif
  }
    
  file.close();
  return true;
}


void SetupWifi(){
  if(loadwifiConfig()){
    #ifdef _GDebug
    Serial.println("WiFi Config Loaded");
    #endif
    InAPMode  = false;
  }else{
    #ifdef _GDebug
    Serial.println("WiFi Config Failed to Load");
    #endif
    InAPMode  = true;
  }
  if(InAPMode){
    DoInAPMode();
  }else{
    startWifiStation();
  }
  #ifdef _GDebug
  Serial.println("Wifi Setup complete");
  #endif
}

void PrintNetStat(){
  Serial.print("Is Wifi Connected: ");
  Serial.println();
  if (isWiFiConnected(false)){
    Serial.println("yes");
    Serial.print("IP:");
    Serial.println(WiFi.localIP());
    
    Serial.print("SubNet:");
    Serial.println(WiFi.subnetMask());

    Serial.print("Gateway:");
    Serial.println(WiFi.gatewayIP());
    //would like to add other info here. Things like gateway and dns server and Maybe MQTT info
  }else{
    Serial.println("No");
  }
}

void DebugSettingsConfig(){
  Serial.print("settingsConfig.OPURI: ");
  Serial.println(settingsConfig.OPURI);
  
  Serial.print("settingsConfig.OPPort: ");
  Serial.println(settingsConfig.OPPort);
  
  Serial.print("settingsConfig.OPAK: ");
  Serial.println(settingsConfig.OPAK);

  Serial.print("settingsConfig.StatusIntervalSec: ");
  Serial.println(settingsConfig.StatusIntervalSec);

  Serial.print("settingsConfig.TimeZoneOffsetHours: ");
  Serial.println(settingsConfig.TimeZoneOffsetHours);
}

String settingsConfigFile = "/config/settingsConfig.json";
bool loadSettings(){
  #ifdef _GDebug
  Serial.print("SettingsConfig file path: ");Serial.println(settingsConfigFile);
  #endif
  
  if (!SPIFFS.exists(settingsConfigFile))
  {
    #ifdef _GDebug
    Serial.println("[WARNING]: SettingsConfig file not found! Loading Factory Defaults.");
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
    Serial.println("[ERROR]: deserializeJson() error in loadSettings");
    Serial.println(error.c_str());
    return false;
  }else{
    #ifdef _GDebug
    Serial.println("SettingsConfig was loaded successfully");
    DebugSettingsConfig();
    #endif
  }
  

  return true;  
}

bool saveSettings(SettingsConfig newSettings){
  #ifdef _GDebug
    Serial.print("settingsConfig file path: ");Serial.println(settingsConfigFile);
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
    Serial.println("[WARNING]: Failed to write to settingsConfi file");
    return false;
  }else {
    settingsConfig = newSettings;
    #ifdef _GDebug
    Serial.print("Characters witten: ");Serial.println(chrW);
    Serial.println("settingsConfig was updated");
    DebugSettingsConfig();
    #endif
  }
    
  file.close();
  return true;
}
