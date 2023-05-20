//Simple Serial IO Driver
//Takes serial commands to control configured IO.
//Reports IO status created for use with OctoPrint_SIOControl_plugin

//the nano has very little variable memory this firmware uses a lot of strings for comm.  
//use includeDebug to ensure that after you have made any changes you have enough memory to be stable. 
//Note some messages have been shortened to save memory. Messages that start with "<" symbolize "Start" messages that start with ">" symbolize "End"
//At the time of this writing enabled is 82% and disabled is 58% free memrory for runtime
#define includeDebug


#define VERSIONINFO "NanoSerialIO 1.0.5"
#define COMPATIBILITY "SIOPlugin 0.1.1"
#include "TimeRelease.h"
#include <Bounce2.h>
#include <EEPROM.h>

#define IO0 2   // pin5/D5
#define IO1 3   // pin6/D3
#define IO2 4   // pin7/D4
#define IO3 5   // pin8/D5
#define IO4 6   // pin9/D6
#define IO5 7  // pin10/D7
#define IO6 8  // pin11/D8
#define IO7 9  // pin12/D9
#define IO8 10  // pin13/D10
#define IO9 11  // pin14/D11
#define IO10 12  // pin15/D12
#define IO11 13 // pin16/D13(LED)* if you want to use this as an input, you will need to remove the led from the MCU board.

#define IO12 A0 // pin19/A0  
#define IO13 A1 // pin20/A1
#define IO14 A2 // pin21/A2
#define IO15 A3 // pin22/A3
#define IO16 A4 // pin23/A4
#define IO17 A5 // pin24/A5
//Note that A6 and A7 can only be used as Analog inputs. 
//Arduino Documentation also warns that Any input that it floating is suseptable to random readings. Best way to solve this is to use a pull up(avalible as an internal) or down resister(external).I set all internals to pull up see IOTypes


#define IOSize  18 //(0-19)note that this equates to 18 bacause of D0andD1 as Serial.
bool _debug = false;
int IOType[IOSize]{INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,OUTPUT,OUTPUT,OUTPUT,OUTPUT,OUTPUT,OUTPUT}; //0-19
int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6,IO7,IO8,IO9,IO10,IO11,IO12,IO13,IO14,IO15,IO16,IO17};
int IO[IOSize];
Bounce Bnc[IOSize];
bool EventTriggeringEnabled = 1;


void StoreIOConfig(){
  #ifdef includeDebug
    debugMsg("< Saving IO Config");
  #endif
  int cs = 0;
  for (int i=0;i<IOSize;i++){
    EEPROM.update(i+1,IOType[i]); //store IO type map in eeprom but only if they are different... Will make it last a little longer but unlikely to really matter.:) 
    cs += IOType[i];
    #ifdef includeDebug
      if(_debug){debugMsgPrefx();Serial.print("EE Pos:");Serial.print(i);Serial.print(" typ:");Serial.println(IOType[i]);Serial.print(" Cur CS:");Serial.println(cs);}
    #endif
  }
  EEPROM.update(0,cs); 
  #ifdef includeDebug
    if(_debug){debugMsgPrefx();Serial.print("Chk-S:");Serial.println(cs);}
  #endif
}


void FetchIOConfig(){
  int cs = EEPROM.read(0); //specifics of number is completely random. 
  int tempCS = 0;
  int IOTTmp[IOSize];
  

  for (int i=0;i<IOSize;i++){ //starting at 2 to avoide reading in first 2 IO points.
    IOTTmp[i] = EEPROM.read(i+1);//retreve IO type map from eeprom. 
    tempCS += IOTTmp[i];
  }

  #ifdef includeDebug
    if(_debug){debugMsgPrefx();Serial.print("tmpCS: ");Serial.println(tempCS);}
  #endif    
  
  if(cs == tempCS){
    if(_debug){Serial.println("aply EE IO");}
    for (int i=0;i<IOSize;i++){
      IOType[i] = IOTTmp[i];
    }
    return;  
  }
  #ifdef includeDebug
  if(_debug){debugMsgPrefx();Serial.print("!:Using IO defaults");Serial.println(tempCS);}
  #endif
}


bool isOutPut(int IOP){
  return IOType[IOP] == OUTPUT; 
}

void ConfigIO(){
  #ifdef includeDebug
    debugMsg("< Set IO");
  #endif
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
TimeRelease IOTimer[9];
unsigned long reportInterval = 3000;



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(300);
  #ifdef includeDebug
    _debug = true;
  #endif
  
  debugMsg(VERSIONINFO);
  
  #ifdef includeDebug 
    debugMsg("< Types");
  #endif
  
  FetchIOConfig();
  ConfigIO();
  reportIOTypes();
  
  #ifdef includeDebug 
    debugMsg("> Types");
    _debug = true;
  #endif
  
  //need to get a baseline state for the inputs and outputs.
  for (int i=0;i<IOSize;i++){
    IO[i] = digitalRead(IOMap[i]);  
  }
  
  IOReport.set(100ul);
  Serial.println("RR"); //send ready for commands    
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
    
  }
  
}
//*********************End loop***************************//

