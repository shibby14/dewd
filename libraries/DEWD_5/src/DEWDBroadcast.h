/* 
 A class that incorporates all the necessary overhead data to achieve correct 
 operation of broadcasts in DEWD project for ESP8266. An object of DEWDBroadcast class
 should be created before a broadcast is initiated.
 Created by Alexander Pukhanov, 2015.
 
 */
 
#ifndef DEWDBroadcast_h
#define DEWDBroadcast_h

#include <IPAddress.h>
#include <WString.h>
class DEWDBroadcast {
    public:
			
		uint8_t id;						// 8-bit, 3-digit ID (100-255) for unique identification of B messages 
		uint8_t resp_index = 0;			// number indicating how many messages sent and how many responses to expect back
		IPAddress src_ip;				// the IP of the originating broadcast
		String resp_message;			// response to be sent to src_ip. Can be a combination of multiple responses
	
        // Constructors
        DEWDBroadcast();
		DEWDBroadcast(int f_id);		
		DEWDBroadcast(IPAddress src, int f_id);			
		DEWDBroadcast(String ip_as_string);
		
		String print_values(void);		// print function for debugging
};

#endif 