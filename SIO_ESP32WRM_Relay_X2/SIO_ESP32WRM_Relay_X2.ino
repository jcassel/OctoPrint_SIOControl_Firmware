//Simple Serial IO Driver
//Takes serial commands to control configured IO.
//Reports IO status created for use with OctoPrint_SIOControl_plugin
//#define ENABLE_WIFI_SUPPORT
//#define USE_DIGESTAUTH //use this for a little extra security. Maybe change the digest and default password too.

#define VERSIONINFO "SIO_ESP32WRM_Relay_X2 1.0.9"
#define COMPATIBILITY "SIOPlugin 0.1.1"
#define DEFAULT_HOSTS_NAME "SIOControler-New"

#include "TimeRelease.h"
#include <ArduinoJson.h>  
#include <SPIFFS.h>   // Filesystem support header
#include <Bounce2.h>
#include "FS.h"
#include "global.h"

#define FS_NO_GLOBALS
#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <Time.h>        
#include <TimeLib.h>


#include "wifiCommon.h"
#include "webPages.h"



void ConfigIO(){
  
  debugMsg("Setting IO");
  for (int i=0;i<IOSize;i++){
    if(IOType[i] == INPUT ||IOType[i] == INPUT_PULLUP || IOType[i] == INPUT_PULLDOWN){ //if it is an input
      pinMode(IOMap[i],IOType[i]);
      Bnc[i].attach(IOMap[i],IOType[i]);
      Bnc[i].interval(5);
    }else{
      pinMode(IOMap[i],IOType[i]);
      digitalWrite(IOMap[i],LOW);
    }
  }

}



TimeRelease IOReport;
TimeRelease ReadyForCommands;
unsigned long reportInterval = 3000;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(300);
  debugMsg(VERSIONINFO);
  _debug = true; //comment out if you dont want to see this info on startup.
   
  debugMsg("Start Types");
  InitStorageSetup();
  loadIOConfig();
  debugMsg("Initializing IO");
  ConfigIO();


  
  reportIOTypes();
  debugMsg("End Types");

  //need to get a baseline state for the inputs and outputs.
  for (int i=0;i<IOSize;i++){
    IO[i] = digitalRead(IOMap[i]);
    debugMsgPrefx();Serial.print("IO[");
    Serial.print(i);
    Serial.print("]: ");
    Serial.println(IO[i]);  
  }

  
  
  IOReport.set(100ul);
  ReadyForCommands.set(reportInterval); //Initial short delay resets will be at 3x
  DisplayIOTypeConstants();

  #ifdef ENABLE_WIFI_SUPPORT
  SetupWifi();
  if(_debug){
    DebugwifiConfig();
  }
  #else
  debugMsg("Disabling Wifi");
  WiFi.mode(WIFI_OFF);    //The entire point of this project is to not use wifi but have directly connected IO for use with the OctoPrint PlugIn and subPlugIn
  #endif

 if(!loadSettings()){
    if(_debug){
      debugMsg("Settings failed to load");
    }
  }else{
    if(_debug){
      debugMsg("Settings loaded from filesystem.");
      DebugSettingsConfig();
    }
  }
  
  #ifdef ENABLE_WIFI_SUPPORT
  debugMsg("begin init pages");
  initialisePages();
  debugMsg("end init pages");
  webServer.begin();
  debugMsg("end webserver.begin");
  #endif
  
  _debug = false; //start up is over.. debug in loop can be turned on by serial command.
}


bool _pauseReporting = false;
bool ioChanged = false;

//*********************Start loop***************************//
void loop() {
  // put your main code here, to run repeatedly:
  checkSerial();
  if(!_pauseReporting){
    ioChanged = checkIO();
    reportIO(ioChanged);
    if(ReadyForCommands.check()){
      Serial.println("RR"); //send ready for commands   
      ReadyForCommands.set(reportInterval *3);
    }
  }
  #ifdef ENABLE_WIFI_SUPPORT
  CheckWifi();
  #endif 
  
}
//*********************End loop***************************//

void reportIO(bool forceReport){
  if (IOReport.check()||forceReport){
    Serial.print("IO:");
    for (int i=0;i<IOSize;i++){
      //if(IOType[i] == 1 ){ //if it is an input
        IO[i] = digitalRead(IOMap[i]);  
      //}
      Serial.print(IO[i]);
    }
    Serial.println();
    IOReport.set(reportInterval);
    
  }
}