void reportIO(bool forceReport){
  if (IOReport.check()||forceReport){
    Serial.print("IO:");
    for (int i=0;i<IOSize;i++){
      IO[i] = digitalRead(IOMap[i]);  
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
       #ifdef includeDebug 
       if(_debug){debugMsg("IO Chg: "+String(i));}
       #endif
      }
    }else{
      
      //is the current state of this output not the same as it was on last report.
      //this really should not happen if the only way an output can be changed is through Serial commands.
      //the serial commands force a report after it takes action.
      if(IO[i] != digitalRead(IOMap[i])){
        #ifdef includeDebug 
        if(_debug){debugMsg("IO Chg: "+String(i));}
        #endif
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
  }
  Serial.println();
  #ifdef includeDebug 
    if(_debug){debugMsg("IOSize:" + String(IOSize));}
  #endif
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
      
      #ifdef includeDebug
      if(_debug){
        debugMsg("command:[" + command + "]");
        debugMsg("value:[" + value+ "]");
      }
      #endif 
    }else{
      command = buf;
      #ifdef includeDebug
      if(_debug){
        debugMsg("command:[" + command + "]");
      }
      #endif
    }
    if(command.startsWith("N") || command == "M115"){
      ack();
      
      Serial.println("Recieved a command that looks like OctoPrint thinks I am a printer.");
      Serial.println("This is not a printer it is a SIOControler. Sending disconnect host action command.");
      #ifdef includeDebug
      Serial.println("To avoide this you should place the name/path of this port in the [Blacklisted serial ports] section of Octoprint");
      Serial.println("This can be found in the settings dialog under 'Serial Connection'");
      #endif
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
      #ifdef includeDebug
        if(value == "1"){
          _debug = true;
          debugMsg("Serial debug On");
        }else{
          _debug=false;
          debugMsg("Serial debug Off");
        }
      #else
      debugMsg("debug not enabled");
      #endif
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
      #ifdef includeDebug
      if(_debug){
        debugMsg("IO#:"+ String(IOMap[IOPoint]));
        debugMsg("Set to:"+ String(IOSet));
      }
      #endif
      if(isOutPut(IOPoint)){
        if(IOSet == 1){
          digitalWrite(IOMap[IOPoint],HIGH);
        }else{
          digitalWrite(IOMap[IOPoint],LOW);
        }
      }else{
        debugMsg("ERR:IO not output");   
      }
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
          #ifdef includeDebug
          if(_debug){
            debugMsg("Report interval now:" +String(reportInterval));
          }
          #endif
        }else{
          debugMsg("Min val 500");
        }
        return;
      }
      debugMsg("ERROR: SI Format");
      return; 
    }
    //Enable event trigger reporting Mostly used for E-Stop
    else if(command == "SE" && value.length() > 0){
      ack();
      EventTriggeringEnabled = value.toInt();
      return;
    }
    else if (command == "restart" || command == "reset" || command == "reboot"){ //could remove this completely to save some memory
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
      debugMsg("ERR: Unrecognized command["+command+"]");
    }
  }
}


void ack(){
  Serial.println("OK");
}

bool validateNewIOConfig(String ioConfig){
  
  if(ioConfig.length() != IOSize){
    #ifdef includeDebug
      if(_debug){Serial.println("IOConfig validation failed(Wrong len)");}
    #endif
    return false;  
  }

  for (int i=0;i<IOSize;i++){
    int pointType = ioConfig.substring(i,i+1).toInt();
    if(pointType > 4){//cant be negative. we would have a bad parse on the number so no need to check negs
      #ifdef includeDebug
      if(_debug){
        Serial.println("IO validation failed");Serial.print("Bad IO Type: index[");Serial.print(i);Serial.print("] type[");Serial.print(pointType);Serial.println("]");
      }
      #endif
      return false;
    }
  }
  #ifdef includeDebug
  if(_debug){Serial.println("IOConfig validation good");}
  #endif
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
  if(typeName == "INPUT_PULLDOWN"){return 3;}
  if(typeName == "OUTPUT_OPEN_DRAIN"){return 4;} //not sure on this value have to double check
}

bool strToUnsignedLong(String& data, unsigned long& result) {
  data.trim();
  long tempResult = data.toInt();
  if (String(tempResult) != data) { // check toInt conversion
    return false;
  } else if (tempResult < 0) { //  OK check sign
    return false;
  } 
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
