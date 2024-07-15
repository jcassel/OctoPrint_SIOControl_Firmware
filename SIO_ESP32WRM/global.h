#include "FS.h"
#include <ArduinoJson.h> 


bool _debug = false;


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

#ifdef WMR2
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
#endif
#ifdef WMR4
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
#endif

#define OUTPUT_PWM -2 //Output Type For PWM
#define INPUT_DHT -3 //Input Type for DHT Module Reading. 

#include "DHTesp.h" //https://github.com/beegee-tokyo/DHTesp/tree/master
#define DHTSensor DHTesp::DHT22
#define DHTSize 4
int dhtSizeDynamic = DHTSize; //This will be dynamic up to 4. Actual count of usage is what will determin if a measurement will be made.
DHTesp dht_sensor[DHTSize]; //allow up to 4 of these sensors to be used
TempAndHumidity DTHValues[DHTSize];

#include "ESP32_FastPWM.h"
#define PWMSize 4
int pwmSizeDynamic = PWMSize; //This will be dynamic up to 4. Actual count of usage is what will determin if a measurement will be made.
int pwmMap[PWMSize] = {-1,-1,-1,-1}; //used to quickly get PWM_Instance from IO Point Id. (-1 is an invalid IO Point position.)
ESP32_FAST_PWM* PWM_Instance[PWMSize];
uint8_t PWMChannel[PWMSize]{0,1,2,3};
float PWMFrequency[PWMSize] = {1000.0f,1000.0f,1000.0f,1000.0f}; //default Frequency is 1k
float PWMDutyCycle[PWMSize] = {0.0f,0.0f,0.0f,0.0f};
int PWMResolution[PWMSize] = {12,12,12,12};  //Resolution is 4096 this is ok res for up to 10k Frequency lower resolution wiht higher Frequencies. 

// Max resolution is 20-bit
// Resolution 65536 (16-bit) for lower frequencies, OK @ 1K
// Resolution  4096 (12-bit) for lower frequencies, OK @ 10K
// Resolution  1024 (10-bit) for higher frequencies, OK @ 50K
// Resolution  256  ( 8-bit)for higher frequencies, OK @ 100K, 200K
// Resolution  128  ( 7-bit) for higher frequencies, OK @ 500K

#ifdef WMR2
  #define IOSize  14 //number should be one more than the IO# :) (must include the idea of Zero) 
  int IOType[IOSize]{INPUT,INPUT,INPUT,INPUT,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,OUTPUT,OUTPUT,OUTPUT};
  int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6,IO7,IO8,IO9,IO10,IO11,IO12,IO13};
  String IOSMap[IOSize] {"IO0","IO1","IO2","IO3","IO4","IO5","IO6","IO7","IO8","IO9","IO10","IO11","IO12","IO13"};
#endif

#ifdef WMR4
  #define IOSize  16 //number should be one more than the IO# :) (must include the idea of Zero) 
  int IOType[IOSize]{INPUT,INPUT,INPUT,INPUT,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,OUTPUT,OUTPUT,OUTPUT,OUTPUT,OUTPUT};
  int IOMap[IOSize] {IO0,IO1,IO2,IO3,IO4,IO5,IO6,IO7,IO8,IO9,IO10,IO11,IO12,IO13,IO14,IO15};
  String IOSMap[IOSize] {"IO0","IO1","IO2","IO3","IO4","IO5","IO6","IO7","IO8","IO9","IO10","IO11","IO12","IO13","IO14","IO15"};
#endif


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
  if (ioType == OUTPUT_PWM){return "OUTPUT_PWM";}
  if (ioType == INPUT_DHT){return "INPUT_DHT";}
  
  return "UnSupported unknown Type: " + String(ioType);
  
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