bool checkIO(){
  bool changed = false;
  
  for (int i=0;i<IOSize;i++){
    if(!isOutPut(i)){
      Bnc[i].update();
      if(Bnc[i].changed()){
       changed = true;
       IO[i]=Bnc[i].read();
       if(_debug){debugMsg("Input Changed: "+String(i));}
      }
    }else{
      
      //is the current state of this output not the same as it was on last report.
      //this really should not happen if the only way an output can be changed is through Serial commands.
      //the serial commands force a report after it takes action.
      if(IO[i] != digitalRead(IOMap[i])){
        if(_debug){debugMsg("Output Changed: "+String(i));}
        changed = true;
      }
    }
    
  }
    
  return changed;
}


void reportIOTypes(){
  Serial.print("IT:");
  for (int i=0;i<IOSize;i++){
    Serial.print(String(IOType[i]));
    Serial.print(",");
    //if(i<IOSize-1){Serial.print(";");}
  }
  Serial.println();
  debugMsg("IOSize:" + String(IOSize));
}

void DisplayIOTypeConstants(){
  debugMsgPrefx();Serial.print("INPUT:");Serial.println(INPUT);//1
  debugMsgPrefx();Serial.print("OUTPUT:");Serial.println(OUTPUT);//2
  debugMsgPrefx();Serial.print("INPUT_PULLUP:");Serial.println(INPUT_PULLUP);//5
  debugMsgPrefx();Serial.print("INPUT_PULLDOWN:");Serial.println(INPUT_PULLDOWN);//9
  debugMsgPrefx();Serial.print("OUTPUT_OPEN_DRAIN:");Serial.println(OUTPUT_OPEN_DRAIN);//18
}



