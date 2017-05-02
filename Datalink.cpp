#include "Datalink.hpp"
/* ------------------------------------------------------------------------------------------
--  SOURCE FILE: Datalink.cpp
--
--  PROGRAM:     Wireless Protocol
--
--  FUNCTIONS:
--               generatePacket
--               readPacket
--				 errorCheckPacket
------------------------------------------------------------------------------------------------*/



/*-----------------------------------------------------------------------------------------------
 FUNCTION:     errorCheckPacket

 RETURNS:      (bool) True if the packet is valid, false otherwise.

 PARAMS:       (const char*) packet: A non null terminated char array. 

 NOTES:
                A valid packet has a SYN byte at index 0, 1027 total bytes, 
				and returns 0 when passed to the CRC function (crcSlow).
-----------------------------------------------------------------------------------------------*/
bool errorCheckPacket(const char* packet) {
	if (packet[0] == SYN) {
		char* data = new char[1026];
		for (size_t i = 0; i < 1026; i++) {
			data[i] = packet[i + 1];
		}
		int c = calculateCRC(data, 1026);
		if (c == 0) {
			return true;
		}
	}
	return false;
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     readPacket

DESCRIPTION:  Places "packet," into the processing buffer that the processing thread
			  later prints to the GUI.

RETURNS:      (VOID)

PARAMS:       (const char*) packet: Packet to be placed in buffer for processing.

NOTES:		  Multiple threads are writing to and reading from processing buffer,
			  so a mutex lock is used to ensure thread safety.
-----------------------------------------------------------------------------------------------*/
VOID readPacket(const char* packet) {
	char* text = new char[1024];
	for (size_t i = 0; i < 1024; i++) {
		text[i] = packet[i + 1];
	}
	mtx.lock();
	for (size_t i = 0; i < 1024; ++i) {
		if (text[i] == NULL) {
			processingBuffer.emplace(NULL);
			break;
		}
		else if (text[i] == DC1) {
			processingBuffer.emplace(DC1);
			break;
		}
		processingBuffer.emplace(text[i]);
	}
	mtx.unlock();
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     generatePacket

DESCRIPTION:  Packetizes data to the protocol specification.

RETURNS:      (char *) The packetized buffer.

PARAMS:       (const char[]) buffer: A non null terminated char array to packetize.

NOTES:		  The protocol states a valid packet contains 1027 bytes, 1 SYN byte at the 
			  beginning, 1024 bytes of data, and a crc in the last two bytes.
-----------------------------------------------------------------------------------------------*/
char* generatePacket(const char buffer[]) {
	char* packet = new char[1027];
	packet[0] = SYN;
	for (int i = 0; i < 1024; ++i) {
		packet[i + 1] = buffer[i];
	}
	crc tc = calculateCRC(buffer, 1024);
	packet[1025] = tc & 0xff;
	packet[1026] = (tc >> 8) & 0xff;
	return packet;
} 

// WORKS WITH EVAS
/*
char* generatePacket(const char buffer[]) {
char* packet = new char[1027];
packet[0] = SYN;
for (int i = 0; i < 1024; ++i) {
packet[i + 1] = buffer[i];
}
crc tc = crcSlow(buffer, 1024);
packet[1025] = (tc >> 8);
packet[1026] = (0x00FF & tc);
return packet;
}
*/
// ORG 
 
// BAD
/*
bool errorCheckPacket(const char* packet) {
if (packet[0] == SYN) {
char* data = new char[1024];
for (size_t i = 0; i < 1024; i++) {
data[i] = packet[i + 1];
}
char* datacrc = new char[2];
for (size_t i = 0; i < 2; i++) {
datacrc[i] = packet[1025+i];
}
crc c = crcSlow(data, 1024);
crc packcrc = datacrc[0] << 8;
packcrc += 0x00FF & datacrc[1];
if (!(crcSlow(data, 1024) - packcrc)) {
std::cout << "That a good pack" << std::endl;
return true;
}
std::cout << "crc failed " << c << std::endl;
}
std::cout << "Discard Packet" << std::endl;
return false;
}
*/
