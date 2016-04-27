/*
 DEWDWiFI.h is a part of the DEWD project at LIU for ESP8266 module and contains a list of configuration functions 
 to establish a wifi network with mesh functionality.
 
 Created by Alexander Pukhanov, 2015.
 
 */
 
#ifndef DEWDWiFi_h
#define DEWDWiFi_h

#include <ESP8266WiFi.h>
#include "include/wl_definitions.h"
#include <DEWDESP.h>

extern "C" {
#include "user_interface.h"
}

	const int PORT=4040;											// TCP port to listen to
	
	const int UDP_PORT=5555;
	const int MULTICAST_PORT=5556;
	const IPAddress MULTICAST_IP(224,1,1,1);
	
	const char MESH_SSID[] = "ESP-MESH1";							// This constant is used to connect to the right network and establish sofAP.
	const char MESH_PASSWORD[] = "password14";
	const int STA_TIMEOUT = 6;										// Amount of time before giving up on connecting to AP. Value is in seconds.
	const int RECONN_FREQ = 15;										// How often to attempt to reconnect to the mesh. Value is in seconds.
	const int RECONN_RST_AFTER = 3;									// Restart module if can't connect after 3 tries.

	bool DEBUG = false;
	bool MESH_MODE_ACTIVE = true;

	int failed_reconnects = 0;

/* A check to ensure WiFi connection to an AP has not been lost. 
     *
	 * return: true if connected to an Access Point
     */
bool is_connected() {
	if (WiFi.status() != WL_CONNECTED) {
		return false;
	}
	return true;
}

/* A check to ensure WiFi connection to the MESH network has not been lost. 
     *
	 * return: true if connected to a MESH_SSID Access Point
     */
bool is_connected_to_mesh(void) {
	if (DEBUG) {
		Serial.println("Checking mesh connection...");
		Serial.print("WiFi status = ");
		switch (WiFi.status()) {
			case 0:
				Serial.println("WL_IDLE_STATUS");
				break;
			case 1:
				Serial.println("WL_NO_SSID_AVAIL");
				break;
			case 2:
				Serial.println("WL_SCAN_COMPLETED");
				break;
			case 3:
				Serial.println("WL_CONNECTED");
				break;
			case 4:
				Serial.println("WL_CONNECT_FAILED");
				break;
			case 5:
				Serial.println("WL_CONNECTION_LOST");
				break;
			case 6:
				Serial.println("WL_DISCONNECTED");
				break;
		}
		Serial.print("SSID = ");
		Serial.println(WiFi.SSID());
		Serial.print("WiFi connection status = ");
		switch (wifi_station_get_connect_status()) {
			case 0:
				Serial.println("STATION_IDLE");
				break;
			case 1:
				Serial.println("STATION_CONNECTING");
				break;
			case 2:
				Serial.println("STATION_WRONG_PASSWORD,");
				break;
			case 3:
				Serial.println("STATION_NO_AP_FOUND");
				break;
			case 4:
				Serial.println("STATION_CONNECT_FAIL");
				break;
			case 5:
				Serial.println("STATION_GOT_IP");
				break;
		}
		Serial.println();
	}
	if (!strcmp(WiFi.SSID().c_str(), MESH_SSID)) {
		if (!is_connected()) {
			// wireless status is not WL_CONNECTED
			return false;
		}
		else if (wifi_station_get_connect_status() != 1 && wifi_station_get_connect_status() != 5) {
			// connection status not CONNECTING or GOT_IP
			return false;
		}		
		return true;
	}
	return false;	
}

/* Check if MESH network is active.
     *
	 * return: true if there is an active AP with SSID=MESH_SSID
     */
bool check_mesh_ap(){
	int n = WiFi.scanNetworks();
	for (int i = 0; i < n; i++)
	{
		if (!strcmp(WiFi.SSID(i).c_str(), MESH_SSID)) {
			return true;
		}
	}
	return false;
}

/* Connect to a network with SSID = MESH_SSID
     *
     */
