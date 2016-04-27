/*
 DEWDComm.h is part of the DEWD project for ESP8266 module and contains a list of functions to tie in serial and wireless communication.
 This file has all necessary functions for TCP, UDP and serial communication, as well as handling broadcast formatting and decoding.
 Created by Alexander Pukhanov, 2015.
 
 */

#ifndef DEWDComm_h
#define DEWDComm_h

#include <ESP8266WiFi.h>
#include <DEWDBroadcast.h>
#include <MACAddress.h>
#include <DEWDUdp.h>
#include <DEWDTcp.h>
	
	IPAddress host;
	DEWDUdpClass udp(UDP_PORT, MULTICAST_IP, MULTICAST_PORT);		// unreliable, sometimes port initialization fails (ports get set to 4097 and 4098)! 
																	// Check for alternative libs for UDP.
	DEWDTcpClass tcp(4040);											// initiate TCP server and listen to port 4040
	
	int active_broadcasts_index = 0;								// nr of active broadcasts, also used as index
	DEWDBroadcast active_broadcasts [6];							// an array of max 5 (0 is ignored) active DEWDBroadcasts, to allow for 
																	// varying propagation delays when multiple broadcast active at same time.
																	// Eventually should be replaced with list, while making sure memory is managed
	
void decode_command(String com);									// forward declaration of decode_command()

/* Helper function for parsing strings 
     *
	 * param str: string
	 * return: pos of the first space character
     */
int get_divider(String str) {
	return str.indexOf(' ');
}
	
/* Convert an object of IPAddress type to string
     *
	 * param addr: IPAddress object to be converted
	 * return: String object x.x.x.x (x is octet 0-255)
     */
String ip_to_string(IPAddress addr) {
	String ret;
	for (int i=0; i<3;i++) {
		ret += addr[i];
		ret += '.';
	}
	ret += addr[3];
	return ret;
}

/* Convert a String into IPAddress object
     *
     * param r: String to be translated, each octet should be separated by a '.' 
	 * return: IPAddress object
     */
IPAddress string_to_ip(String r) {
	int i=0, start_pos=0, oct_nr=0;
	uint8_t octets [4];
	
	while (r[i] != '\0') {
		if (r[i] == '.') {
			octets[oct_nr++] = atoi(r.substring(start_pos, i).c_str());
			start_pos = i+1;
		}
		i++;
	}
	octets[3] = atoi(r.substring(start_pos, i).c_str());
	return IPAddress(octets[0], octets[1], octets[2], octets[3]);
}

/* Returns the MAC address of either the STA or the softAP
     *
     * param sta: boolean signalling whether the output should be STAs MAC: 1=STA, 0=softAP
	 * return: a String object formatted in the typical MAC fashion: xx:xx:xx:xx:xx:xx
     */
String mac_string (bool sta){
	uint8 mac[6];
	String ret;
	if (sta)
		wifi_get_macaddr(STATION_IF, mac);		//station MAC 
	else
		wifi_get_macaddr(SOFTAP_IF, mac);		//softAP MAC 

	for (int i=0;i<5;i++) {
		ret += String(mac[i], HEX);
		ret += ":";
	}
	ret += String(mac[5], HEX);
		
	return ret;
}


/* Once a broadcast is complete it needs to be deleted and a response sent to broadcast originator. 
	This function rearranges the active_broadcasts array so that the removed DEWDBroadcast 
	object does not necessarily have to be last in active_broadcasts array
     *
	 * param nr: index of the object in active_broadcasts that is to be deleted
	 * 
     */
void remove_active_broadcasts(int nr) {
	if (nr > 5) {
		if (DEBUG)
			Serial.println("No such broadcast nr");
	}
	else {
		if (nr != active_broadcasts_index) {									// if not the last broadcast...						
			active_broadcasts[nr] = active_broadcasts[active_broadcasts_index];	// overwrite with last broadcast
		}
		if (DEBUG) {
			Serial.println("Removing broadcast: ");		
			Serial.println(active_broadcasts[nr].print_values());
		}

		//String tcp_packet = tcp.make_packet('R', INADDR_NONE, active_broadcasts[nr].resp_message, active_broadcasts[nr].id);			// CHANGE IP BEFORE RELEASE!
		String tcp_packet = tcp.make_packet('R', WiFi.softAPIP(), active_broadcasts[nr].resp_message, active_broadcasts[nr].id);
		if (!tcp.send_by_ip(tcp_packet, active_broadcasts[nr].src_ip)) {											// send response to broadcast source
			if (DEBUG)
				Serial.println("Couldn't reach source IP for broadcast response");
		}
		else {
			if (DEBUG)
				Serial.println("Response sent");
			active_broadcasts_index--;		
		}
	}
}

