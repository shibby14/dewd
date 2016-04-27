/*
 DEWDTcp.cpp Body file defining TCP client-server functionality.
 Created by Alexander Pukhanov, 2015.
 
 */
 
#include <IPAddress.h>
#include <WString.h>
#include <DEWDTcp.h>
#include <ESP8266WiFi.h>
extern "C" {
#include "user_interface.h"
}

ICACHE_FLASH_ATTR DEWDTcpClass::DEWDTcpClass(int port) {
	_port = port;
	_server = WiFiServer(port);
	start_server();
}

void ICACHE_FLASH_ATTR DEWDTcpClass::start_server() {	
	_server.begin();
}

void ICACHE_FLASH_ATTR DEWDTcpClass::restart_server() {	
	_server = WiFiServer(_port);
	_server.begin();
}

String ICACHE_FLASH_ATTR DEWDTcpClass::make_packet(char flag, IPAddress src_ip, String payload, int id) {
	String ret;
	ret += flag;
	ret += " ";
	ret += id;
	
	// Make broadcast message, format: "B <id> <src_ip> <payload>"
	if (flag == 'B') {
		ret += " ";
		for (int i=0; i<3;i++) {
			ret += src_ip[i];
			ret += '.';
		}
		ret += src_ip[3];
		ret += " ";
		ret += payload;
	}
	// Make response message, format: "R <id> <payload>"
	else if(flag == 'R') {
		ret += " ";
		ret += payload;
	}
	// Make direct message, format: "M <id> <src_ip> <payload>"
	else if(flag == 'M') {
		ret += " ";
		for (int i=0; i<3;i++) {
			ret += src_ip[i];
			ret += '.';
		}
		ret += src_ip[3];
		ret += " ";
		ret += payload;
	}
	//else wrong-response message is created, format: "W <id>"
	
	if (ret.indexOf('\n'))														// new-line char is copied along with text from UART
		ret.replace('\n', '\0');
	return ret;
}

bool ICACHE_FLASH_ATTR DEWDTcpClass::send_by_ip(String str, IPAddress dest) {    
  if (_client.connect(dest, _port)) {
    _client.println(str);
	return true;
  }
  else {
	  return false;
  }
}

bool ICACHE_FLASH_ATTR DEWDTcpClass::send_by_mac(String str, MACAddress dest) {    
	uint8_t * macp;

	// check if dest = station interface
	wifi_get_macaddr(STATION_IF, macp);
	MACAddress mac(macp);
	if (mac == dest) {									
		return false;
	}
  
	// check if dest = softAP interface
	wifi_get_macaddr(SOFTAP_IF, macp);
	mac = MACAddress(macp);
	if (mac == dest) {									
		return false;
	}
	
	// check if host is dest
	struct station_config host_sta;
	wifi_station_get_config(&host_sta);
	if (host_sta.bssid_set != 0)
		mac = MACAddress(host_sta.bssid);
	else
		uint8 current_ap_id = wifi_station_get_current_ap_id();
	
	if (mac == dest) {
		if (send_by_ip(str, WiFi.gatewayIP()))					// REPLACE WITH NON-ESP8266WiFi FUNCTION!
			return true;
		return false;
	}
		

	// check if one of the clients is dest 
	int count = 0;
	struct station_info * station = wifi_softap_get_station_info();
	struct station_info * next_station;
	
	if (station == NULL) {
		return false;
	}
	  	  
     while(station) { 			
		mac = MACAddress(station->bssid);
		if (mac == dest) {
			if (send_by_ip(str, (&station->ip)->addr))
				return true;
			return false;
		}
		next_station = STAILQ_NEXT(station, next);
		station = next_station;
    }
	wifi_softap_free_station_info();
}

String DEWDTcpClass::listen() {
	_client = _server.available();
	if (!_client) 
		return "";
	
	String ret = _client.readStringUntil('\r');
	return ret;
}

String ICACHE_FLASH_ATTR DEWDTcpClass::parse(String s) {
	return s.substring( 7 + s.substring(6).indexOf(' '));
}

String ICACHE_FLASH_ATTR DEWDTcpClass::get_info() {
	String ret = " port=";
	ret += _port;		
	return ret;
}

IPAddress ICACHE_FLASH_ATTR DEWDTcpClass::get_remote_ip() {
	return IPAddress(_client.remoteIP());
}
