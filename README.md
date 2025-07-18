# SIOControl Firmware
This firmware is designed to work with the SIOControl PlugIn for OctoPrint. It could be used for general control from any device that can send commands over a serial line. 

There are several versions listed. Each targeting a specific board. These boards can be obtained though my [Tindie store front](https://www.tindie.com/stores/softwaresedge/) or in a number of other places.
One of the goals of this project is to make it easy for people that dont have the ability to just build out a control board. This is why I have setup the Tindie store. 
If you have a board / MCU you perfer, you likley can just modify the Firmware and get moving. Knowing what MCU is on that board should give you a good starting point if you have another board with the same MCU.


If you are looking for some support, checkout the [Firmware wiki](https://github.com/jcassel/OctoPrint_SIOControl_Firmware/wiki) and the [PlugIn wiki](https://github.com/jcassel/OctoPrint-Siocontrol/wiki). 
If you dont find what you are looking for there, I lurk a lot on the [OctoPrint community site](https://community.octoprint.org/). Post there in the plugins section and I will likely see it and respond.
You can also post an issue in the [issues area here on GitHub](https://github.com/jcassel/OctoPrint_SIOControl_Firmware/issues).

## Prerequisites for ESP32 Versions.

1. [`Arduino IDE 1.8.19+` for Arduino](https://github.com/arduino/Arduino). [![GitHub release](https://img.shields.io/github/release/arduino/Arduino.svg)](https://github.com/arduino/Arduino/releases/latest)
2. [`ESP32 Core (2.0.6 - 2.0.17)`](https://github.com/espressif/arduino-esp32) for ESP32-based boards. [![Latest release](https://img.shields.io/github/release/espressif/arduino-esp32.svg)](https://github.com/espressif/arduino-esp32/releases/latest/).
