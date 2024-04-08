//#define includeDebug
#define VERSIONINFO "SIO_Arduino_General 1.0.11"
#define COMPATIBILITY "SIOPlugin 0.1.1"
#define FLASHSIZE "NA"

#include "TimeRelease.h"
#include <Bounce2.h>
#include <EEPROM.h>


bool _debug = false;

#ifdef __AVR_ATmega2560__
  #define IOSize  19 //(generic arduino board with mega2560) 
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
  #define IO11 A0
  #define IO12 A1
  #define IO13 A2 
  #define IO14 A3
  #define IO15 A4
  #define IO16 A5
  #define IO17 A6
  #define IO18 A7 
  const int IOTYPE_DEFAULT[IOSize]{OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT };
  int IOType[IOSize]{OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT };
  int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6,IO7,IO8,IO9,IO10,IO11,IO12,IO13,IO14,IO15,IO16,IO17,IO18};
  String IOSMap[IOSize] {"IO0","IO1","IO2","IO3","IO4","IO5","IO6","IO7","IO8","IO9","IO10","IO11","IO12","IO13","IO14","IO15","IO16","IO17","IO18"};  

#elif __AVR_ATmega328P__
  #define IOSize  18 
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
  const int IOTYPE_DEFAULT[IOSize]{INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,OUTPUT,OUTPUT,OUTPUT,OUTPUT,OUTPUT,OUTPUT}; //0-17
  int IOType[IOSize]{INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,OUTPUT,OUTPUT,OUTPUT,OUTPUT,OUTPUT,OUTPUT}; //0-17
  int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6,IO7,IO8,IO9,IO10,IO11,IO12,IO13,IO14,IO15,IO16,IO17};
  String IOSMap[IOSize] {"IO0","IO1","IO2","IO3","IO4","IO5","IO6","IO7","IO8","IO9","IO10","IO11","IO12","IO13","IO14","IO15","IO16","IO17"};  
 
#else //some well known defined boards are (__AVR_ATmega16U4__, __AVR_ATmega1280__, __AVR_ATmega168__, __AVR_ATmega32U4__) 
  Error(F("Unknown/unconfigured board")); 
  Will need to remove this if this is the board you have see how the other boards are setup and create a setup you need for your board.  
  Please if you add support for a new board here, either create a pull request or put the details in an issue request and I will add it to the firmware. 
#endif


int IO[IOSize];
Bounce Bnc[IOSize];
bool EventTriggeringEnabled = 1;


bool RemoveIOSettings(){
  //updating IO Settings in EProm to be defaults.
  for (int i=0;i<IOSize;i++){
    IOType[i] = IOTYPE_DEFAULT[i];
  }
  StoreIOConfig(); 
  ConfigIO();
  return true;
  
}


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
  //return (IOType[IOP] == OUTPUT || IOType[IOP] == OUTPUT_PWM);
  return (IOType[IOP] == OUTPUT);
}

bool isINPUT(int IOP){
  return (IOType[IOP] == INPUT)||(IOType[IOP] == INPUT_PULLUP);
}