/* Return the payload part of a message
     *
	 * param str: part of a message containing only src_ip and payload i.e. 192.168.201.2 PAYLOAD
	 * return: string containing only the payload part
     */
String get_payload_string(String str) {
	int i=0;
	while(true) {
		if (str[i] == ' ')
			return str.substring(++i);
		i++;
	}
}

/* Return the src_ip part of a message
     *
	 * param str: part of a message containing only src_ip and payload ie. 192.168.201.2 PAYLOAD
	 * return: string containing only the ip_src part
     */
String get_ip_string(String str) {
	int i=0;
	while(true) {
		if (str[i] == ' ') {
			return str.substring(0, i);
		}
		i++;
	}
}

/* Execute the command depending on the broadcast-mode flag in the payload part of the broadcast. 
	A CR character is added to all strings as a precaution against incorrect input. 
     *
	 * param command: string containing the command
	 * return: string containing response, either sensor output or status 
     */
String execute_broadcast(String command) {
	String resp = "No response";
	char buff[1024];					// response buffer, may be increased if needed
	
	// map the network by returning the current nodes host and clients MAC addresses
	if (!strcmp(command.substring(0, 11).c_str(), "MAP_NETWORK")) {
		resp = "| H: "; 		
		// ---- HOST MAC ADDRESS -------
		uint8 * mac_gate = WiFi.BSSID();
		for (int i=0;i<5;i++) {
			resp += String(*mac_gate, HEX);
			resp += ':';
			mac_gate++;
		}
		resp += String(*mac_gate, HEX);
		// ------------------------------
		resp += "| C: ";
		// ---- CLIENT MAC ADDRESSES ----
		int count = 0;
		struct station_info * station = wifi_softap_get_station_info();
		struct station_info * next_station;	  
		if (station == NULL)
			resp += "none|";
		  
		while(station)
		{ 		
			for (int i=0;i<5;i++) {
				resp += String(station->bssid[i], HEX);
				resp += ':';
			}
			resp += String(station->bssid[5], HEX);		
			next_station = STAILQ_NEXT(station, next);
			station = next_station;
			resp += '|';
		}
		wifi_softap_free_station_info();
		// --------------------------------
		return resp;
	}
	// shortcut for network-wide deep-sleep broadcast. No resp message is needed
	else if (!strcmp(command.substring(0, 10).c_str(), "DEEP_SLEEP")) {
		long sleep_time = command.substring(11).toInt();	
		sleep_time = sleep_time * 1000000;							// multiply with 10^6 to get microseconds
		system_deep_sleep_set_option(1);							// calibrate RF when waking up
		system_deep_sleep(sleep_time);
	}
	// shortcut for network-wide restart. No resp message is needed
	else if (!strcmp(command.substring(0, 10).c_str(), "RESTART")) {
		system_restart();
	}
	// forward command to sensor and wait for response
	else if (!strcmp(command.substring(0, 6).c_str(), "SENSOR")) {
		command = command.substring(7);
		command += '\r';											// guarantee correct sensor input termination 
		Serial.print(command);										// print command to serial (to sensor)
		Serial.flush();												// force CPU to wait for serial to transmit

		Serial.readBytes(buff, 1024);								// fill buffer
		resp = strlen(buff);										
		resp += ' ';
		resp += String(buff);
		resp.replace('\r', '|');
		return resp;												// i.e. resp = 17 data from sensor|
	}
	// execute an ESP8266 command. See API (or decode_command()) for exact list and format of command
	else {
		decode_command(command);
		resp = "executed";
		return resp;						
	}					
}

