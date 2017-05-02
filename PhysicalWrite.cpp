#include "PhysicalWrite.hpp"
/* ------------------------------------------------------------------------------------------
--  SOURCE FILE: PhysicalWrite.cpp
--
--  PROGRAM:     Wireless Protocol
--
--  FUNCTIONS:
--               WriteChar
--               Write
--				 SendPacket
------------------------------------------------------------------------------------------------*/

std::queue<char*> sendingBuffer;

/*-----------------------------------------------------------------------------------------------
FUNCTION:     WriteChar

DESCRIPTION:  Writes a character to the serial port.

PARAMS:       (CHAR) c: The character to write.

RETURNS:      (BOOL) True if write is successful, false if not.

NOTES:        Essentially just a wrapper for the write function that allows use
              of defined characters and doesn't require length param.
-----------------------------------------------------------------------------------------------*/
BOOL WriteChar(CONST CHAR c) {
	if (c == ACK) {
		ackCountS++;
	}
	LPCSTR str = &c;
	return Write(str, 1);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     Write

DESCRIPTION:  Writes the outbuff param to the serial port.

PARAMS:       (CONST LPCSTR) outbuff: Non-null terminated char array to write
                                      to the serial port.
			  (DWORD) length:         Length of the param outbuff.

RETURNS:      (BOOL) True if write is successful, false if not.
-----------------------------------------------------------------------------------------------*/
BOOL Write(CONST LPCSTR outbuff, DWORD length) {
	OVERLAPPED o = { 0 };
	o.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	DWORD bytesWritten = 0;
	WriteFile(hComm, outbuff, length, &bytesWritten, &o);
	return GetOverlappedResult(hComm, &o, &bytesWritten, TRUE);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     SendPacket

DESCRIPTION:  Writes a packet to the serial port, starts a timer at TTX,
              and waits for an ACK. If an ACK is not received before TTX
			  elapses it tries again. Tries a max of 3 times.

RETURNS:      (BOOL) True if packet is sent and ACK is received. False otherwise.
-----------------------------------------------------------------------------------------------*/
VOID SendPacket() {
	char *outbuffer = sendingBuffer.front();
	DWORD sendCount = 0;
	while (sendCount++ < 3) {
		if (Write(outbuffer, 1027)) {
			if (ReadChar(ACK, TTX)) {
				packetSentCount++;
				sendingBuffer.pop();
				ResetEvent(sendEvent);
				if (!sendingBuffer.empty()) {
					SetEvent(sendEvent);
				}
				else {
					updateSendStatus("Status: Sending Complete");
				}
			}
		}
	}
}

