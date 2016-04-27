/*
 DEWDUdp.cpp
 Created by Alexander Pukhanov, 2015.
 
 */
 
#include <IPAddress.h>
#include <WString.h>
#include <DEWDUdp.h>
#include <ESP8266WiFi.h>
 
ICACHE_FLASH_ATTR DEWDUdpClass::DEWDUdpClass(int port, IPAddress multicast_group, int multicast_port) {
	_port = port;
	_multicast_group_ip = multicast_group;
	_multicast_group_port = multicast_port;
	start_server();
}

void ICACHE_FLASH_ATTR DEWDUdpClass::set_multicast(IPAddress new_ip, int new_port) {
	_Mudp.stop();
	_multicast_group_ip = new_ip;
	_multicast_group_port = new_port;
	_Mudp.beginMulticast(WiFi.localIP(), new_ip, new_port);
}

void ICACHE_FLASH_ATTR DEWDUdpClass::start_server() {	
	_Mudp.beginMulticast(WiFi.localIP(), _multicast_group_ip, _multicast_group_port);
	_udp.begin(_port);
}

void ICACHE_FLASH_ATTR DEWDUdpClass::restart_server() {	
	_Mudp.stop();
	_udp.stop();
	
	_Mudp.beginMulticast(WiFi.localIP(), _multicast_group_ip, _multicast_group_port);
	_udp.begin(_port);
}

String ICACHE_FLASH_ATTR DEWDUdpClass::make_packet(String str) {
	String ret = "U ";
	uint8 id = random(100, 256);
	_broadcast_id =id;
	ret += id;
	ret += " ";
	if (str.indexOf('\n'))														// new-line char is copied along with text from UART
		str.replace('\n', '\0');
	ret += str;
	return ret;
}

void ICACHE_FLASH_ATTR DEWDUdpClass::send_unicast(String str, IPAddress dest) {
	_udp.beginPacket(dest, _port);
	_udp.write(str.c_str(), strlen(str.c_str()));
	_udp.endPacket();
}

void ICACHE_FLASH_ATTR DEWDUdpClass::send_multicast(String str) {
	_Mudp.beginPacket(_multicast_group_ip, _multicast_group_port);
	_Mudp.write(str.c_str(), strlen(str.c_str()));
	_Mudp.endPacket();
}

String ICACHE_FLASH_ATTR DEWDUdpClass::parse(String s) {
	return s.substring(6);
}

String ICACHE_FLASH_ATTR DEWDUdpClass::get_info() {
	String ret = " broadcast_id=";
	ret += _broadcast_id;
	ret += "\n port=";
	ret += _port;		
	ret += "\n multicast_group_ip=";
	String ip;
	for (int i=0; i<3;i++) {
		ip += _multicast_group_ip[i];
		ip += '.';
	}
	ip += _multicast_group_ip[3];
	ret += ip;
	ret +="\n multicast_port=";
	ret += _multicast_group_port;
	return ret;
}

String DEWDUdpClass::listen() {
	unsigned char buff[512]; 
	String ret;
	
	int cb = _udp.parsePacket();	
	if (cb) {
		/*ret += "Unicast packet received, length=";
		ret += cb;
		ret += " received from ";
		IPAddress rem = _udp.remoteIP();
		for (int i=0; i<3;i++) {
			ret += rem[i];
			ret += '.';
		}
		ret += rem[3];		
		ret += ":";
		ret += _udp.remotePort();
		ret += '\n';*/
		
		int a = _udp.read(buff, cb);
		_udp.flush(); 
		buff[cb] = '\0';
		String str1(reinterpret_cast<char*>(buff));
		ret += str1;
	}
	
	int Mcb = _Mudp.parsePacket();
	if (Mcb) {
		/*ret += "Multicast packet received, length=";
		ret += Mcb;
		ret += " received from ";	
		IPAddress rem = _Mudp.remoteIP();
		for (int i=0; i<3;i++) {
			ret += rem[i];
			ret += '.';
		}
		ret += rem[3];		
		ret += ":";
		ret += _Mudp.remotePort();
		ret += '\n';*/
		
		int b = _Mudp.read(buff, Mcb);
		_Mudp.flush(); 
		buff[Mcb] = '\0';
		String str1(reinterpret_cast<char*>(buff));
		
		int udp_id = atoi(str1.substring(1, 5).c_str());
		if (_broadcast_id != udp_id) {										// check that this multicast hasn't been received already
			send_multicast(str1);												// forward it along...
			_broadcast_id = udp_id;											// change id to prevent echoes
			ret += str1;
		}
		else																			// multicast echo - ignore message
			ret += String();
	}
	return ret;
}