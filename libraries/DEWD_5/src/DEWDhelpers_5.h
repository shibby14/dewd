#ifndef DEWDhelpers_h
#define DEWDhelpers_h

#include <ESP8266WiFi.h>
#include <ESP.h>
#include "include/wl_definitions.h"
#include <WiFiUdp.h>
#include <list>

extern "C" {
#include "user_interface.h"
}

	const int PORT=5050;													//TCP port to listen to
	const char MESH_SSID[] = "ESP-MESH1";					//This constant is used to connect to the right network and establish sofAP
	const int STA_TIMEOUT = 30;										//Amount of 200ms periods before stopping attempts to connect to AP
	const int RECONN_FREQ = 20000;								//How often to attempt to reconnect to the mesh. Value is in ms.
	const int CHECK_CONN = 5000;									//Time between checks if module is connected to mesh. Vlue is in ms. 
	
	
	WiFiServer server(PORT);
	WiFiClient client;
	IPAddress host;
	std::list<int> bid_list (2);
		
		
		
/* Send a string to the gateway IP-address (AP host)
     *
     * param mes: String to be sent.
     */
void send_message(String mes, IPAddress dest, int client_nr = 2) {    
  if (dest != WiFi.gatewayIP()) {
	  dest[3] = client_nr+1;
  }
  Serial.print("Sending to ");
  Serial.println(dest);
  if (client.connect(dest, PORT)) {
    client.print(mes);
    client.println();
    Serial.println("Message sent");
  }
  else
    Serial.println("Failed to connect to host");
}

/* Print the received  message to serial
     *
     */
void listen_to_port() {
  client = server.available();
  
  if (!client) 
    return;
  
  
  String req = client.readStringUntil('\r');
  
  if (req[0] == 'C') {
	  pinMode(5, OUTPUT);
	  digitalWrite(5, 1);
	  Serial.println("TURN ON LED");
  }
  else if (req[0] == 'B') {
	  Serial.println("BROADCAST");
	  
  }
	  
  
  Serial.println("Client available");
  Serial.println(req);
  
  client.flush();  
}

/* Generate an integer between 1 and max
     *
     * param max: Max value.
	 * return: integer
     */
int gen_random(int max) {
  randomSeed(system_get_rtc_time());
  return random(1, max);
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
    Serial.println(") ");
	//Serial.println(WiFi.BSSID(i));
	
	/*if (!strcmp(WiFi.SSID(i), MESH_SSID))
		mesh_bssid = WiFi.BSSID(i);*/
  }
  Serial.println();
}

/* A check to ensure WiFi connection to mesh-network has not been lost. 
     *
	 * return: true if connected to an AP
     */
bool is_connected() {
	if (WiFi.status() != WL_CONNECTED) {
		return false;
		//Serial.println("CONNECTED TO SOMETHING");
	}
	return true;
	
}

bool is_connected_to_mesh() {
	if (!strcmp(WiFi.SSID(), MESH_SSID)) {
		if (WiFi.status() != WL_CONNECTED)
			return false;
		//Serial.println("CONNECTED TO MESH");
		return true;
	}
	return false;
}

/* Check if mesh network is active.
     *
	 * return: true if there is an active AP with SSID=MESH_SSID
     */
bool check_mesh_ap(){
	int n = WiFi.scanNetworks();
	for (int i = 0; i < n; i++)
	{
		if (!strcmp(WiFi.SSID(i), MESH_SSID)) {
			return true;
		}
	}
	return false;
}

/* Connect to a network with SSID = MESH_SSID defined at the top of this file.
     *
     */
void connect_to_mesh() {  
  if (is_connected_to_mesh()) {
	return;
  }
  WiFi.disconnect();

  if (wifi_get_opmode() > 1)							//if in AP or STA_AP mode
	  WiFi.mode(WIFI_AP_STA);				
  else 															//if in NULL or STA mode			
	  WiFi.mode(WIFI_STA);
	
  long start_millis = millis(); 
  String temp_ssid;
  int timeout = 0;

  Serial.print("Connecting to ");
  Serial.print(MESH_SSID);
  Serial.println("...");
  
	if (WiFi.SSID() != MESH_SSID) {
		WiFi.begin(MESH_SSID, "password14");
		while (WiFi.status() != WL_CONNECTED) {
			delay(200);														
			timeout++;
			if (timeout >= STA_TIMEOUT) {
				Serial.print("Request timed out after ");
				Serial.print(millis()-start_millis);
				Serial.println(" ms");
				//wifi_station_set_auto_connect(false);				//ONLY TAKES EFFECT AFTER RESTART - REMOVE EVENTUALLY?
				WiFi.disconnect();
				Serial.println();
				//RESTART MODULE?
				return;
			}			
		}
	}	
	String chip_id_message = "C ";
	chip_id_message += ESP.getChipId();
	send_message(chip_id_message, WiFi.gatewayIP());
	
	Serial.println();
	Serial.print("WiFi connected in: ");   
	Serial.println(millis()-start_millis);
	Serial.println("Connected to mesh");
}

