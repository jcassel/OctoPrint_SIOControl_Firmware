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

#define IO0 34  //  Input Only (Must configure(in code) without pullup or down. These inputs do not have these on chip)
#define IO1 35  //  Input Only (Must configure(in code) without pullup or down. These inputs do not have these on chip)
#define IO2 36  //  Input Only (Must configure(in code) without pullup or down. These inputs do not have these on chip)
#define IO3 39  //  input only (Must configure(in code) without pullup or down. These inputs do not have these on chip)
#define IO4 0   //  general IO Important note.. if this is in a low state on boot of the device, it will go into programing mode.
#define IO5 5   //  general IO Important note.. Will output a PWM signal on boot. Should not have any affect if configured as an input.
#define IO6 12  //  general IO Important note.. boot will failif this is pulled high during boot.
#define IO7 13  //  general IO touch capable
#define IO8 14  //  general IO Important note.. Will output a PWM signal on boot. Should not have any affect if configured as an input.
#define IO9 16  //  general IO (RXD 2)
#define IO10 17 //  general IO (TXD 2)
#define IO11 26 //  Relay K1
#define IO12 25 //  Relay K2
#define IO13 33 //  Relay K3
#define IO14 32 //  Relay K4
#define IO15 23 //  output only LED 

#define OUTPUT_PWM -2 //Output Type For PWM

#define IOSize  16 //number should be one more than the IO# :) (must include the idea of Zero) 
bool _debug = false;
int IOType[IOSize]{INPUT,INPUT,INPUT,INPUT,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,OUTPUT,OUTPUT,OUTPUT,OUTPUT,OUTPUT};
int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6,IO7,IO8,IO9,IO10,IO11,IO12,IO13,IO14,IO15};
String IOSMap[IOSize] {"IO0","IO1","IO2","IO3","IO4","IO5","IO6","IO7","IO8","IO9","IO10","IO11","IO12","IO13","IO14","IO15"};
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
  Serial.println("************************review this for ESP32*********************");
  for (int i=2;i<IOSize;i++){//start at 2 to avoid D0 and D1 
    int nIOC = newIOConfig.substring(i,i+1).toInt();
    if(IOType[i] != nIOC){
      IOType[i] = nIOC;
      if(nIOC == OUTPUT ||nIOC == OUTPUT_OPEN_DRAIN){
        digitalWrite(IOMap[i],LOW); //set outputs to low since they will be high coming from INPUT_PULLUP
      }
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
