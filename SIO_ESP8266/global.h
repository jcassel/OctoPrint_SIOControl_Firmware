#include "FS.h"
#include <ArduinoJson.h> 

//gpio 0,2 and, 15 presents some challanges for use as an IO point. You can use at least 2 of them for IO but you must do it in the specific way needed for the specific pin.
//These are not including in the IO list here due to this challange.
//this link has some details on how to use the IO points 0,2 and 15 as inputs or outputs
//https://www.instructables.com/ESP8266-Using-GPIO0-GPIO2-as-inputs/

#define IO0 LED_BUILTIN //Active LOW. Ya this is on the chip.. good test IO Point to play with. Blue LED on MCU. Also Programing Switch. attach to ground on start up. Posts are on same line as the UART interface.
#define IO1 5  // Relay1 has relay and red led D1 on board near power in terminal
#define IO2 4  //  Relay1 has relay and red led D4 on board near power in terminal
#define IO3 16 // has blue LED on board D7 Active LOW. Also has post on header next to chip
#define IO4 14 // post on header next to chip  
#define IO5 12 // post on header next to chip
#define IO6 13 // post on header next to chip

 
  
//#define IOA A0//Analog 0 


#define IOSize  7
bool _debug = false;

int IOType[IOSize]{OUTPUT,OUTPUT,OUTPUT,OUTPUT,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP};
int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6};
String IOSMap[IOSize] {"IO0","IO1","IO2","IO3","IO4","IO5","IO6"};
int IO[IOSize];
Bounce Bnc[IOSize];
bool EventTriggeringEnabled = 1;


void debugMsgPrefx(){
  Serial.print("DG:");
}

void debugMsg(String msg){
  debugMsgPrefx();
  Serial.println(msg);
}

bool isOutPut(int IOP){
  return (IOType[IOP] == OUTPUT);
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

String getIOTypeString(int ioType){
  if (ioType == 0){return "INPUT";}
  if (ioType == 1){return "OUTPUT";}
  if (ioType == 2){return "INPUT_PULLUP";}
  if (ioType == 3){return "INPUT_PULLDOWN";}
  if (ioType == 4){return "OUTPUT_OPEN_DRAIN";}
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
