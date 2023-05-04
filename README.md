# SIOControlFirmware
SIOControl Firmware
## SIO Serial Commands 

All commands must end with a new line character (\n) If the command is recognized as a valid command the device will respond with an "OK" as an Ack. if it is not recognized, it will return an error in the format of: if command "AA" is sent to the device.A return code of: "ERROR: Unrecognized command[AA]\n" will be sent.


- EIO Pause/End IO Status Autoreporting. This setting is not maintained through restarts of the device.
- BIO Begin AutoReporting IO status (only needed if EIO has been called.) 
- VC will return the Version and compatibility messaging (this is used by the Octoprint_SIOPlugin to determin if it is a compatible device. 
- IC returns the number of IO points being monitored. 
- debug [0/1] turns on[1] or off[0] debug output. Should not be used while connected to Octoprint SIOControl. This setting is not maintained through restarts of the device.
- CIO [io pattern] ; The [io pattern]  is an string of integers as a single value. For example: 0000111111 (this sets the first 4 io points to inputs and the last 6 to output.)
In most cases, there are 4 posible values that can be used for each type. 0=input, 1=output, 2 input_Pullup, 3 input_pulldown, Output_open_drain. Not all devices support all of the types listed. 0-2 are supported in most cases. This setting is not maintained through restarts of the device unless settings are stored using SIO command.
- SIO Stores current IO point type settings to local storage. This causes the current IO point types to be maintained through restarts of the device.
- IO [#] [0/1] Sets an IO point {#:position in IO pattern] to a logic level [0:low] [1:HIGH]. Example: "IO 9 1" will set the io point 9 to High 
- IOT outputs the current IO Point types pattern
- SI [500-4,294,967,295]  .5 seconds to ~1,193 hours realistically you would want this setting to be no less than 10,000 or 10 seconds to ensure generally acceptable feedback on changes made to the IO.
- SE enables or disables event triggering of IO Reports. (on by default). When enabled, a change in IO Status(up or down) will cause a report output to be sent to the host. When disabled, reports are only given on interval. 
- restart,reboot,reset will all force a restart of the device. (tested on esp8266 and esp32) 
- GS will force an IO status report. 
- note that if a command starting with an "N" or "M115" is sent the firmware should issue a disconnect command to the host. This is meant to help when your octoprint setup tries to connect to the SIO Device thinking it is the printer. 
