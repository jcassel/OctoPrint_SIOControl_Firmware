#include <ArduinoJson.h> 

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
    if(_debug){
      debugMsgPrefx();Serial.printf("%d: %s, Ch:%d (%ddBm) %s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
    }
    
  }
  
  Networks += "]}";
  
  WiFi.scanDelete(); //not supported for esp8266
  
}

const char* www_realm = "admin@SIOControl";
String authFailResponse = "Authentication Failed";
bool checkAuth(){
  
  if(!webServer.authenticate(www_username,wifiConfig.apPassword)){
    #ifdef USE_DIGESTAUTH
    webServer.requestAuthentication(DIGEST_AUTH, www_realm, authFailResponse);
      if(_debug){
        debugMsgPrefx();
        Serial.print("AuthFailed?:");
        Serial.println(authFailResponse);
      }
    #else
    webServer.requestAuthentication();
    #endif

    return webServer.authenticate("admin",wifiConfig.apPassword);
    
  }else{
    return true;  
  }
  
}
bool handleFileRead(String path,bool forDownLoad=false) {
  if(!checkAuth())
    return false;
    
  if(_debug){
    debugMsgPrefx();Serial.println("start handleFileRead");
  }
  
  if (SPIFFS.exists(path)) {
    if(_debug){
      debugMsgPrefx();
      Serial.print("File found:");
      Serial.println(path);
    }
    File file = SPIFFS.open(path, "r");
    if(forDownLoad){
      webServer.streamFile(file, "text/plain");
    }else if(path.endsWith("css")){
      webServer.streamFile(file, "text/css");
    }else{
      webServer.streamFile(file, "text/html");
    }
    file.close();
    return true;
  }else{
    if(_debug){
      debugMsgPrefx();
      Serial.print("File not found");
      Serial.println(path);
    }
  }
  return false;
}

