#include "FS.h"
#include <ArduinoJson.h> 





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

#define OUTPUT_PWM -2 //Output Type For PWM
#define INPUT_DHT -3 //Input Type for DHT Module Reading. 


#include "DHTesp.h" //https://github.com/beegee-tokyo/DHTesp/tree/master
#define DHTSensor DHTesp::DHT22
#define dhtSize 4
int dhtSizeDynamic = dhtSize; //This will be dynamic up to 4. actual count of usage is what will determin if a measurement will be made.
DHTesp dht_sensor[dhtSize]; //allow up to 4 of these sensors to be used
TempAndHumidity DTHValues[dhtSize];


#define IOSize  14 //number should be one more than the IO# :) (must include the idea of Zero) 
bool _debug = false;
int IOType[IOSize]{INPUT,INPUT,INPUT,INPUT,INPUT_DHT,INPUT_DHT,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,OUTPUT,OUTPUT,OUTPUT};
int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6,IO7,IO8,IO9,IO10,IO11,IO12,IO13};
String IOSMap[IOSize] {"IO0","IO1","IO2","IO3","IO4","IO5","IO6","IO7","IO8","IO9","IO10","IO11","IO12","IO13"};
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
  return (IOType[IOP] == OUTPUT || IOType[IOP] == OUTPUT_OPEN_DRAIN || IOType[IOP] == OUTPUT_PWM);
}

bool isINPUT(int IOP){
  return (IOType[IOP] == INPUT)||(IOType[IOP] == INPUT_PULLUP)||(IOType[IOP] == INPUT_PULLDOWN);
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
  if (ioType == INPUT){return "INPUT";}
  if (ioType == OUTPUT){return "OUTPUT";}
  if (ioType == INPUT_PULLUP){return "INPUT_PULLUP";}
  if (ioType == INPUT_PULLDOWN){return "INPUT_PULLDOWN";}
  if (ioType == OUTPUT_OPEN_DRAIN){return "OUTPUT_OPEN_DRAIN";}
  
  if (ioType == OUTPUT_PWM){
    #ifdef ENABLE_PWM_SUPPORT
    return "OUTPUT_PWM";
    #else
    return "UnSupported OUTPUT_PWM";
    #endif
    }
    
  if (ioType == INPUT_DHT){
    return "INPUT_DHT";
  }
    
  
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