/* Create a broadcast to be sent to all neighbours (AP and STAs).
     *
	 * param payload: String to be broadcast
	 * param b: broadcast object containing the necessary meta-data
	 * 
     */
void broadcast(String payload, DEWDBroadcast b) {
	if (DEBUG)
		Serial.println("Composing broadcast messages...");
	
	// This part forwards broadcast to host
	if (b.src_ip != WiFi.gatewayIP() && WiFi.gatewayIP()[0] != 0) {						// host it not source of broadcast		
		if (DEBUG)
			Serial.println("Sending to host...");
		
		String tcp_packet = tcp.make_packet('B', WiFi.localIP(), payload, b.id);		// make tcp broadcast-packet with STA-IP as source
		if (!tcp.send_by_ip(tcp_packet, WiFi.gatewayIP())) {																
			if (DEBUG)
				Serial.println("failed");
		}
		else {
			active_broadcasts[active_broadcasts_index].resp_index++;					// increase number of responses expected
			if (DEBUG)
					Serial.println("sent");
		}			
	}
	
	if (DEBUG) {
		Serial.print("active_broadcasts_index=");
		Serial.println(active_broadcasts_index);
	}
	
	// This part forwards broadcast to clients
	struct station_info * station = wifi_softap_get_station_info();
	struct station_info * next_station;
	
	if (station == NULL) {																		// if no clients...
		if (active_broadcasts[active_broadcasts_index].resp_index == 0)							// and not awaiting any responses...
			remove_active_broadcasts(active_broadcasts_index);									// finalize broadcast by sending a response and then freeing active broadcasts slot  
		return;
	}
	int count = 2;																				// last octet of client. Always starts with 2
	String tcp_packet = tcp.make_packet('B', WiFi.softAPIP(), payload, b.id);					// make tcp broadcast-packet with SoftAP-IP as source
	
	while(station)																				// for each station connected to AP ...
	{ 
		ip_addr * temp = &station->ip;								
		IPAddress sta_ip(temp->addr);							
		
		IPAddress client_ip = WiFi.softAPIP();
		client_ip[3] = count;																	// first clients last octet is always 2, second client - 3, third - 4, etc.																	
		
		if (DEBUG) {
			Serial.print("Client ");
			Serial.print(count - 1);
			Serial.print(": ");
			Serial.print(client_ip);
			Serial.println(" - sending...");
		}

		if (b.src_ip != client_ip) {															// check that the client is not the source of the broadcast
			if (!tcp.send_by_ip(tcp_packet, client_ip)) {																
				if (DEBUG)
					Serial.println("failed");
			}
			else {
				active_broadcasts[active_broadcasts_index].resp_index++;						// expect a response from this client
				if (DEBUG)
					Serial.println("sent");
			}		
		}
		else {																					// nothing sent to this client and no response is awaited
			if (DEBUG)
				Serial.println("client is the source - nothing sent");
		}
			
		count++;
		next_station = STAILQ_NEXT(station, next);
		station = next_station;
	}  
	if (active_broadcasts[active_broadcasts_index].resp_index == 0){							// this is needed if the only client connected to AP 
		if (DEBUG)																				// was the source of the broadcast
			Serial.println("No more broadcasts");
		remove_active_broadcasts(active_broadcasts_index);										// finalize broadcast by sending a responces and then freeing active broadcasts slot
	}
}

/* This function does one of 3 things, sequentially going from 1-3:
		1) broadcast is identified as a duplicate and a wrong-response message is sent (flag 'W')
		2) current node is an edge node so a response message is sent (flag 'R')
		3) current node is connected to at least one other ESP that is not the source of broadcast - a new broadcast object is created 
     *
	 * param s: Broadcast string to be parsed
	 *
     */
