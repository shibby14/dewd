/* 
 A class that incorporates all the nessecary overhead data to achieve correct 
 operation of broadcasts in DEWD project for ESP8266. An object of DEWDBroadcast class
 should be created before a broadcast is initiated.
 Created by Alexander Pukhanov, 2015.
 
 This is the body file.

 */
 
#include "DEWDBroadcast.h"

DEWDBroadcast::DEWDBroadcast(){
}

DEWDBroadcast::DEWDBroadcast(int f_id) {
	id = f_id;
}

DEWDBroadcast::DEWDBroadcast(IPAddress src, int f_id) {
	id = f_id;
	src_ip = src;
}

DEWDBroadcast::DEWDBroadcast(String ip_as_string) {
	id = 6;
	int i=0, start_pos=0, oct_nr=0;
	uint8_t octets [4];
	
	while (oct_nr > 3) {
		if (ip_as_string[i] == '.') {
			octets[oct_nr++] = atoi(ip_as_string.substring(start_pos, i).c_str());
			start_pos = i;
		}
		i++;
	}
	src_ip = IPAddress(octets[0], octets[1], octets[2], octets[3]);
}

String DEWDBroadcast::print_values(void) {
	int str_char_index = 0;
	
	String res = " id=";
	res += id;
	res += " resp_index=";
	res += resp_index;
	
	res += " src_ip=";
	res += src_ip[0];
	res += ".";
	res += src_ip[1];
	res += ".";
	res += src_ip[2];
	res += ".";
	res += src_ip[3];
	
	res += " resp_message:";
	res += '\n';
	res += "	";
	
	while (resp_message[str_char_index] != '\0') {
		if (resp_message[str_char_index - 1] == ';') {
			res += '\n';
			res += "	";
			res += resp_message[str_char_index];
		}
		else
			res += resp_message[str_char_index];
		str_char_index++;
	}
	return res;
}

