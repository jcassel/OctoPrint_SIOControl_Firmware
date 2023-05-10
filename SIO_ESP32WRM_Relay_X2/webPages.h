String LastStatus = "Ready!"; //latest status message.
String Networks = "";

//takes the value from the wifi scan and turns it into a percentage signal strength
int dBmtoPercentage(int dBm)
{
  int quality;
    if(dBm <= -100)
    {
        quality = 0;
    }
    else if(dBm >= -50)
    {  
        quality = 100;
    }
    else
    {
        quality = 2 * (dBm + 100);
   }

     return quality;
}//dBmtoPercentage 


void storeWifiScanResult(int networksFound) {
  
  Networks = "{\"networks\":[";
  for (int i = 0; i < networksFound; i++){

    if((i + 1)< networksFound){
      Networks += "\"";
      Networks += WiFi.SSID(i);
      Networks += " (";
      Networks += dBmtoPercentage(WiFi.RSSI(i));
      Networks += "%)\",";
    }else{
      Networks += "\"";
      Networks += WiFi.SSID(i);
      Networks += " (";
      Networks += dBmtoPercentage(WiFi.RSSI(i));
      Networks += "%)\"";
    }
    #ifdef _GDebug
    Serial.printf("%d: %s, Ch:%d (%ddBm) %s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
    #endif
    
  }
  
  Networks += "]}";
  
  //WiFi.scanDelete(); not supported for esp8266
  
}

const char* www_realm = "Login Required";
String authFailResponse = "Authentication Failed";
bool checkAuth(){
  
  if(!webServer.authenticate(www_username,wifiConfig.apPassword)){
    #ifdef USE_DIGESTAUTH
    webServer.requestAuthentication(DIGEST_AUTH, www_realm, authFailResponse);
      #ifdef _GDebug
      Serial.print("AuthFailed?:");
      Serial.println(authFailResponse);
      #endif
    #else
    webServer.requestAuthentication();
    #endif

    return webServer.authenticate("admin",wifiConfig.apPassword);
    
  }else{
    return true;  
  }
  
}
bool handleFileRead(String path) {
  if(!checkAuth())
    return false;
    
  #ifdef _GDebug
  Serial.println("start handleFileRead");
  #endif
  
  if (SPIFFS.exists(path)) {
    #ifdef _GDebug
    Serial.print("File found:");
    Serial.println(path);
    #endif
    File file = SPIFFS.open(path, "r");
    if(path.endsWith("css")){
      webServer.streamFile(file, "text/css");
    }else{
      webServer.streamFile(file, "text/html");
    }
    file.close();
    return true;
  }else{
    #ifdef _GDebug
    Serial.print("File not found");
    Serial.println(path);
    #endif
  }
  return false;
}

void initialisePages(){
  #ifdef _GDebug
  Serial.println("Initializing pages start");
  #endif
  webServer.serveStatic("/config.js",SPIFFS,"/config.js");//.setAuthentication(www_username,wifiConfig.apPassword);
  webServer.serveStatic("/ioconfig.js",SPIFFS,"/ioconfig.js");//.setAuthentication(www_username,wifiConfig.apPassword);
  webServer.serveStatic("/style.css",SPIFFS,"/style.css");//.setAuthentication(www_username,wifiConfig.apPassword);//31536000
  
  
  
  webServer.on("/config",HTTP_GET,[]{
    #ifdef _GDebug
    Serial.println("start config");
    #endif
    
    handleFileRead("/config.htm");
    
    #ifdef _GDebug
    Serial.println("end config");
    #endif
  });

  webServer.on("/ioconfig",HTTP_GET,[]{
    #ifdef _GDebug
    Serial.println("start ioconfig");
    #endif
    
    handleFileRead("/ioconfig.htm");
    
    #ifdef _GDebug
    Serial.println("end ioconfig");
    #endif
  });
  
  webServer.on("/APIGetIOSettings",HTTP_GET,[]{
    if(!checkAuth()){return ;}
    String settings = "{\"settings\":{";
    settings += "\"LastStatus\":\"" + LastStatus + "\"";
    settings += ",\"hostname\":\"" + String(wifiConfig.hostname) + "\"";
    settings += ",\"firmwareV\":\"" + String(VERSIONINFO)+ "\"";
    settings += ",\"IO\":{";
    settings += "\"io0_type\":\"" + String(IOType[0]) + "\""; //initial item (no comma)      
    for (int i=1;i<IOSize;i++){
      settings += ",\"io" +String(i) +"_type\":\"" + IOType[i]+ "\"";      
    }

    settings += "}}}";
    webServer.send(200, "application/json", settings);
  });
  
  webServer.on("/APIGetSettings",HTTP_GET,[]{
    if(!checkAuth()){return ;}
      
    String settings = "{\"settings\":{";
    settings += "\"wifimode\":\"" + String(wifiConfig.wifimode) + "\"";
    settings += ",\"hostname\":\"" + String(wifiConfig.hostname) + "\"";
    settings += ",\"ssid\":\"" + String(wifiConfig.ssid) + "\"";
    settings += ",\"password\":\"" + String(wifiConfig.password) + "\"";
    settings += ",\"attempts\":\"" + String(wifiConfig.attempts) + "\"";
    settings += ",\"attemptdelay\":\"" + String(wifiConfig.attemptdelay) + "\"";
    settings += ",\"apPassword\":\"" + String(wifiConfig.apPassword) + "\"";
    
    /*
    settings += ",\"MQTTEN\":\"" + String(mqttConfig.MQTTEN) + "\"";
    settings += ",\"MQTTSrvIp\":\"" + String(mqttConfig.MQTTSrvIp) + "\"";
    settings += ",\"MQTTPort\":\"" + String(mqttConfig.MQTTPort) + "\"";
    settings += ",\"MQTTUser\":\"" + String(mqttConfig.MQTTUser) + "\"";
    settings += ",\"MQTTPW\":\"" + String(mqttConfig.MQTTPW) + "\"";
    */
    
    settings += ",\"OPURI\":\"" + String(settingsConfig.OPURI) + "\""; // octoprint url
    settings += ",\"OPPort\":\"" + String(settingsConfig.OPPort) + "\"";//octoprint port
    settings += ",\"OPAK\":\"" + String(settingsConfig.OPAK) + "\""; //octoprint apikey
    settings += ",\"StatusIntervalSec\":\"" + String(settingsConfig.StatusIntervalSec) + "\"";
    
    settings += ",\"TimeZoneOffsetHours\":\"" + String(settingsConfig.TimeZoneOffsetHours) + "\"";
    settings += ",\"LastStatus\":\"" + LastStatus + "\"";
    
    settings += ",\"firmwareV\":\"" + String(VERSIONINFO)+ "\"";
    settings += "}}";
    
    webServer.send(200, "application/json", settings);
  });
  
  webServer.on("/getDeviceName",HTTP_GET,[]{
  if(!checkAuth()){ return;}
    
    String msg = String(wifiConfig.hostname) + ": " + String(VERSIONINFO);
    
    webServer.send(200, "text/plain", msg);
  });

  webServer.on("/APIScanWifi",HTTP_GET,[]{
    if(!checkAuth()){return ;}
      
    int n = WiFi.scanNetworks();
    storeWifiScanResult(n);
    doWiFiScan = true;
    #ifdef _GDebug
    Serial.print("Network Scanned requested:");
    #endif
    webServer.send(200, "text/plain","OK" );
  });

  webServer.on("/APIGetNetworks",HTTP_GET,[]{
  if(!checkAuth()){return;}
    if(Networks.length() == 0){
      #ifdef _GDebug
      Serial.println("Sending Empty Network list");
      #endif
      webServer.send(200, "application/json", "{\"networks\":[]}");
    }else{
      #ifdef _GDebug
      Serial.println("Sending Network list");
      #endif
      
      webServer.send(200, "application/json", Networks);
      Networks = "";  
    }
  });

  webServer.on("/config",HTTP_POST,[]{
    if(!checkAuth()){return;}

    if(webServer.hasArg("update")){
      #ifdef _GDebug
      Serial.println("updating settings");
      #endif

      strlcpy(wifiConfig.wifimode, webServer.arg("cbo_WFMD").c_str(), sizeof(wifiConfig.wifimode));
      strlcpy(wifiConfig.hostname, webServer.arg("tx_HNAM").c_str(), sizeof(wifiConfig.hostname));
      strlcpy(wifiConfig.ssid, webServer.arg("tx_SSID").c_str(), sizeof(wifiConfig.ssid));
      strlcpy(wifiConfig.password, webServer.arg("tx_WFPW").c_str(), sizeof(wifiConfig.password));
      wifiConfig.attempts = webServer.arg("tx_WFCA").toInt();
      wifiConfig.attemptdelay = webServer.arg("tx_WFAD").toInt();
      strlcpy(wifiConfig.apPassword, webServer.arg("tx_APW").c_str(), sizeof(wifiConfig.apPassword));
      
      
      #ifdef _GDebug
      Serial.println("AfterUpdate: wificonfig");
      DebugwifiConfig();
      #endif
      

      //MQTT settings (future)
      /*
      mqttConfig.MQTTEN = webServer.arg("ch_MQTTEN").toInt();
      strlcpy(mqttConfig.MQTTSrvIp, webServer.arg("tx_MQTTSRV").c_str(), sizeof(mqttConfig.MQTTSrvIp));
      strlcpy(mqttConfig.MQTTUser, webServer.arg("tx_MQTTUSR").c_str(), sizeof(mqttConfig.MQTTUser));
      strlcpy(mqttConfig.MQTTPW, webServer.arg("tx_MQTTPW").c_str(), sizeof(mqttConfig.MQTTPW));
      mqttConfig.MQTTPort = webServer.arg("tx_MQTTPort").toInt();
      */
      
      strlcpy(settingsConfig.OPURI, webServer.arg("tx_OPURI").c_str(), sizeof(settingsConfig.OPURI));
      settingsConfig.OPPort = webServer.arg("tx_OPPort").toInt();
      strlcpy(settingsConfig.OPAK, webServer.arg("tx_OPAK").c_str(), sizeof(settingsConfig.OPAK));
      settingsConfig.StatusIntervalSec = webServer.arg("tx_STI").toInt();
      settingsConfig.TimeZoneOffsetHours = webServer.arg("tx_TZOS").toInt();


      #ifdef _GDebug
      Serial.println("AfterUpdate: SettingsConfig");
      DebugSettingsConfig(); //prints the settings struct to the Serial line
      #endif 
      
      savewifiConfig(wifiConfig);
      saveSettings(settingsConfig);
      //saveMQTTConfig(mqttConfig);
      
      LastStatus = "Update Completed(Ready!)";
      
    }else if(webServer.hasArg("reboot")){
      #ifdef _GDebug
      Serial.println("web called a reboot");
      #endif
      
      LastStatus = "Rebooting in 5 Sec";
      resetTimeDelay.set(10000UL); //need to give it more time so that the web page shows the message.
      needReset = true;
      
    }
    
    handleFileRead("/config.htm");
    #ifdef _GDebug
    Serial.println("end post config");
    #endif
    
  });
  
  webServer.on("/ioconfig",HTTP_POST,[]{
    if(!checkAuth()){return;}

    if(webServer.hasArg("update")){
      #ifdef _GDebug
      Serial.println("updating ioconfig");
      #endif  
      
      String newIOConfig = "";
      for (int i=0;i<IOSize;i++){      
        newIOConfig += webServer.arg("cbo_io"+String(i)+"_type");
      }

      #ifdef _GDebug
      Serial.print("New IO String:");
      Serial.println(newIOConfig);
      #endif

      updateIOConfig(newIOConfig);
      StoreIOConfig();
      
    }
    
    handleFileRead("/ioconfig.htm");
  
    #ifdef _GDebug
    Serial.println("end post config");
    #endif
    
  });
  
  
}
