//Simple Serial IO Driver
//Takes serial commands to control configured IO.
//Reports IO status created for use with OctoPrint_SIOControl_plugin

#define VERSIONINFO "SIO_ESP32WRM_Relay_X2 1.0.5"
#define COMPATIBILITY "SIOPlugin 0.1.1"
#include "TimeRelease.h"
#include <ArduinoJson.h> 
#include <SPIFFS.h>   // Filesystem support header
#include "FS.h"
#define FS_NO_GLOBALS
#include <WiFi.h>
#include <Bounce2.h>


//note that there are some IO points that are not optimal to use as basic IO points. There state at start up of the 
//device migh cause issue where the device could go into programing mode or the IO pin might be pulsed on startup. 
//These pins on the Esp32wroom module are 0,1,3,5, and 12. 
//GPIO 0 outputs a PWM Signal at boot and will cause the module to go into programming mode if it is pulled low on boot. 
//GPIO 1 is the TX pin and will output debug data on boot.
//GPIO 3 is the RX pin and will go high on boot. 
//GPIO 5 outputs a PWM Signal at boot, is a strapping pin
//GPIO 12 boot will fail if it is pulled High during a boot, is a strapping pin
//These iopoints are used for SPI Flash (6,7,8,9,10,11) so they also should not be used by user code. 
//GPIO 14 outputs a PWM Signal at boot, is a strapping pin
//GPIO 15 outputs a PWM Signal at boot.
//Additionally IO 34,35,36,and39 can only be used as inputs.
//reference: https://randomnerdtutorials.com/esp32-pinout-reference-gpios/


#define IO0 34//  input only (Must configure (in code)without pullup or down. These inputs do not have these on chip)
#define IO1 35//  input only  (Must configure (in code)without pullup or down. These inputs do not have these on chip)
#define IO2 36//  input only (Labeled as SVP)  (Must configure (in code)without pullup or down. These inputs do not have these on chip)
#define IO3 39//  input only (Labeled as SVN)  (Must configure (in code)without pullup or down. These inputs do not have these on chip)
#define IO4 0//  general IO  Important note.. if this is in a low state on boot of the device, it will go into programing mode. 
#define IO5 5//  general IO
#define IO6 12//  general IO
#define IO7 13//  general IO
#define IO8 14//  general IO
#define IO9 33//  general IO
#define IO10 25//  general IO
#define IO11 16 // Relay1
#define IO12 17//  Relay2
#define IO13 23//  output has the board built in LED attached. Active LOW.



#define IOSize  14 //number should be one more than the IO# :) (must include the idea of Zero) 
bool _debug = false;
int IOType[IOSize]{INPUT,INPUT,INPUT,INPUT,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,OUTPUT,OUTPUT,OUTPUT};
int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6,IO7,IO8,IO9,IO10,IO11,IO12,IO13};
String IOSMap[IOSize] {"IO0","IO1","IO2","IO3","IO4","IO5","IO6","IO7","IO8","IO9","IO10","IO11","IO12","IO13"};
int IO[IOSize];
Bounce Bnc[IOSize];
bool EventTriggeringEnabled = 1;


