

#include <ESP8266WiFi.h>
#include "DEWDESP.h"

extern "C" {
#include "user_interface.h"
}

int gen_random(int max) {
  //randomSeed(system_get_rtc_time());
  return random(1, max);
}

void station_mode() {
	WiFi.mode(WIFI_STA);
}