File fsUploadFile;  //SPIFFS file object to hold the new file info.
void handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = webServer.upload();
    
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    LastStatus = filename; //put this here for use later when done the upload.
    if(!filename.startsWith("/")) filename = "/"+filename;
    debugMsgPrefx();Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      debugMsgPrefx();Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      LastStatus = "File Upload success:" + LastStatus + ":" + String(upload.totalSize) + " bytes"; 
      webServer.sendHeader("Location","/spiffs");      // Redirect the client to the success page
      webServer.send(303);
    } else {
      webServer.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

/*
void GetFileList(String &PageContent){
  return;
}
*/
void GetFileList(String &PageContent){
  File dir = SPIFFS.open ("/");
  File file = dir.openNextFile();
  PageContent += "<p>";
  while (file) {
      PageContent += "<a href='/spiffsDL?download=";
      PageContent += file.name();
      PageContent += "'>";
      PageContent += file.name();
      PageContent += " ";
      PageContent += file.size();
      PageContent += "</a>";
      PageContent += "<br/>";
      file = dir.openNextFile();
  }
  
  PageContent += "<p>";
  return;
}


void getSpiffsPage(String &page){
    page = "<!DOCTYPE HTML><html><head><title>SPIFFS UPload</title><body>";
    page += "<form method='post' enctype='multipart/form-data'><input type='file' name='name'><input class='button' type='submit' value='Upload'></form>";
    GetFileList(page);
    page += "<p>Last Status: " + LastStatus + "</p>";
    page += "<p><a href='/config'>Return to Config</a></p>";
    page += "</body></html>";
}



void initialisePages(){
  if(_debug){
    debugMsgPrefx();Serial.println("Initializing pages start");
  }
  webServer.serveStatic("/config.js",SPIFFS,"/config.js");//.setAuthentication(www_username,wifiConfig.apPassword);
  webServer.serveStatic("/ioconfig.js",SPIFFS,"/ioconfig.js");//.setAuthentication(www_username,wifiConfig.apPassword);
  webServer.serveStatic("/style.css",SPIFFS,"/style.css");//.setAuthentication(www_username,wifiConfig.apPassword);//31536000
  
  
  
  webServer.on("/config",HTTP_GET,[]{
    if(_debug){
      debugMsg("start config");
    }
    
    handleFileRead("/config.htm");
    
    if(_debug){
      debugMsg("end config");
    }
  });

  webServer.on("/ioconfig",HTTP_GET,[]{
    if(_debug){
      debugMsg("start ioconfig");
    }
    
    handleFileRead("/ioconfig.htm");
    
    if(_debug){
      debugMsg("end ioconfig");
    }
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
    if(_debug){
      debugMsg("Network Scan requested");
    }
    webServer.send(200, "text/plain","OK" );
  });

  webServer.on("/APIGetNetworks",HTTP_GET,[]{
  if(!checkAuth()){return;}
    if(Networks.length() == 0){
      if(_debug){
        debugMsg("Sending Empty Network list");
      }
      webServer.send(200, "application/json", "{\"networks\":[]}");
    }else{
      if(_debug){
        debugMsg("Sending Network list");
      }
      
      webServer.send(200, "application/json", Networks);
      Networks = "";  
    }
  });

  webServer.on("/config",HTTP_POST,[]{
    if(!checkAuth()){return;}

    if(webServer.hasArg("update")){
      if(_debug){
        debugMsg("updating settings");
      }

      strlcpy(wifiConfig.wifimode, webServer.arg("cbo_WFMD").c_str(), sizeof(wifiConfig.wifimode));
      strlcpy(wifiConfig.hostname, webServer.arg("tx_HNAM").c_str(), sizeof(wifiConfig.hostname));
      strlcpy(wifiConfig.ssid, webServer.arg("tx_SSID").c_str(), sizeof(wifiConfig.ssid));
      strlcpy(wifiConfig.password, webServer.arg("tx_WFPW").c_str(), sizeof(wifiConfig.password));
      wifiConfig.attempts = webServer.arg("tx_WFCA").toInt();
      wifiConfig.attemptdelay = webServer.arg("tx_WFAD").toInt();
      strlcpy(wifiConfig.apPassword, webServer.arg("tx_APW").c_str(), sizeof(wifiConfig.apPassword));
      
      
      if(_debug){
        debugMsg("AfterUpdate: wificonfig");
        DebugwifiConfig();
        debugMsg("before direct Update: SettingsConfig");
        DebugSettingsConfig(); //prints the settings struct to the Serial line
      }
      
      
      
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


      
      debugMsg("After direct Update: SettingsConfig");
      DebugSettingsConfig(); //prints the settings struct to the Serial line
      
      
      savewifiConfig(wifiConfig);
      saveSettings(settingsConfig);
      //saveMQTTConfig(mqttConfig);
      
      LastStatus = "Update Completed(Ready!)";
      
    }else if(webServer.hasArg("reboot")){
      if(_debug){
        debugMsg("web called a reboot");
      }
      
      LastStatus = "Rebooting in 5 Sec";
      resetTimeDelay.set(10000UL); //need to give it more time so that the web page shows the message.
      needReset = true;
      
    }
    
    handleFileRead("/config.htm");
    if(_debug){
      debugMsg("end post config");
    }
    
  });
  
  webServer.on("/ioconfig",HTTP_POST,[]{
    if(!checkAuth()){return;}

    if(webServer.hasArg("update")){
      if(_debug){
        debugMsg("updating ioconfig");
      } 
      
      String newIOConfig = "";
      for (int i=0;i<IOSize;i++){      
        newIOConfig += webServer.arg("cbo_io"+String(i)+"_type");
      }

      if(_debug){
        debugMsgPrefx();Serial.print("New IO String:");
        Serial.println(newIOConfig);
      }

      updateIOConfig(newIOConfig);
      StoreIOConfig();
      
    }
    
    handleFileRead("/ioconfig.htm");
  
    if(_debug){
      debugMsg("end post config");
    }
    
  });
  
  webServer.on("/ioControl/status",HTTP_GET,[]{
    if(!checkAuth()){return;}
    if(_debug){
      debugMsg("[net] ioControl/status Called");
    }
    String ioData = "{\"IO\":{";
    ioData += "\"io0\":\"" + String(IO[0]) + "\""; //initial item (no comma)      
    for (int i=1;i<IOSize;i++){
      ioData += ",\"io" +String(i) +"\":\"" + String(IO[i]) + "\"";      
    }

    ioData += "}}";
    webServer.send(200, "application/json", ioData);
  });
  
  webServer.on("/ioControl/update",HTTP_GET,[]{
    if(!checkAuth()){return;}
    String response = "{\"OK\":\"";
      if(webServer.hasArg("IO")){
          String request = webServer.arg("IO");
          DynamicJsonDocument doc(256);
          DeserializationError error = deserializeJson(doc,request);
          // Test if parsing succeeds.
          if (error) {
            String eSt = error.f_str();
            response += "Error: deserializeJson() failed:";
            response += error.f_str();
            debugMsg(response); //serial response 
          }else{
            //Example set IO point 12 to a HIGH >>> {"point":[12,1]} >>> http://192.168.4.1/ioControl/update?IO={%22point%22:[12,1]}
            //Example set IO point 11 to a LOW >>> {"point":[11,0]}
            int ioPoint = doc["point"][0];
            int ioValue = doc["point"][1];
            if(isOutPut(ioPoint)){
              if(ioValue == 1){
                digitalWrite(IOMap[ioPoint],HIGH);//Sticking to constants incase there is a change there.
              }else{
                digitalWrite(IOMap[ioPoint],LOW);//Sticking to constants incase there is a change there.
              }
              response += "Success\"";
            }else{
              response += "Error: Attempt to set IO which is not an output\"";
              debugMsg(response);   
            }
          }
          response += "}";
          webServer.send(200, "application/json", response);
          
      }
  });

  webServer.on("/spiffsDL",HTTP_GET,[]{
    if(!checkAuth()){return;} //might be redundant since this is also done in the handleFileRead method
    if(webServer.hasArg("download")){
       String fileName = webServer.arg("download").c_str();
       handleFileRead(fileName,true);
    }
  });
  
  webServer.on("/spiffs",HTTP_GET,[]{
    if(!checkAuth()){return;} 
    String page = "";
    getSpiffsPage(page);
    webServer.send(200, "text/html",page );
  });

  webServer.on("/spiffs", HTTP_POST,[](){ 
    Serial.println("spiffs Called [post]");
    if(!checkAuth()){return;}
    webServer.send(200);}, // Send status 200 (OK) to tell the client we are ready to receive
    handleFileUpload      // Receive and save the file
  );

  
  
}