void ConfigIO(){
  #ifdef includeDebug
    debugMsg(F("< Set IO"));
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
TimeRelease ReadyForCommands;
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
  ReadyForCommands.set(reportInterval); //Initial short delay resets will be at 3x
  //Serial.println(F("RR")); //send ready for commands    
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
    if(isINPUT(i)){
      Bnc[i].update();
      if(Bnc[i].changed()){
       changed = true;
       IO[i]=Bnc[i].read();
       if(_debug){debugMsgPrefx();Serial.print(F("Input Changed: "));Serial.println(i);}
      }
    }else if(isOutPut(i)){
      //is the current state of this output not the same as it was on last report.
      //this really should not happen if the only way an output can be changed is through Serial commands.
      //the serial commands force a report after it takes action.
      if(IO[i] != digitalRead(IOMap[i])){
        if(_debug){debugMsgPrefx();Serial.print(F("Output Changed: "));Serial.println(i);}
        changed = true;
      }
    //}else if(IOType[i] == INPUT_DHT || IOType[i] == OUTPUT_PWM ){
      //not checking digital change on this type of IO
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

void DisplayIOTypeConstants(){
  Serial.print(F("TC "));
  Serial.print(F("INPUT:"));Serial.print(INPUT);Serial.print(",");//0
  Serial.print(F("OUTPUT:"));Serial.print(OUTPUT);Serial.print(",");//1

  Serial.print(F("INPUT_PULLUP:"));Serial.println(INPUT_PULLUP);//2  NOTE*********fix the new line if you add additional response types.
  //Serial.print(F("INPUT_PULLUP:"));Serial.print(INPUT_PULLUP);Serial.print(",");//5
  //Serial.print("INPUT_PULLDOWN:");Serial.print(INPUT_PULLDOWN);Serial.print(",");//9
  //Serial.print("OUTPUT_OPEN_DRAIN:");Serial.print(OUTPUT_OPEN_DRAIN);Serial.print(",");//18
  //Serial.print("INPUT_DHT:");Serial.print(INPUT_DHT);Serial.print(",");//-3
  //Serial.print("OUTPUT_PWM:");Serial.println(OUTPUT_PWM);//-2 
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
        debugMsgPrefx();Serial.print(F("cmnd:["));Serial.print(command);Serial.println(F("]"));
        debugMsgPrefx();Serial.print(F("val:["));Serial.print(value);Serial.println(F("]"));
      }
      #endif 
    }else{
      command = buf;
      #ifdef includeDebug
      if(_debug){
        debugMsgPrefx();Serial.print(F("cmnd:["));Serial.print(command);Serial.println(F("]"));
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
      ack();
      Serial.print(F("VI:"));
      Serial.println(VERSIONINFO);
      Serial.print(F("CP:"));
      Serial.println(COMPATIBILITY);
      Serial.print("FS:");
      Serial.println(FLASHSIZE);
      Serial.print("XT BRD:");
      #ifdef __AVR_ATmega1280__
        Serial.print("ATmega1280");
      #elif __AVR_ATmega2560__
        Serial.println("ATmega2560");
      #elif __AVR_ATmega328P__
        Serial.println("ATmega328P");
      #elif __AVR_ATmega168__
        Serial.println("ATmega168");
      #elif __AVR_ATmega32U4__
        Serial.println("ATmega32U4");
      #elif __AVR_ATmega16U4__
        Serial.println("ATmega16U4");
      #else
        Serial.println(F("Unknown board"));
      #endif
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
    else if(command == "TC"){
      ack();
      DisplayIOTypeConstants();
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
          debugMsgPrefx();Serial.print(F("IO#:"));Serial.println(sIOPoint);
          debugMsgPrefx();Serial.print(F("GPIO#:"));Serial.println(IOMap[sIOPoint.toInt()]);
          debugMsgPrefx();Serial.print(F("Set to:"));Serial.println(sIOSet);
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
    else if(command == "FS") {
      ack();
      if (value == "FormatSPIFFS"){
        debugMsg(F("FormatSPIFFS is Not Supported IN Ardino"));
      }else if(value == "RemoveSavedIOSettings"){
        bool result = RemoveIOSettings();
        if(result){debugMsg(F("IOSettings Remove Success"));}else{debugMsg(F("IOSettings Remove Failed"));}    
      }
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
  int newConfig[IOSize];
  int incomingSize = 0;
  
  if(ioConfig.length() == (IOSize*3+1)){
    debugMsgPrefx();
    Serial.print(F("IOConfig validation failed(Wrong len): required minimal len["));
    Serial.print((IOSize*3+1));
    Serial.print(F("] supplied len["));
    Serial.print(ioConfig.length());
    Serial.println("]" );
    return false;  
  }

  for(int i=0;i<IOSize;i++){
    int istart = i*3;
    String token = ioConfig.substring(istart,istart+2);
    if(_debug){debugMsgPrefx();Serial.print(F("Token: "));Serial.println(token.toInt());}
    newConfig[i] = token.toInt();
    if(!isValidIOType(newConfig[i])){
      debugMsgPrefx();Serial.print(F("IO Type is invalid: ["));Serial.print(token.toInt());Serial.print(F("] @ position:["));Serial.print(i);Serial.println(F("]"));
      return false;
    }
    incomingSize = i;
  }

  if(incomingSize+1 != IOSize){
    debugMsg(F("IOConfig validation failed. IO pattern did not have the correct number of point configs."));
    debugMsg("IO Count: " + String(IOSize));
    debugMsg("Parttern Count: " + String(incomingSize));
    return false;
  }

  if(_debug){
    debugMsg(F("New Config passed Validation"));
    debugMsgPrefx();
    for(int i=0;i< IOSize;i++){
      Serial.print("[");Serial.print(newConfig[i]);Serial.print("],");
    }
    Serial.println("");
  }

 return true; //seems its a good set of point Types.
 
}

//newIOConfig format is "##,##,##,##,##..." where each ## should be a 2 char string 
//representing an integer with a leading zero where needed. A negative number like -3 does not need a leading zero.  
//Where as a number like 1 or  5 should belike this 01 05
void updateIOConfig(String newIOConfig){

 for(int i=0;i<IOSize;i++){
    int istart = i*3;
    String token = newIOConfig.substring(istart,istart+2);
    IOType[i] = token.toInt();
    if(isOutPut(i)){
      digitalWrite(IOMap[i],LOW); //set outputs to low since they will be high coming from INPUT_PULLUP
    }
  }
}

String getIOTypeString(int ioType){
  if (ioType == INPUT){return F("INPUT");}
  if (ioType == OUTPUT){return F("OUTPUT");}
  if (ioType == INPUT_PULLUP){return F("INPUT_PULLUP");}
  //if (ioType == INPUT_PULLDOWN){return F("INPUT_PULLDOWN");}
  //if (ioType == OUTPUT_OPEN_DRAIN){return F("OUTPUT_OPEN_DRAIN");}
  //if (ioType == OUTPUT_PWM){return F("OUTPUT_PWM");}
  //if (ioType == INPUT_DHT){return F("INPUT_DHT");}
  
  return "UnSupported Type: " + String(ioType);
  
}


bool isValidIOType(int ioType){
  if(ioType == INPUT){return true;}
  if(ioType == OUTPUT){return true;}
  if(ioType == INPUT_PULLUP){return true;}
  //if(ioType == INPUT_PULLDOWN){return true;}
  //if(ioType == OUTPUT_OPEN_DRAIN){return true;}
  //if(ioType == OUTPUT_PWM){return true;}
  //if(ioType == INPUT_DHT){return true;}
  return false;
}


int getIOType(String typeName){
  if(typeName == "INPUT"){return INPUT;}
  if(typeName == "OUTPUT"){return OUTPUT;}
  if(typeName == "INPUT_PULLUP"){return INPUT_PULLUP;}
  //if(typeName == "INPUT_PULLDOWN"){return INPUT_PULLDOWN;}
  //if(typeName == "OUTPUT_OPEN_DRAIN"){return OUTPUT_OPEN_DRAIN;} //not sure on this value have to double check
  //if(typeName == "OUTPUT_PWM"){return OUTPUT_PWM;} //not sure on this value have to double check
  //if(typeName == "INPUT_DHT"){return INPUT_DHT;} //not sure on this value have to double check
  return -1;
}





void debugMsg(String msg){
  debugMsgPrefx();
  Serial.println(msg);
}
 
void debugMsgPrefx(){
  Serial.print(F("DG:"));
}