void parse_broadcast(String s) {
	
	// s might look something like "B 123 192.168.4.1 texttextext"
	int s_id = atoi(s.substring(1, 5).c_str());
	IPAddress s_src = string_to_ip(get_ip_string(s.substring(6)));
	if (DEBUG) {
		Serial.print("ID is: ");
		Serial.print(s_id);
		Serial.print(", SRC_IP is: ");
		Serial.println(s_src);
	}
	
	if (active_broadcasts_index > 5) {
		if (DEBUG)
			Serial.println("Too many active broadcasts");
		active_broadcasts_index = 1;										// round-robin
	}
	
	if (active_broadcasts_index != 0) {										// check if duplicate broadcast
		for (int i=1; i<=active_broadcasts_index; i++) {	
			if (DEBUG) {
				Serial.print("active_broadcasts[");
				Serial.print(i);
				Serial.print("].id = ");
				Serial.println(active_broadcasts[i].id);
			}
			if (active_broadcasts[i].id == s_id) {
				if (DEBUG)
					Serial.println("Duplicate broadcast!");
				String tcp_packet = tcp.make_packet('W', INADDR_NONE, "", s_id);
				// send "W <id>" to s_src IP
				if (!tcp.send_by_ip(tcp_packet, s_src)) {										
					if (DEBUG)
						Serial.println("W-message not sent");
				}
				return;
			}
		}
	}	
	// if this is an edge node...
	if (wifi_softap_get_station_num() == 0) {								
		if (DEBUG) 
			Serial.println("Edge node");
		
		String payload = mac_string(true) + " " + execute_broadcast(get_payload_string(s.substring(6))) + ";";
		String tcp_packet = tcp.make_packet('R', INADDR_NONE, payload, s_id);
		
		if (!tcp.send_by_ip(tcp_packet, s_src)) {							// send response to broadcast source
			if (DEBUG)
				Serial.println("Couldn't reach source IP for broadcast response");
		}
		else {
			if (DEBUG)
				Serial.println("Response sent");
		}
		return;
	}		
	// ...otherwise create broadcast object
	if (DEBUG)
		Serial.println("Create new DEWDBroadcast object...");

	DEWDBroadcast br(s_src, s_id);											
	active_broadcasts[++active_broadcasts_index] = br;
	
	String mac;
	if (WiFi.localIP()[0] != 0)
		mac = mac_string(true);
	else
		mac = mac_string(false);
	
	active_broadcasts[active_broadcasts_index].resp_message += mac;
	active_broadcasts[active_broadcasts_index].resp_message += " ";
	active_broadcasts[active_broadcasts_index].resp_message += execute_broadcast(tcp.parse(s));
	active_broadcasts[active_broadcasts_index].resp_message += ";";
	
	// broadcast message, must be placed here, otherwise br is removed in broadcast() before response is composed
	if (DEBUG) 
		Serial.println("Here calling broadcast()...");
	broadcast(tcp.parse(s), br);
}

/* Listen to all ports, identify packet-flags and take appropriate actions
     *
     */