void connect_to_mesh() {  

	if (wifi_get_opmode() > 1)								// if in AP or STA_AP mode...
		WiFi.mode(WIFI_AP_STA);								// ...set mode to STA_AP
	else 													// if in NULL or STA mode...			
		WiFi.mode(WIFI_STA);								// ...set mode to STA
	
  	long start_millis = millis(); 
  	int timeout = 0;

	if (DEBUG) {
		Serial.println();
		Serial.print("Connecting to ");
		Serial.print(MESH_SSID);
		Serial.println("...");
	}
  
	if (WiFi.SSID() != MESH_SSID) {
		WiFi.begin(MESH_SSID, MESH_PASSWORD);		
		while (WiFi.status() != WL_CONNECTED) {
			delay(200);														
			timeout++;
			if (timeout >= STA_TIMEOUT*5) {
				if (DEBUG) {
					Serial.print("Request timed out after ");
					Serial.print(millis()-start_millis);
					Serial.println(" ms");
					Serial.println();
				}				
				failed_reconnects++;
				if (failed_reconnects >= RECONN_RST_AFTER) {
					if (!check_mesh_ap()) {
						if (DEBUG) {
							Serial.print("RECONN_RST_AFTER reached - disconnecting...");
						}
						WiFi.disconnect();
					}	
					failed_reconnects = 0;
				}		
				return;
			}			
		}
	}	
	if (DEBUG) {
		Serial.print("WiFi connected in: ");   
		Serial.println(millis()-start_millis);
		Serial.println("Connected to mesh");
		Serial.println();
	}
	wifi_station_set_auto_connect(true);
}

/* Set up an open Access Point with SSID=MESH_SSID at random channel, configure AP addresses, start DHCP (default), and start the TCP server
     *
     */
void setup_mesh() {  
	struct softap_config conf;
	wifi_softap_get_config(&conf);
	String ssid(reinterpret_cast<char*>(conf.ssid));			
		
	WiFi.mode(WIFI_AP_STA);		

	int channel = gen_random(13);										// randomly generate channel number
	int subn = gen_random(256);											// randomly generate private subnet octet (192.168.x.1)				

	IPAddress locAP(192,168,subn,1);
	IPAddress gateAP(192,168,subn,1);
	IPAddress mask(255,255,255,0);
	WiFi.softAPConfig(locAP, gateAP, mask);

	if (DEBUG) {
		Serial.println();
		Serial.print("Setting up ");
		Serial.println(MESH_SSID); 
	}	
	WiFi.softAP(MESH_SSID, "password14", channel);
	delay(100);
}

/* Print all detected APs to serial
     * 
     */
void list_all_ap(){
  int n = WiFi.scanNetworks();
  Serial.println("Available APs:");
  Serial.print(n);
  Serial.println(" networks found.");
  Serial.println("");
  for (int i = 0; i < n; i++)
  {
    Serial.print(i+1);
    Serial.print(". ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print(") ");
	
	uint8 * mac = WiFi.BSSID(i);
	for (int i=0;i<5;i++) {
		Serial.print(*mac, HEX);
		Serial.print(":");
		mac++;
	}
	Serial.println(*mac, HEX);
		
  }
  Serial.println();
}

/* Print all relevant IP and MAC addresses to serial
     *
     */
void print_IP() {
  IPAddress local_ip = WiFi.localIP();
  IPAddress ap_ip = WiFi.softAPIP();
  IPAddress gateway_ip = WiFi.gatewayIP();
  
  uint8 mac[6];
  wifi_get_macaddr(STATION_IF, mac);		//station MAC 
  
  Serial.print("Station IP:	");
  Serial.print(local_ip);
  Serial.print("	");
  for (int i=0;i<5;i++) {
	Serial.print(mac[i], HEX);
	Serial.print(":");
  }
  Serial.println(mac[5], HEX);
  
  Serial.print("Access Point IP:");
  Serial.print(ap_ip);
  Serial.print("	");
  wifi_get_macaddr(SOFTAP_IF, mac);
  for (int i=0;i<5;i++) {
	Serial.print(mac[i], HEX);
	Serial.print(":");
  }
  Serial.println(mac[5], HEX);
  
	Serial.print("Gateway IP:	");
	Serial.print(gateway_ip); 
	Serial.print("	");
	uint8 * mac_gate = WiFi.BSSID();
	for (int i=0;i<5;i++) {
		Serial.print(*mac_gate, HEX);
		Serial.print(":");
		mac_gate++;
	}
	Serial.println(*mac_gate, HEX);
}
#endif