/* Print all relevant IP addresses to serial
     *
     */
void print_IP() {
  IPAddress local_ip = WiFi.localIP();
  IPAddress ap_ip = WiFi.softAPIP();
  IPAddress gateway_ip = WiFi.gatewayIP();
  
  uint8 mac[6];
  wifi_get_macaddr(STATION_IF, mac);		//station MAC 
  
  Serial.print("Station IP: ");
  Serial.println(local_ip);
  Serial.print("Access Point IP: ");
  Serial.println(ap_ip);
  Serial.print("Gateway IP: ");
  Serial.println(gateway_ip);
  
  Serial.println();
  Serial.print("Station interface MAC: ");
  for (int i=0;i<5;i++) {
	Serial.print(mac[i], HEX);
	Serial.print(":");
  }
  Serial.println(mac[5], HEX);
		
  wifi_get_macaddr(SOFTAP_IF, mac);
  Serial.print("SoftAP interface MAC: ");
  for (int i=0;i<5;i++) {
	Serial.print(mac[i], HEX);
	Serial.print(":");
  }
  Serial.println(mac[5], HEX);
  
  /*Serial.print("Station connected to BSSID: ");
  for (int i=0;i<5;i++) {
	Serial.print(mesh_bssid[i], HEX);
	Serial.print(":");
  }
  Serial.println(mesh_bssid[5], HEX);*/
}

/* Set up an open access point with SSID=MESH_SSID at random channel, configure AP addresses, start DHCP (default), and start the TCP server
     *
     */
void setup_mesh() {  
	
	struct softap_config conf;
	wifi_softap_get_config(&conf);
	String ssid(reinterpret_cast<char*>(conf.ssid));			

	if (ssid == MESH_SSID && wifi_get_opmode() > 1) {
		Serial.println("Mesh already active");
		return;
	}
		
	if (wifi_get_opmode() % 2 != 0)										//if in STA or STA_AP mode
	  WiFi.mode(WIFI_AP_STA);		
	else 																			//if NULL or AP mode
	  WiFi.mode(WIFI_AP);	  
	
	int channel = gen_random(15);
	int subn = gen_random(256);																

	IPAddress locAP(192,168,subn,1);
	IPAddress gateAP(192,168,subn,1);
	IPAddress mask(255,255,255,0);
	WiFi.softAPConfig(locAP, gateAP, mask);

	Serial.print("Setting up ");
	Serial.println(MESH_SSID);  
	WiFi.softAP(MESH_SSID, "password14", channel);
	delay(200);
	server.begin();
}

/* Put the module into station only mode, making it only a WiFi client
     *
     */
void station_mode() {
	WiFi.mode(WIFI_STA);
}

/* Activated by the mesh-timer. If module is not connected to mesh => check if mesh-network is active and connect to it. 
     *
     */
void check_and_connect() { 
	if (!is_connected()){
      connect_to_mesh();                
    }
  }

  // ex: "B 148 this is the message to be forwarded"
void broadcast(String payload) {
	String mes = "B ";											// broadcast flag
	
	int id = gen_random(256);								// packet ID
	int count = 0;
	
	mes += id;
	mes += ' ';
	mes += payload;											// add payload to message
	
	send_message(mes, WiFi.gatewayIP());			// forward message to host
		
	bid_list.push_front(id);
	
	struct station_info * station = wifi_softap_get_station_info();
	struct station_info * next_station;
	
	if (station == NULL)
		return;
	
	while(station)												// for each station connected to AP ...
	{ 
		ip_addr * temp = &station->ip;					// get IP
		IPAddress sta_ip(temp->addr);					// type conversion
		send_message(mes, sta_ip, ++count);						// forward message to client
		next_station = STAILQ_NEXT(station, next);
		station = next_station;
	}  
}
  
  
/* Decode the string printed to the serial COM port by the user. 
     *
     * param com: String received from the serial COM port.
	 * return: "done" if ok or "Unidentified Command" if no such command found
     */