void listen_to_ports() {
	String req;
	
	// UDP listener
	String udp_printout = udp.listen();
	if(udp_printout.length() > 0) {
		req = udp_printout;
		if (DEBUG) {
			Serial.println("UDP received!");
			Serial.println(udp_printout);
		}
	}
	udp.restart_server();										// NEEDED BECAUSE OF RANDOM-PORT PROBLEM
	
	// TCP listener
	String tcp_printout = tcp.listen();
	if(tcp_printout.length() > 0) {
		req = tcp_printout;										// TCP CAN OVERWRITE UDP REQUEST
		if (DEBUG) {
			Serial.println("TCP received!");
			Serial.println(tcp_printout);
		}
	}
  
	if (tcp_printout.length() + udp_printout.length() < 1) 		// no data received
		return;
	
	char flag = req[0];
	
	// turn led ON on EVB board. Does nothing on non-EVB modules
	pinMode(5, OUTPUT);
	digitalWrite(5, 1);
	// ---------------------------------------------------------

  if (flag == 'C') {										// connected message
	pinMode(5, OUTPUT);										// FILLER CODE
	digitalWrite(5, 1);
	if (DEBUG)
		Serial.println("Connected!");
  }	
  else if (flag == 'B') {									// broadcast message
	if (DEBUG) 
		Serial.println("Identified as broadcast...");
	parse_broadcast(req);									// parse it!
  }
  else if (flag == 'U') {									// UDP message
	if (DEBUG) 
		Serial.println("Identified as UDP message...");
	Serial.println(udp.parse(req));							// print it to serial!
  }
  else if (flag == 'M') {									// TCP message
	if (DEBUG) 
		Serial.println("Identified as TCP message...");
	Serial.println(tcp.parse(req));							// print it to serial!
  }
  else if (flag == 'W') {									// wrong response, means source already received broadcast through another route
	if (DEBUG) 
		Serial.println("Identified as wrong-response...");
	
	int w_id = atoi(req.substring(1, 5).c_str());
	for (int i=1; i<=active_broadcasts_index; i++) {		// find the broadcast in question...				
		if (active_broadcasts[i].id == w_id) {
			active_broadcasts[i].resp_index--;				// lower expected responses by 1
			if (active_broadcasts[i].resp_index == 0)
				remove_active_broadcasts(i);				// send responses to broadcasts originator IP and remove this broadcast
		}
	}
  }
  else if (flag == 'R') {									// broadcast response
	if (DEBUG) 
		Serial.println("Identified as response to a broadcast...");
	
	int w_id = atoi(req.substring(1, 5).c_str());
	for (int i=0; i<=active_broadcasts_index; i++) {						// find broadcast in question
		if (active_broadcasts[i].id == w_id) {			
			active_broadcasts[i].resp_message += req.substring(6);			// add response to that DEWDBroadcasts object 			
			active_broadcasts[i].resp_index--;								// lower expected responses by 1
			if (active_broadcasts[i].resp_index == 0){						// if all responses have been received...
				if (DEBUG) 
					Serial.println("No more responses, remove broadcast");
				remove_active_broadcasts(i);								// send responses to broadcasts originator IP and remove this broadcast
			}
			break;
		}
	}
  }
  else {													// none of the above - do nothing
	  if (DEBUG) {
		Serial.println("Non-standard message!");
		Serial.print("Exact message: ");
		Serial.println(req);
	  }
  }

	if (DEBUG)
		Serial.println();									// pretty formatting in Arduino serial terminal
}
  
  /* Decode the string written to the serial COM port, or received through a broadcast 
     *
     * param com: command to be executed
	 * return: "done" if ok or com if not
     */