void checkSerial(){
  if (Serial.available()){
    
      String buf = Serial.readStringUntil('\n');
    buf.trim();
    if(_debug){debugMsg("buf:" + buf);}
    int sepPos = buf.indexOf(" ");
    String command ="";
    String value = "";
    
    if(sepPos != -1){
      command = buf.substring(0,sepPos);
      value = buf.substring(sepPos+1);;
      if(_debug){
        debugMsg("command:[" + command + "]");
        debugMsg("value:[" + value+ "]");
      }
    }else{
      command = buf;
      if(_debug){
        debugMsg("command:[" + command + "]");
      }
    }
    
    if(command.startsWith("N") || command == "M115"){
      ack();
      Serial.println("Recieved a command that looks like OctoPrint thinks I am a printer.");
      Serial.println("This is not a printer it is a SIOControler. Sending disconnect host action command.");
      Serial.println("To avoide this you should place the name/path of this port in the [Blacklisted serial ports] section of Octoprint");
      Serial.println("This can be found in the settings dialog under 'Serial Connection'");
      Serial.println("//action:disconnect");
    }
    else if(command == "BIO"){ 
      ack();
      _pauseReporting = false; //restarts IO reporting 
      return;
    }    
    else if(command == "EIO"){ //this is the command meant to test for good connection.
      ack();
      _pauseReporting = true; //stops all IO reporting so it does not get in the way of a good confimable response.
      return;
    }
    else if (command == "VC"){//version and compatibility
      Serial.print("VI:");
      Serial.println(VERSIONINFO);
      Serial.print("CP:");
      Serial.println(COMPATIBILITY);
    }
    else if (command == "IC") { //io count.
      ack();
      Serial.print("IC:");
      Serial.println(IOSize -1);
      return;
    }
    
    else if(command =="debug" && value.length() == 1){
      ack();
      if(value == "1"){
        _debug = true;
        debugMsg("Serial debug On");
      }else{
        _debug=false;
        debugMsg("Serial debug Off");
      }
      return;
    }
    else if(command=="CIO"){
      ack();
      if(value.length() == 0){ //set IO Configuration
        debugMsg("ERROR: command value out of range");
      }else if (validateNewIOConfig(value)){
        updateIOConfig(value);
      }
      return;
    }
    else if(command=="SIO"){
      ack();
      StoreIOConfig();
      return;
    }
    else if(command=="IOT"){
      ack();
      reportIOTypes();
      return;
    }
    
    //Set IO point high or low (only applies to IO set to output)
    else if(command =="IO"){ 
      ack();
      if(value.length() < 3){
        debugMsg("ERROR: command value out of range");
        return;
      }
      
      if(value.indexOf(" ") >=1 && value.substring(value.indexOf(" ")+1).length()==1){//is there at least one character for the IO number and the value for this IO point must be only one character (1||0)
        String sIOPoint = value.substring(0,value.indexOf(" "));
        String sIOSet = value.substring(value.indexOf(" ")+1); // leaving this as a string allows for easy check on correct posible values
                 
        if(_debug){
          debugMsg("IO#:"+ sIOPoint);
          debugMsg("GPIO#:"+ String(IOMap[sIOPoint.toInt()]));
          debugMsg("Set to:"+ sIOSet);
        }

        //checks to see if the IO point is not a number if the string is not a zero but the number is that means it was not a number
        if(isOutPut(sIOPoint.toInt()) && !(sIOPoint.toInt() == 0 && sIOPoint != "0") ){
          if(sIOSet == "0" || sIOSet == "1"){// need to make sure it is a valid IO point value 
            if(sIOSet == "1"){
              digitalWrite(IOMap[sIOPoint.toInt()],HIGH);
            }else{
              digitalWrite(IOMap[sIOPoint.toInt()],LOW);
            }
          }else{
            debugMsg("ERROR: Attempt to set IO to a value that is not valid");   
          }
        }else{
          debugMsg("ERROR: Attempt to set IO which is not an output");   
        }
        //delay(200); // give it a moment to finish changing the 
        reportIO(true);
      }else{
        debugMsg("ERROR: IO point or value is invalid");   
      }
      return;
    }
    
    //Set AutoReporting Interval  
    else if(command =="SI"){ 
      ack();
      if(value.length() > 2){
        long newTime = value.toInt();
        if(newTime >=500 && (String(newTime) == value)){
          reportInterval = newTime;
          IOReport.clear();
          IOReport.set(reportInterval);
          debugMsg("Auto report timing changed to:" +String(reportInterval));
        }else{
          debugMsg("ERROR: command value out of range: Min(500) Max(2147483647)");
        }
        return;
      }
      debugMsg("ERROR: bad format number out of range.(500- 2147483647); actual sent [" + value + "]; Len["+String(value.length())+"]");
      return; 
    }
    //Enable event trigger reporting Mostly used for E-Stop
    else if(command == "SE"){
      ack();
      if(value.length() == 1){
        if(value == "1" || value == "0"){
          EventTriggeringEnabled = value.toInt();
        }else{
          debugMsg("ERROR: command value out of range");
        }
      }else{
        debugMsg("ERROR: command value not sent or bad format");
      }
      return;
    }
    else if (command == "restart" || command == "reset" || command == "reboot"){
      debugMsg("[WARNING]: Restarting device");
      ESP.restart();
    }

    //Get States 
    else if(command == "GS"){
      ack();
      reportIO(true);
      return; 
    }
    
    #ifdef ENABLE_WIFI_SUPPORT
    else if(command == "WF"){
      
      if(value == "netstat"){ack();PrintNetStat();}
      if(value == "filelist"){
        ack();
        //list files in SPIFF
      }
      if(value.startsWith("set")){
        ack();
        WifiConfig wfconfig;
        
        //wfconfig.ssid = wifiConfig.ssid;
        strlcpy(wfconfig.ssid, wifiConfig.ssid, sizeof(wfconfig.ssid));
        //wfconfig.password = wifiConfig.password;
        strlcpy(wfconfig.password, wifiConfig.password, sizeof(wfconfig.password));
        //wfconfig.wifimode = wifiConfig.wifimode;
        strlcpy(wfconfig.wifimode, wifiConfig.wifimode, sizeof(wfconfig.wifimode));
        //wfconfig.hostname = wifiConfig.hostname;
        strlcpy(wfconfig.hostname, wifiConfig.hostname, sizeof(wfconfig.hostname));
        wfconfig.attempts = wifiConfig.attempts;
        wfconfig.attemptdelay = wifiConfig.attemptdelay;
        //wfconfig.apPassword = wifiConfig.apPassword;
        strlcpy(wfconfig.apPassword, wifiConfig.apPassword, sizeof(wfconfig.apPassword));
        String nameValuePare = value.substring(value.indexOf(" ")+1);
        String setting = nameValuePare.substring(0,nameValuePare.indexOf(" "));
        String newValue = nameValuePare.substring(nameValuePare.indexOf(" ")+1);
        if(_debug){
        Serial.print("Wifi Setting entered:");Serial.println(setting);
        Serial.print("Wifi value entered:");Serial.println(newValue);
        }
        if(setting == "ssid"){
          //wfconfig.ssid = newValue;
          strlcpy(wfconfig.ssid, newValue.c_str(), sizeof(wfconfig.ssid));
        }else if (setting == "password"){
          //wfconfig.password = newValue;
          strlcpy(wfconfig.password, newValue.c_str(), sizeof(wfconfig.password));
        }else if (setting == "hostname"){
          //wfconfig.hostname = newValue;
          strlcpy(wfconfig.hostname, newValue.c_str(), sizeof(wfconfig.hostname));
        }else if (setting == "attempts"){
          wfconfig.attempts = newValue.toInt();
        }else if (setting == "attemptdelay"){
          wfconfig.attemptdelay = newValue.toInt();
        }else if (setting == "apPassword"){
          //wfconfig.apPassword = newValue;
          strlcpy(wfconfig.apPassword, newValue.c_str(), sizeof(wfconfig.apPassword));
        }else if (setting == "wifimode"){
          // WIFI_AP or WIFI_STA
          if(newValue == "STA"){
            //wfconfig.wifimode = "WIFI_STA";
            strlcpy(wfconfig.wifimode, "WIFI_STA", sizeof(wfconfig.wifimode));
          }else if(newValue == "AP"){
            //wfconfig.wifimode = "WIFI_AP";
            strlcpy(wfconfig.wifimode, "WIFI_AP", sizeof(wfconfig.wifimode));
          }
        }
        if(_debug){
          debugMsg("saving updates to wifiConfig");
        }
        savewifiConfig(wfconfig);
        loadwifiConfig();
      }
    }
    #endif //ENABLE_WIFI_SUPPORT
    else{
      debugMsg("ERROR: Unrecognized command["+command+"]");
    }
  }
}

