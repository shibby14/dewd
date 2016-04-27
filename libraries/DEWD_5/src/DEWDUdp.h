/*
 DEWDUdp.h
 Created by Alexander Pukhanov, 2015.
 
 */
 
#ifndef DEWDUdp_h
#define DEWDUdp_h

#include <MACAddress.h>
#include <WiFiUDP.h>
#include <IPAddress.h>
#include <WString.h>

class DEWDUdpClass
{
private:
	int _broadcast_id;
	
	int _port = 5555;												// UDP port
	int _multicast_group_port = 5556;
	
	IPAddress _multicast_group_ip = IPAddress(224,1,1,1);
	
	WiFiUDP _udp;
	WiFiUDP _Mudp;

public:
	DEWDUdpClass(int port, IPAddress multicast_group, int multicast_port);
	void set_multicast(IPAddress new_ip, int new_port);
	void start_server();
	void restart_server();
	String make_packet(String str);
	void send_unicast(String str, IPAddress dest);
	void send_multicast(String str);
	String parse(String s); 
	String get_info();
	String listen();
};
#endif