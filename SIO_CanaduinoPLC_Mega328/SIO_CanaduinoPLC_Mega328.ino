//Simple Serial IO Driver for the CanaduinoPLC board
//This is based on the Nanno micro controller so it has the same limitations associated. 

//the nano has very little variable memory this firmware uses a lot of strings for comm.  
//use includeDebug to ensure that after you have made any changes you have enough memory to be stable. 
//Note some messages have been shortened to save memory. Messages that start with "<" symbolize "Start" messages that start with ">" symbolize "End"
//At the time of this writing enabled is 75% and disabled is 51% free memrory for runtime
//#define includeDebug

#define VERSIONINFO "CanaduinoPLCSerialIO 1.0.7"
#define COMPATIBILITY "SIOPlugin 0.1.1"
#include "TimeRelease.h"
#include <Bounce2.h>
#include <EEPROM.h>


#define IO0 7     //(Input D1)
#define IO1 8     //(Input D2)
#define IO2 12   //(Input D3)
#define IO3 13   //(Input D4) BuiltIn LED (you should have been removed it from the nano board)

#define IO4 2     //(Output Relay1)
#define IO5 3     //(Output Relay2)
#define IO6 4     //(Output Relay3)
#define IO7 5     //(Output Relay4)
#define IO8 A2   //(Output Relay5)
#define IO9 A3   //(Output Relay6)


//Unconfigured/unused in this example (Not included in the report array.)
#define IO10 2     // pin2/D0  //cant be used for this... used as TX on the comm port
#define IO11 1     // pin1/D1  //cant be used for this... used as RX on the comm port
#define IO12 6     //(Output A1 [analog])
#define IO13 9     //(Output A2 [analog])
#define IO14 10   //(Output A3 [analog])
#define IO15 11   //(Output A4 [analog])
#define IO16 A0   //(Input A1 [analog])
#define IO17 A1   //(Input A2 [analog])
#define IO18 A6   //(Input A3 [analog])
#define IO19 A7   //(Input A4 [analog])
#define IO20 A4   //(i2c SDA)
#define IO21 A5   //(i2c SCL) 
//Arduino Documentation also warns that Any input that it floating is suseptable to random readings. Best way to solve this is to use a pull up(avalible as an internal) or down resister(external).
#define IOSize  10 //(0-9)

bool _debug = false;
int IOType[IOSize]{INPUT_PULLUP,INPUT,INPUT,INPUT,OUTPUT,OUTPUT,OUTPUT,OUTPUT,OUTPUT,OUTPUT}; //0-9
int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6,IO7,IO8,IO9};
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
    if(IOType[i] == 0 ||IOType[i] == 2){ //if it is an input
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
  DisplayIOTypeConstants();
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


void DisplayIOTypeConstants(){
  debugMsgPrefx();Serial.print("INPUT:");Serial.println(INPUT);//1
  debugMsgPrefx();Serial.print("OUTPUT:");Serial.println(OUTPUT);//2
  debugMsgPrefx();Serial.print("INPUT_PULLUP:");Serial.println(INPUT_PULLUP);//5
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
    
    if(sepPos !=-1){
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
    
    else if(command =="debug" && value.length() == 1){
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
}




void debugMsg(String msg){
  debugMsgPrefx();
  Serial.println(msg);
}
 
void debugMsgPrefx(){
  Serial.print("DG:");
}