String decode_command(String com) {
  Serial.println();
  String ret;
  
  if (com == "list ap\r\n") {
    list_all_ap();
    ret = "done";
  }
  else if (com.indexOf("send host ") != -1) {
    send_message(com.substring(10), WiFi.gatewayIP());
    ret = "done";    
  }
  else if (com.indexOf("send client ") != -1) {
    send_message(com.substring(14), WiFi.softAPIP(), atoi(com.substring(12).c_str()));
    ret = "done";    
  }
  else if (com == "check mesh\r\n") {
    if (!check_mesh_ap())
		Serial.println("No mesh AP");
	else
		Serial.println("Mesh AP active");
    ret = "done";    
  }
  else if (com == "get status\r\n") {
    char* ssid = WiFi.SSID();
    
	Serial.print("WiFi status: ");
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
    
	Serial.print("Connected to: ");
	if (strlen(ssid) < 2)
		Serial.println("NONE");
	else
		Serial.println(ssid);
	
	Serial.print("AP active: ");
	if (wifi_get_opmode() > 1)	
		Serial.println("YES");
	else
		Serial.println("NO");
	
	ret = "done";
  }
  else if (com == "connect tritech\r\n") {
    WiFi.disconnect();
	delay(200);
    WiFi.begin("3T-Guest","SkrivStora3SS");   
    ret = "done";
  }
  else if (com == "disconnect\r\n") {
    WiFi.disconnect();
	//in_mesh = false;
    ret = "done";
  }
  else if (com == "station mode\r\n") {
	station_mode();
	ret = "done";
  }
  else if (com == "connect mesh\r\n") {
    connect_to_mesh();
    ret = "done";
  }
  else if (com == "setup mesh\r\n") {
    setup_mesh();
    ret = "done";
  }
  else if (com == "ap info\r\n") {
	  ret = "No stations connected to current AP";
	  int count = 0;
	  struct station_info * station = wifi_softap_get_station_info();
	struct station_info * next_station;	  
	  if (station == NULL)
		return ret;
	  	  
      while(station)
      { 
		ip_addr * temp = &station->ip;
		IPAddress sta_ip(temp->addr);
		Serial.print(++count);		
		Serial.print(".  IP: ");	
		Serial.print(sta_ip);
			
		Serial.print("       MAC: ");		
		for (int i=0;i<5;i++) {
			Serial.print(station->bssid[i], HEX);
			Serial.print(":");
		}
		Serial.println(station->bssid[5], HEX);		
		next_station = STAILQ_NEXT(station, next);
		station = next_station;
      }
	  wifi_softap_free_station_info();
	  ret = "done";
  }
  else if (com == "ip\r\n" || com == "IP\r\n") {
    print_IP();    
    ret = "done";
  }
  else if (com.indexOf("broadcast ") != -1)  {
	broadcast(com.substring(10));  
	ret = "done";
  }
  else if (com == "restart\r\n" || com == "reset\r\n") {
    system_restart();    
    ret = "done";
  }
  else if (com == "secret\r\n") {
    Serial.print("Vdd is: "); 
	Serial.println(system_get_vdd33());
	Serial.print("ADC is: "); 
	Serial.println(system_adc_read());
	Serial.print("SDK version: ");
	Serial.println(system_get_sdk_version());
	Serial.print("Chip ID: ");
	Serial.println(ESP.getChipId());
	//Serial.print("Connection status: ");
	//Serial.println(wifi_station_get_connection_status());		//Returns an enum
	//Serial.print("RSSI of AP hosting this module: ");
	//Serial.println(wifi_station_get_rssi());
	//Serial.print("Number of stations connected to ESP AP: ");
	//Serial.println(wifi_softap_get_station_num());
	//Serial.println("Sending UDP broadcast...");
	//if (wifi_set_broadcast_if(3))										//1=station; 2=softAP; 3=both
    Serial.print("Random number: ");
	Serial.println(gen_random(256));

	ret = "done";
  }
  else if (com.indexOf("deep-sleep ") != -1){
	int per = atoi(com.substring(11).c_str());     	 			// length of sleep in microseconds
	Serial.print("Deep-sleep for ");
	Serial.print(per);
	Serial.println("us");
	system_deep_sleep_set_option(1);								//Calibrate RF when waking up
	system_deep_sleep(per);	
}
else if (com.indexOf("wifi-sleep ") != -1){
	sleep_type t = (sleep_type) atoi(com.substring(11).c_str());
	wifi_set_sleep_type(t);												//0=NONE, 1=LIGHT, 2=MODEM
	Serial.print("Sleep type ");
	Serial.print(wifi_get_sleep_type());
	Serial.println(" is set!");
}
 /* else if (com == "send udp\r\n") {
	char test[] = "test udp data!";
    sendUDPpacket(host, &test);    
    ret = "done";
  }*/
  else {
    ret = "Unidentified command";
  }  
  return ret;
}


#endif