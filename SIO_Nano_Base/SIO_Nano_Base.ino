//Simple Serial IO Driver
//Takes serial commands to control configured IO.
//Reports IO status created for use with OctoPrint_SIOControl_plugin

//the nano has very little variable memory this firmware uses a lot of strings for comm.  
//commentout includeDebug to ensure that after you have made any changes you have enough memory to be stable. 
//Note some messages have been shortened to save memory. Messages that start with "<" symbolize "Start" messages that start with ">" symbolize "End"
//ERR is short for Error
//val is short for value
//cmnd is short for command
//Reciently v1.0.0.6 I have made heavey use of the F String Macro to move strings to the Program space. This has helped alot. 
//There is plenty of program space so using the F macro seems to be the answer to the run time memory issue when using string. 
//At the time of this writing enabled is 46%(1104 bytes free) and disabled is 43%(1167 bytes free) free memrory for runtime
//I may in the near future just remove the includeDegbug define and the associated checks. 
#define includeDebug


#define VERSIONINFO "NanoSerialIO 1.0.6"
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
    debugMsg(F("< Saving IO Config"));
  #endif
  int cs = 0;
  for (int i=0;i<IOSize;i++){
    EEPROM.update(i+1,IOType[i]); //store IO type map in eeprom but only if they are different... Will make it last a little longer but unlikely to really matter.:) 
    cs += IOType[i];
    #ifdef includeDebug
      if(_debug){debugMsgPrefx();Serial.print(F("EE Pos:"));Serial.print(i);Serial.print(F(" typ:"));Serial.println(IOType[i]);Serial.print(F(" Cur CS:"));Serial.println(cs);}
    #endif
  }
  EEPROM.update(0,cs); 
  #ifdef includeDebug
    if(_debug){debugMsgPrefx();Serial.print(F("Chk-S:"));Serial.println(cs);}
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
    if(_debug){debugMsgPrefx();Serial.print(F("tmpCS: "));Serial.println(tempCS);}
  #endif    
  
  if(cs == tempCS){
    if(_debug){Serial.println("aply EE IO");}
    for (int i=0;i<IOSize;i++){
      IOType[i] = IOTTmp[i];
    }
    return;  
  }
  #ifdef includeDebug
  if(_debug){debugMsgPrefx();Serial.print(F("!:Using IO defaults"));Serial.println(tempCS);}
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
    debugMsg(F("< Types"));
  #endif
  
  FetchIOConfig();
  ConfigIO();
  reportIOTypes();
  
  #ifdef includeDebug 
    debugMsg(F("> Types"));
    _debug = true;
  #endif
  
  //need to get a baseline state for the inputs and outputs.
  for (int i=0;i<IOSize;i++){
    IO[i] = digitalRead(IOMap[i]);  
  }
  
  IOReport.set(100ul);
  Serial.println(F("RR")); //send ready for commands    
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
    Serial.print(F("IO:"));
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
        if(_debug){debugMsgPrefx();Serial.print(F("IO Chg: "));Serial.print(String(i));}
        #endif
        changed = true;
      }
    }
    
  }
    
  return changed;
}

