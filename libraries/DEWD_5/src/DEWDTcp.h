/*
 DEWDTcp.h Header file defining TCP client-server functionality.
 Created by Alexander Pukhanov, 2015.
 
 */
 
#ifndef DEWDTcp_h
#define DEWDTcp_h

#include <MACAddress.h>
#include <IPAddress.h>
#include <WString.h>
#include <ESP8266WiFi.h>
#include <WiFiServer.h>

class DEWDTcpClass
{
private:

	uint16 _port = 4040;												// TCP port
	WiFiClient _client;
	WiFiServer _server = WiFiServer(_port);

public:
	DEWDTcpClass(int port);
	String make_packet(char flag, IPAddress src_ip, String payload, int id);
	void start_server();
	void restart_server();
	bool send_by_ip(String str, IPAddress dest);
	bool send_by_mac(String str, MACAddress dest);
	String listen();
	String parse(String s);
	String get_info();
	IPAddress get_remote_ip();
};
#endif