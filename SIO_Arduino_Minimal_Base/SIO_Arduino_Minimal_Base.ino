//Note that this sketch is too large due to some of the Serial String use for an Arduino Nano.
  

#define VERSIONINFO "SIO_Arduino_General"
#define COMPATIBILITY "SIOPlugin 0.1.1"
#include "TimeRelease.h"
#include <Bounce2.h>



#define IOSize  19 //number should be one more than the IO# :) (must include the idea of Zero)  
//Outputs
#define IO0 3  
#define IO1 4
#define IO2 5
#define IO3 6
#define IO4 7
#define IO5 8
#define IO6 9
#define IO7 10
#define IO8 11
#define IO9 12
#define IO10 13
//Inputs
#define IO11 A0
#define IO12 A1
#define IO13 A2 
#define IO14 A3
#define IO15 A4
#define IO16 A5
#define IO17 A6
#define IO18 A7 

bool _debug = false;

int IOType[IOSize]{OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT };
int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6,IO7,IO8,IO9,IO10,IO11,IO12,IO13,IO14,IO15,IO16,IO17,IO18};
String IOSMap[IOSize] {"IO0","IO1","IO2","IO3","IO4","IO5","IO6","IO7","IO8","IO9","IO10","IO11","IO12","IO13","IO14","IO15","IO16","IO17","IO18"};
int IO[IOSize];
Bounce Bnc[IOSize];
bool EventTriggeringEnabled = 1;
 
//void(* resetFunc) (void) = 0;
 
bool isOutPut(int IOP){
  return IOType[IOP] == OUTPUT; 
}
 
void ConfigIO(){
  
  Serial.println("Setting IO");
  for (int i=0;i<IOSize;i++){
    if(i > 10){ //if it is an input
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
  //debugMsgPrefx();Serial.print("INPUT_PULLDOWN:");Serial.println(INPUT_PULLDOWN);//9
  //debugMsgPrefx();Serial.print("OUTPUT_OPEN_DRAIN:");Serial.println(OUTPUT_OPEN_DRAIN);//18
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
      debugMsg("Live IO confiuration feature is only partially supported.");
      if (validateNewIOConfig(value)){
        updateIOConfig(value);
      }
      return;
    }
    
    else if(command=="SIO"){
      ack();
      debugMsg("Live IO confiuration feature is not avalible in this firmware");
      debugMsg("To change IO configuration you must edit and update firmware");
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
      ack();
      debugMsg("Command not supported.");
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

//Because there is no way to store between uses, know that chaneges will not survive restarts.
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


//Note that this list and values must be updated to match the expected for your MCU 
int getIOType(String typeName){
  if(typeName == "INPUT"){return 1;}
  if(typeName == "OUTPUT"){return 2;}
  if(typeName == "INPUT_PULLUP"){return 5;}
  //if(typeName == "INPUT_PULLDOWN"){return 9;}
  //if(typeName == "OUTPUT_OPEN_DRAIN"){return 18;} //not sure on this value have to double check
  Serial.print("ERROR: unrecognized IOType name IOType["+typeName+"]");
  return 0;
}


//Note that this list and values must be updated to match the expected for your MCU  
String getIOTypeString(int ioType){
  if (ioType == 1){return "INPUT";}
  if (ioType == 2){return "OUTPUT";}
  if (ioType == 5){return "INPUT_PULLUP";}
  //if (ioType == 9){return "INPUT_PULLDOWN";}
  //if (ioType == 18){return "OUTPUT_OPEN_DRAIN";}
  Serial.print("ERROR: unrecognized IOType value[");Serial.print(ioType);Serial.println("]");
  return "";
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
 
void debugMsg(String msg){
  debugMsgPrefx();
  Serial.println(msg);
}
 
void debugMsgPrefx(){
  Serial.print("DG:");
}
