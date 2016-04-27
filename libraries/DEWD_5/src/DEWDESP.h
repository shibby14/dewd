/*
 DEWDESP.h is a part of the DEWD project for ESP8266 module and contains a list of general functions specific to ESP8266.
 Created by Alexander Pukhanov, 2015.
 
 */
 
#ifndef DEWDESP_h
#define DEWDESP_h




/* Generate an integer between 1 and max
     *
     * param max: Max value.
	 * return: integer
     */
int gen_random(int max);

/* Put the module into station only mode, making it a WiFi client only
     *
     */
void station_mode();


#endif