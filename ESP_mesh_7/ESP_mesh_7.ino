#include "DEWDWiFi.h"
#include "DEWDComm.h"
#include <osapi.h>
#include <os_type.h>
#include "ESP8266WiFi.h"
//------------------------------ Constants ----------------------------------
// Check DEWDWiFi.h and DEWDComm.h for further network configuration constants

const int DELAY = 500;                  // delay in ms to allow TCP/IP to process
//---------------------------- Global vars ----------------------------------
int ticker = 0;                         // timer init 
long start_ms;                        
//---------------------------------------------------------------------------
void setup() { 
  Serial.begin(9600);                   // baud rate (should be same for EM50 datalogger)

  randomSeed(system_get_rtc_time());    // randomize seed for subnet generation
  
  if (MESH_MODE_ACTIVE){  
    setup_mesh();                       // setup mesh AP
    connect_to_mesh();                  // connect to mesh AP
    start_ms = millis(); 
  }
}

void loop() {    
  if (Serial.available()) {
    decode_command(Serial.readString());  // decodes/executes serial command
  }
  listen_to_ports();                      // process TCP/UDP messages

  if (MESH_MODE_ACTIVE){
    if (ticker % (RECONN_FREQ*1000/DELAY) == 0) {

      // --- Sets LED on DEV board to off ---
      pinMode(5, OUTPUT);
      digitalWrite(5, 0);    
      // ------------------------------------
      
      if (!is_connected_to_mesh()) {
        connect_to_mesh();
      }               
      ticker=0; 
    }
    ticker++;
  }
  delay(DELAY);
}