void ack(){
  Serial.println("OK");
}





bool validateNewIOConfig(String ioConfig){
  
  if(ioConfig.length() != IOSize){
    debugMsg("IOConfig validation failed(Wrong len): required len["+ String(IOSize) + "] supplied len[" +String(ioConfig.length()) + "]" );
    return false;  
  }
  
  for (int i=0;i<IOSize;i++){
    String spointType = ioConfig.substring(i,i+1);
    int pointType = ioConfig.substring(i,i+1).toInt();
    //if point type is a non number it will parse to zero(0) cant be out of range (-n) or (4+)
    if((spointType != "0" && pointType == 0) || pointType > 4 || pointType < 0){
      debugMsg("IOConfig validation failed");debugMsg("Bad IO Point type: index[" +String(i)+"] type["+spointType+"]");
      return false;
    }
  }
  if(_debug){debugMsg("IOConfig validation good");}
  return true; //seems its a good set of point Types.
}



int getIOType(String typeName){
  if(typeName == "INPUT"){return 1;}
  if(typeName == "OUTPUT"){return 2;}
  if(typeName == "INPUT_PULLUP"){return 5;}
  if(typeName == "INPUT_PULLDOWN"){return 9;}
  if(typeName == "OUTPUT_OPEN_DRAIN"){return 18;} //not sure on this value have to double check
}


bool loadIOConfig(){
  
if (!SPIFFS.exists("/IOConfig.json"))
  {
    debugMsg("[WARNING]: IOConfig file not found!");
    debugMsg("Using Default Config");
    return false;
  }
  File configfile = SPIFFS.open("/IOConfig.json","r");

  DynamicJsonDocument doc(2048);

  DeserializationError error = deserializeJson(doc, configfile);
  for (int i=0;i<IOSize;i++){
    IOType[i] = getIOType(doc[IOSMap[i]]) ;
  }

  if(_debug){
    debugMsg("loaded IO From json: ");
    String jsonConfig;
    serializeJson(doc, jsonConfig); 
    debugMsg(jsonConfig);  
    //serializeJson(doc, Serial);
  }
  configfile.close();

  if (error)
  {
    debugMsg("[ERROR]: deserializeJson() error in loadIOConfig");
    debugMsg(error.c_str());
    return false;
  }else{
    debugMsg("Loaded IO Config from storage");
  }
  
  return true;
}






float SpaceLeft()
{
  float freeMemory = SPIFFS.totalBytes() - SPIFFS.usedBytes();
  return freeMemory;
}


bool IsSpaceLeft()
{
  float minmem = 100000.00; // Always leave 100 kB free pace on SPIFFS
  float freeMemory = SpaceLeft();
  if(_debug){String s = "[INFO]: Free memory left: "; s += freeMemory; s += "bytes"; debugMsg(s);}
  
  if (freeMemory < minmem)
  {
    return false;
  }

  return true;
}


#define FORMAT_SPIFFS_IF_FAILED true

void InitStorageSetup(){
   if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
      debugMsg("Warning SPIFFS Mount Failed (Attempt Restart if this is the first time this firmware was loaded.)");
      return;
   }
}