void reportIOTypes(){
  Serial.print(F("IT:"));
  for (int i=0;i<IOSize;i++){
    Serial.print(String(IOType[i]));
    Serial.print(F(","));
  }
  Serial.println();
  #ifdef includeDebug 
    if(_debug){debugMsgPrefx();Serial.print(F("IOSize:")); Serial.println(String(IOSize));}
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
    
    if(sepPos !=-1){
      command = buf.substring(0,sepPos);
      value = buf.substring(sepPos+1);;
      
      #ifdef includeDebug
      if(_debug){
        debugMsg("cmnd:[" + command + "]");
        debugMsg("val:[" + value+ "]");
      }
      #endif 
    }else{
      command = buf;
      #ifdef includeDebug
      if(_debug){
        debugMsg("cmnd:[" + command + "]");
      }
      #endif
    }
    if(command.startsWith("N") || command == "M115"){
      ack();
      
      Serial.println(F("Looks like OctoPrint thinks I am a printer."));
      Serial.println(F("I not printer; Am SIOControler. Send disconnect host action cmnd."));
      #ifdef includeDebug
      Serial.println(F("To avoide this you should place the name/path of this port in the [Blacklisted serial ports] section of Octoprint"));
      Serial.println(F("This can be found in the settings dialog under 'Serial Connection'"));
      #endif
      Serial.println(F("//action:disconnect"));
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
      Serial.print(F("VI:"));
      Serial.println(VERSIONINFO);
      Serial.print(F("CP:"));
      Serial.println(COMPATIBILITY);
    }
    else if (command == "IC") { //io count.
      ack();
      Serial.print(F("IC:"));
      Serial.println(IOSize -1);
      return;
    }
    
    else if(command =="debug" && value.length() == 1){
      ack();
      #ifdef includeDebug
        if(value == "1"){
          _debug = true;
          debugMsg(F("Debug On"));
        }else{
          _debug=false;
          debugMsg(F("Debug Off"));
        }
      #else
      debugMsg(F("debug disabled"));
      #endif
      return;
    }
    else if(command=="CIO"){ //set IO Configuration
      ack();
      if(value.length() == 0){ //set IO Configuration
      if(_debug){
        #ifdef includeDebug
        debugMsg(F("ERR: val out of rng"));
        #endif
      }
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
        debugMsg(F("ERR: value out of range"));
        return;
      }
      
      if(value.indexOf(" ") >=1 && value.substring(value.indexOf(" ")+1).length()==1){//is there at least one character for the IO number and the value for this IO point must be only one character (1||0)
        String sIOPoint = value.substring(0,value.indexOf(" "));
        String sIOSet = value.substring(value.indexOf(" ")+1); // leaving this as a string allows for easy check on correct posible values
        #ifdef includeDebug
        if(_debug){
          debugMsg("IO#:"+ sIOPoint);
          debugMsg("GPIO#:"+ String(IOMap[sIOPoint.toInt()]));
          debugMsg("Set to:"+ sIOSet);
        }
        #endif

        //checks to see if the IO point is not a number if the string is not a zero but the number is that means it was not a number
        if(isOutPut(sIOPoint.toInt()) && !(sIOPoint.toInt() == 0 && sIOPoint != "0") ){
          if(sIOSet == "0" || sIOSet == "1"){// need to make sure it is a valid IO point value 
            if(sIOSet == "1"){
              digitalWrite(IOMap[sIOPoint.toInt()],HIGH);
            }else{
              digitalWrite(IOMap[sIOPoint.toInt()],LOW);
            }
          }else{
            debugMsg(F("ERR: Atmpt set IO value not valid"));   
          }
        }else{
          debugMsg(F("ERR: Atmpt set IO not an output"));   
        }
        //delay(200); // give it a moment to finish changing the 
        reportIO(true);
      }else{
        debugMsg(F("ERR: invalid cmd/vlu"));   
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
          debugMsgPrefx();Serial.print(F("ART now:"));Serial.println(String(reportInterval));
        }else{
          debugMsg(F("ERR: # out of range."));
        }
        return;
      }
      debugMsg(F("ERR: # out of range."));
      return; 
    }
    //Enable event trigger reporting Mostly used for E-Stop
    else if(command == "SE"){
      ack();
      if(value.length() == 1){
        if(value == "1" || value == "0"){
          EventTriggeringEnabled = value.toInt();
        }else{
          debugMsg(F("EROR: val out of range"));
        }
      }else{
        debugMsg(F("EROR: val bad format"));
      }
      return;
    }
    else if (command == "restart" || command == "reset" || command == "reboot"){ //could remove this completely to save some memory
      ack();
      debugMsg(F("!supported"));
    }

    //Get States 
    else if(command == "GS"){
      ack();
      reportIO(true);
      return; 
    }
   
    else{
      debugMsgPrefx();Serial.print(F("ERR: bad cmnd["));Serial.print(command);Serial.println(F("]"));
    }
  }
}


void ack(){
  Serial.println(F("OK"));
}

bool validateNewIOConfig(String ioConfig){
  
  if(ioConfig.length() != IOSize){
    #ifdef includeDebug
      if(_debug){debugMsg(F("IOConfig validation failed(Wrong len)"));}
    #endif
    return false;  
  }

  for (int i=0;i<IOSize;i++){
    int pointType = ioConfig.substring(i,i+1).toInt();
    if(pointType > 4){//cant be negative. we would have a bad parse on the number so no need to check negs
      #ifdef includeDebug
      if(_debug){
        debugMsgPrefx();Serial.println(F("IO validation failed"));
        debugMsgPrefx();Serial.print(F("Bad IO Type: index["));Serial.print(i);Serial.print(F("] type["));Serial.print(pointType);Serial.println(F("]"));
      }
      #endif
      return false;
    }
  }
  #ifdef includeDebug
  if(_debug){debugMsg(F("IOConfig validation good"));}
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


void debugMsg(String msg){
  debugMsgPrefx();
  Serial.println(msg);
}
 
void debugMsgPrefx(){
  Serial.print(F("DG:"));
}