bool isOutPut(int IOP){
  return (IOType[IOP] == OUTPUT);
}

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
  debugMsg("Disabling Wifi");
  WiFi.mode(WIFI_OFF);    //The entire point of this project is to not use wifi but have directly connected IO for use with the OctoPrint PlugIn and subPlugIn
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
  }
  
  IOReport.set(100ul);
  ReadyForCommands.set(reportInterval); //Initial short delay resets will be at 3x
  DisplayIOTypeConstants();
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
      if(_debug){debugMsgPrefx();Serial.print("NOT Output IO#:");Serial.println(i);}
      Bnc[i].update();
      
      if(Bnc[i].changed()){
       changed = true;
       IO[i]=Bnc[i].read();
       if(_debug){debugMsg("Input Changed: "+String(i));}
      }
    }else{
      //debugMsgPrefx();Serial.print("IS Output IO#:");Serial.println(i);
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
    
    if(sepPos){
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
    
    else if(command =="debug"){
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
    else if(command=="CIO"){ //set IO Configuration
      ack();
      if (validateNewIOConfig(value)){
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
    else if(command =="IO" && value.length() > 0){
      ack();
      int IOPoint = value.substring(0,value.indexOf(" ")).toInt();
      int IOSet = value.substring(value.indexOf(" ")+1).toInt();
      if(_debug){
        debugMsg("IO#:"+ String(IOMap[IOPoint]));
        debugMsg("Set to:"+ String(IOSet));
      }
      
      if(isOutPut(IOPoint)){
        if(IOSet == 1){
          digitalWrite(IOMap[IOPoint],HIGH);
        }else{
          digitalWrite(IOMap[IOPoint],LOW);
        }
      }else{
        debugMsg("ERROR: Attempt to set IO which is not an output");   
      }
      //delay(200); // give it a moment to finish changing the 
      reportIO(true);
      return;
    }
    
    //Set AutoReporting Interval  
    else if(command =="SI" && value.length() > 0){
      ack();
      unsigned long newTime = 0;
      if(strToUnsignedLong(value,newTime)){ //will convert to a full long.
        if(newTime >=500){
          reportInterval = newTime;
          IOReport.clear();
          IOReport.set(reportInterval);
          if(_debug){
            debugMsg("Auto report timing changed to:" +String(reportInterval));
          }
        }else{
          debugMsg("ERROR: minimum value 500");
        }
        return;
      }
      debugMsg("ERROR: bad format number out of range");
      return; 
    }
    //Enable event trigger reporting Mostly used for E-Stop
    else if(command == "SE" && value.length() > 0){
      ack();
      EventTriggeringEnabled = value.toInt();
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
    if(_debug){debugMsg("IOConfig validation failed(Wrong len)");}
    return false;  
  }

  for (int i=0;i<IOSize;i++){
    int pointType = ioConfig.substring(i,i+1).toInt();
    if(pointType > 4){//cant be negative. we would have a bad parse on the number so no need to check negs
      if(_debug){
        debugMsg("IOConfig validation failed");debugMsg("Bad IO Point type: index[" +String(i)+"] type["+pointType+"]");
      }
      return false;
    }
  }
  if(_debug){debugMsg("IOConfig validation good");}
  return true; //seems its a good set of point Types.
}

void updateIOConfig(String newIOConfig){
  for (int i=2;i<IOSize;i++){//start at 2 to avoid D0 and D1
    int nIOC = newIOConfig.substring(i,i+1).toInt();
    if(IOType[i] != nIOC){
      IOType[i] = nIOC;
      if(nIOC == OUTPUT){
        digitalWrite(IOMap[i],LOW); //set outputs to low since they will be high coming from INPUT_PULLUP
      }
    }
  }
}

int getIOType(String typeName){
  if(typeName == "INPUT"){return 1;}
  if(typeName == "OUTPUT"){return 2;}
  if(typeName == "INPUT_PULLUP"){return 5;}
  if(typeName == "INPUT_PULLDOWN"){return 9;}
  if(typeName == "OUTPUT_OPEN_DRAIN"){return 18;} //not sure on this value have to double check
}

bool strToUnsignedLong(String& data, unsigned long& result) {
  data.trim();
  long tempResult = data.toInt();
  if (String(tempResult) != data) { // check toInt conversion
    // for very long numbers, will return garbage, non numbers returns 0
   // Serial.print(F("not a long: ")); Serial.println(data);
    return false;
  } else if (tempResult < 0) { //  OK check sign
   // Serial.print(F("not an unsigned long: ")); Serial.println(data);
    return false;
  } //else
  result = tempResult;
  return true;
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


bool StoreIOConfig(){
  SPIFFS.remove("/IOConfig.json");
  File file = SPIFFS.open("/IOConfig.json", "w");
  if(_debug){debugMsg("saving IO Config");}
  DynamicJsonDocument doc(2048);

  JsonObject configObj = doc.to<JsonObject>();

  for (int i=0;i<IOSize;i++){
    configObj[IOSMap[i]]= getIOTypeString(IOType[i]);
  }
  
  
  if(_debug){
    debugMsg("Saved IO as json: ");    
    String jsonConfig;
    serializeJson(configObj, jsonConfig); 
    debugMsg(jsonConfig);
  }
    
  
  if (serializeJsonPretty(doc, file) == 0)
  {
    debugMsg("[WARNING]: Failed to write to file:/IOConfig.json");
    return false;
  }
  file.close();
  if(_debug){debugMsg("Saved IO Config");}
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

String getIOTypeString(int ioType){
  if (ioType == 1){return "INPUT";}
  if (ioType == 2){return "OUTPUT";}
  if (ioType == 5){return "INPUT_PULLUP";}
  if (ioType == 9){return "INPUT_PULLDOWN";}
  if (ioType == 18){return "OUTPUT_OPEN_DRAIN";}
}

#define FORMAT_SPIFFS_IF_FAILED true
void InitStorageSetup(){
   if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
      debugMsg("Warning SPIFFS Mount Failed (Attempt Restart if this is the first time this firmware was loaded.)");
      return;
   }

  
  //SPIFFS.begin();
  //do anything that might be needed related to file and storage on startup.
}

void debugMsg(String msg){
  debugMsgPrefx();
  Serial.println(msg);
}

void debugMsgPrefx(){
  Serial.print("DG:");
}
