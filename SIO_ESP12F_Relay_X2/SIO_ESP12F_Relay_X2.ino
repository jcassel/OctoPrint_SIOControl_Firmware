//simple Serial IO Driver
//Takes serial commands to control configured IO.
//Can create automations for interactions between inputs, outputs and actions. (Future)

//Eventually should support a closed loop temerature control. Need to have a sensor and a output to control a heating element. Could also do a fan / exaust system to cool enclosure down. 
//Likely will want to do the config for this in a web page but could be done also through OctoPrint

//Target outputs 
//Digital/relay: PSU,Lighting,Enclosure heater, alarm, filter relay,exaust fan
//Digital (PWM):cooling fan(PWM*) (Future)

//Target inputs
//Digital: General switches(NO or NC Selectable)
//temperature inputs(Future + TBA on type)


#define VERSIONINFO "SIO_ESP12F_Relay_X2 1.0.5"
#define COMPATIBILITY "SIOPlugin 0.1.1"
#include "TimeRelease.h"
#include <ArduinoJson.h> 
#include "FS.h"
#define FS_NO_GLOBALS
#include <ESP8266WiFi.h>
#include <Bounce2.h>

//gpio 0,2 and, 15 presents some challanges for use as an IO point. You can use at least 2 of them for IO but you must do it in the specific way needed for the specific pin.
//These are not including in the IO list here due to this challange.
//this link has some details on how to use the IO points 0,2 and 15 as inputs or outputs
//https://www.instructables.com/ESP8266-Using-GPIO0-GPIO2-as-inputs/

#define IO0 LED_BUILTIN //Ya this is on the chip.. so we can show some kind of staus of we want. good test IO Point to play with. Blue LED on MCU
#define IO1 5 //  Relay1 has relay and red led D1 on board near power in terminal
#define IO2 4//   Relay1 has relay and red led D4 on board near power in terminal
#define IO3 16// has blue LED on board D7 
#define IO4 14//   
#define IO5 12//
#define IO6 13//  

 
  
//#define IOA A0//Analog 0 


#define IOSize  7
bool _debug = false;
int IOType[IOSize]{OUTPUT,OUTPUT,OUTPUT,OUTPUT,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP};
int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6};
String IOSMap[IOSize] {"IO0","IO1","IO2","IO3","IO4","IO5","IO6"};
int IO[IOSize];
Bounce Bnc[IOSize];
bool EventTriggeringEnabled = 1;


bool isOutPut(int IOP){
  return IOType[IOP] == OUTPUT; 
}

void ConfigIO(){
  
  debugMsg("Setting IO");
  for (int i=0;i<IOSize;i++){
    if(IOType[i] == 0 ||IOType[i] == 2 || IOType[i] == 3){ //if it is an input
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
TimeRelease IOTimer[9];
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
  _debug = false;
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
      if(IOType[i] == 1 ){ //if it is an output
        IO[i] = digitalRead(IOMap[i]);  
      }
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
    Serial.print(IOType[i]);
    //if(i<IOSize-1){Serial.print(";");}
  }
  Serial.println();
}

void DisplayIOTypeConstants(){
  debugMsgPrefx();Serial.print("INPUT:");Serial.println(INPUT);//0
  debugMsgPrefx();Serial.print("OUTPUT:");Serial.println(OUTPUT);//1
  debugMsgPrefx();Serial.print("INPUT_PULLUP:");Serial.println(INPUT_PULLUP);//2
  //debugMsgPrefx();Serial.print("INPUT_PULLDOWN:");Serial.println(INPUT_PULLDOWN);//9 not avalible in ESP8266
  debugMsgPrefx();Serial.print("OUTPUT_OPEN_DRAIN:");Serial.println(OUTPUT_OPEN_DRAIN);//3
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
  if(typeName == "INPUT"){return 0;}
  if(typeName == "OUTPUT"){return 1;}
  if(typeName == "INPUT_PULLUP"){return 2;}
  if(typeName == "INPUT_PULLDOWN"){debugMsg("IO type incompatible with this MCU");return 0;} //not avalible in esp8266
  if(typeName == "OUTPUT_OPEN_DRAIN"){return 3;} //not sure on this value have to double check
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


float SpaceLeft(){
  FSInfo fs_info;
  
  float freeMemory = fs_info.totalBytes - fs_info.usedBytes;
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
  if (ioType == 0){return "INPUT";}
  if (ioType == 1){return "OUTPUT";}
  if (ioType == 2){return "INPUT_PULLUP";}
  if (ioType == 3){return "INPUT_PULLDOWN";}
  if (ioType == 4){return "OUTPUT_OPEN_DRAIN";}
}



void InitStorageSetup(){
  SPIFFS.begin();
  //do anything that might be needed related to file and storage on startup.
}

void debugMsg(String msg){
  debugMsgPrefx();
  Serial.println(msg);
}

void debugMsgPrefx(){
  Serial.print("DG:");
}