void decode_command(String com) {
  	String ret;
  
	if (!strcmp(com.substring(0, 6).c_str(), "print ")){
		if (!strcmp(com.substring(6, 8).c_str(), "-a")) {						// list all APs in range
			list_all_ap();
		}
		else if (!strcmp(com.substring(6, 8).c_str(), "-b")) {					// print all active broadcasts
			for (int i=1; i <= active_broadcasts_index; i++) {
				Serial.println(active_broadcasts[i].print_values());
			}
		} 
		else if (!strcmp(com.substring(6, 8).c_str(), "-c")) {					// print IPs of clients connected to this node
			Serial.println("No stations connected to current AP");
			Serial.println();
			int count = 0;
			struct station_info * station = wifi_softap_get_station_info();
			struct station_info * next_station;	  
			if (station == NULL)
				return;
			  
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
		}
		else if (!strcmp(com.substring(6, 8).c_str(), "-i")) {					// print station, AP and gateway IPs
			Serial.println();
			print_IP();
		}
		else
			Serial.println("Valid flags: -a -b -c -i");
		ret = "done";
  	}
	else if (!strcmp(com.substring(0, 5).c_str(), "mesh ")) {
		if (!strcmp(com.substring(5, 7).c_str(), "-s")) {							// Check if AP with mesh SSID is active
			if (!check_mesh_ap())
				Serial.println("No mesh AP");
			else
				Serial.println("Mesh AP active");
		}
		else if (!strcmp(com.substring(5, 7).c_str(), "-S")) {					// Set up a mesh-node
			 setup_mesh();
		}
		else if (!strcmp(com.substring(5, 7).c_str(), "-a")) {					// Toggle mesh-mode
			if (MESH_MODE_ACTIVE) {
				Serial.println("Mesh inactive");
				MESH_MODE_ACTIVE = false;
			}
			else {
				Serial.println("Mesh active");
				MESH_MODE_ACTIVE = true;
			}
		}
		else if (!strcmp(com.substring(5, 7).c_str(), "-c")) {
			connect_to_mesh();
		}
		else
			Serial.println("Valid flags: -s -S -a -c");
		ret = "done";
	}
	else if (!strcmp(com.substring(0, 4).c_str(), "esp ")) {
		if (!strcmp(com.substring(4, 6).c_str(), "-s")) {					// Print out modules WiFi-status and interface info
			const char* ssid = WiFi.SSID().c_str();
    
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
		}
		else if (!strcmp(com.substring(4, 6).c_str(), "-c")) {				// Connect to specified AP
			WiFi.disconnect();
			String ssid = com.substring(7);			
			ssid.replace('\r', '\0');
			
			if (ssid == "mesh") {
				connect_to_mesh();
				ret = "done";
				return;
			}		
			Serial.println("Password:");
			Serial.setTimeout(10000);
			String password = Serial.readStringUntil('\n');
			Serial.setTimeout(1000);
			password.replace('\r', '\0');

			WiFi.begin(ssid.c_str(), password.c_str());
		}
		else if (!strcmp(com.substring(4, 7).c_str(), "-wm")) {				// Change WiFi PHY mode
			String mode = com.substring(8);
			mode.replace('\r', '\0');
			phy_mode enum_mode;
			
			if (mode == "b" || mode == "B")
				enum_mode = PHY_MODE_11B;
			else if (mode == "g" || mode == "G")
				enum_mode = PHY_MODE_11G;
			else if (mode == "n" || mode == "N")
				enum_mode = PHY_MODE_11N;
			else {
				ret = "wrong mode";
				return;
			}
			if (wifi_set_phy_mode(enum_mode)) {
				ret = "done";
				system_restart();
			}
			else
				ret = "failed";
		}
		else if (!strcmp(com.substring(4, 6).c_str(), "-d"))				// Disconnect from current AP
			WiFi.disconnect();
		else if (!strcmp(com.substring(4, 6).c_str(), "-m")) {			// Change ESP mode to 
			String mode = com.substring(7);
			mode.replace('\r', '\0');
			if (mode == "OFF") {
				WiFi.disconnect();
				WiFi.mode(WIFI_OFF);
			}
			else if (mode == "STA")
				WiFi.mode(WIFI_STA);
			else if (mode == "AP") {
				WiFi.disconnect();
				WiFi.mode(WIFI_AP);
			}
			else if (mode == "AP_STA")
				WiFi.mode(WIFI_AP_STA);
			else
				Serial.println("Error! Mode must be either OFF, STA, AP or AP_STA");
		}
		else if (!strcmp(com.substring(4, 6).c_str(), "-r")) 				// Restart module
			system_restart();
		else if (!strcmp(com.substring(4, 6).c_str(), "-D")) {			// Enter deep-sleep for a given amount of us
			long per = com.substring(7).toInt();     	 					// length of sleep in microseconds
			Serial.print("Deep-sleep for ");
			Serial.print(per);
			Serial.println("us");
			system_deep_sleep_set_option(1);									//Calibrate RF when waking up
			system_deep_sleep(per);
		}
		else if (!strcmp(com.substring(4, 6).c_str(), "-W")) {			// Change wifi-sleep mode - 0=NONE, 1=LIGHT, 2=MODEM
			sleep_type t = static_cast<sleep_type>(com.substring(7).toInt());
			wifi_set_sleep_type(t);	
			Serial.print("WiFi-Sleep mode changed to ");
			Serial.println(wifi_get_sleep_type());
		}
		else if (!strcmp(com.substring(4, 6).c_str(), "-p")) {
			if (DEBUG) {
				Serial.println("Debug mode disabled");
				DEBUG = false;
			}
			else {
				Serial.println("Debug mode enabled");
				DEBUG = true;
			}
		}
		else
			Serial.println("Valid flags: -s -c -d -m -r -D -W -p");
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
		Serial.print("Connection status: ");
		
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
	
		Serial.print("Active broadcasts: ");
		Serial.println(active_broadcasts_index);
		Serial.print("RSSI of AP hosting this module: ");
		Serial.println(wifi_station_get_rssi());
		Serial.print("Number of stations connected to ESP AP: ");
		Serial.println(wifi_softap_get_station_num());
		Serial.print("Random number: ");
		Serial.println(gen_random(256));
		Serial.print("Free space on heap:: ");
		Serial.println(system_get_free_heap_size());
		Serial.print("TCP remote IP address: ");
		Serial.println(ip_to_string(tcp.get_remote_ip()));
		Serial.print("WIFI PHY mode: ");
		switch (wifi_get_phy_mode()) {
			case 1:
				Serial.println("802.11b");
				break;
			case 2:
				Serial.println("802.11g");
				break;
			case 3:
				Serial.println("802.11n");
				break;
		}
		ret = "done";
  	}
	else if (!strcmp(com.substring(0, 4).c_str(), "udp ")) {
		if (!strcmp(com.substring(4, 6).c_str(), "-m"))	{							// multicast 
			String udp_packet = udp.make_packet(com.substring(7));
			udp.send_multicast(udp_packet);
			if (DEBUG) {
				Serial.println(udp_packet);
				Serial.println("sent");
			}
		}
		else if (!strcmp(com.substring(4, 6).c_str(), "-r"))						// restart server
			udp.restart_server();
		else if (!strcmp(com.substring(4, 6).c_str(), "-c"))						// change multicast port
			udp.set_multicast(MULTICAST_IP, atoi(com.substring(7).c_str()));
		else if (!strcmp(com.substring(4, 6).c_str(), "-s"))						// print status values
			Serial.println(udp.get_info());
		else {																						// send unicast to IP
			String udp_packet = udp.make_packet(com.substring(7 + com.substring(6).indexOf(' ')));
			udp.send_unicast(udp_packet, string_to_ip(get_ip_string(com.substring(4)))); //GET PAYLOAD
			if (DEBUG) {
				Serial.println("Sending UDP unicast");
				Serial.println(udp_packet);
				Serial.println("sent");		
			}
		}
		ret = "done";
	}
	else if (!strcmp(com.substring(0, 4).c_str(), "tcp ")) {
		String tcp_packet;
		if (!strcmp(com.substring(4, 6).c_str(), "-b")) {					// tcp broadcast
			
			DEWDBroadcast br(random(100, 256));
			String command_string = com.substring(7);
			command_string.replace('\n', '\0');
			
			if (WiFi.localIP()[0] != 0)
				br.src_ip = WiFi.localIP();
			else
				br.src_ip= WiFi.softAPIP();
			
			active_broadcasts[++active_broadcasts_index] = br;
			
			broadcast(command_string, br);
			
			active_broadcasts[active_broadcasts_index].resp_message += mac_string(true);
			active_broadcasts[active_broadcasts_index].resp_message += " ";
			active_broadcasts[active_broadcasts_index].resp_message += execute_broadcast(command_string);
			active_broadcasts[active_broadcasts_index].resp_message += ";"; 
		}
		else if (!strcmp(com.substring(4, 6).c_str(), "-s"))										// print tcp status data
			Serial.println(tcp.get_info());
		else if (!strcmp(com.substring(4, 6).c_str(), "-B")) {										// clear all active broadcasts
			active_broadcasts_index = 0;
		}
		else if (!strcmp(com.substring(4, 6).c_str(), "-r"))										// restart TCP server
			tcp.restart_server();
		else {																						// send to IP
			if (WiFi.localIP()[0] != 0)
				tcp_packet = tcp.make_packet('M', WiFi.localIP(), com.substring(7 + com.substring(6).indexOf(' ')), random(100,256));
			else
				tcp_packet = tcp.make_packet('M', WiFi.softAPIP(), com.substring(7 + com.substring(6).indexOf(' ')), random(100,256));
			
			if (DEBUG) {
				Serial.print("Sending tcp to ");
				Serial.println(get_ip_string(com.substring(4)));
				Serial.println(tcp_packet);
			}
			
			if (tcp.send_by_ip(tcp_packet, string_to_ip(get_ip_string(com.substring(4)))))
				Serial.println("sent");
			else
				Serial.println("failed");
		}
		ret = "done";
	}
	else {
		ret = com;
	}
	if (DEBUG)
		Serial.println(ret);
}
#endif