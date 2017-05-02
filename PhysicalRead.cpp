#include "PhysicalRead.hpp"
/* ------------------------------------------------------------------------------------------
--  SOURCE FILE: PhysicalRead.cpp
--
--  PROGRAM:     Wireless Protocol
--
--  FUNCTIONS:
--               ReadChar
--               WaitForTrandTimeout
--				 waitForPacket
--               getTrand
------------------------------------------------------------------------------------------------*/

std::random_device rd;     // only used once to initialise (seed) engine

/*-----------------------------------------------------------------------------------------------
 FUNCTION:     ReadChar

 DESCRIPTION:   Attempts to read a single character from the serial port for the timeout
                param and if a character is read, it checks if it is the expected character
				param.

 RETURNS:      (BOOL) True if a character was read and it is the expected character.
			   False if no character is read or the character read is not the 
			   expected character.

 PARAMS:       (CHAR) expectedChar
			   (DWORD) timeout: The time to spend attempting to read the char.
-----------------------------------------------------------------------------------------------*/
BOOL ReadChar(CONST CHAR expectedChar, DWORD timeout) {
	CHAR inbuff[1];
	OVERLAPPED o = { 0 };
	o.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	DWORD bytesRead = 0;
	DWORD readResult = ReadFile(hComm, inbuff, 1, &bytesRead, &o);
	if (GetLastError() == ERROR_IO_PENDING) {
		if (WaitForSingleObject(o.hEvent, timeout) == WAIT_OBJECT_0) {
			if (GetOverlappedResult(hComm, &o, &bytesRead, FALSE)) {
				if (bytesRead == 1 && inbuff[0] == expectedChar) {
					if (expectedChar == ACK) {
						ackCountR++;
					}
					return TRUE;
				}
			}
		}
		CancelIo(hComm);
	}
	else if (readResult == TRUE) {
		if (GetOverlappedResult(hComm, &o, &bytesRead, FALSE)) {
			if (bytesRead == 1 && inbuff[0] == expectedChar) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     ReadChar

DESCRIPTION:  Sets rand timer and watches for an ENQ. An ENQ is sent
              if timer elapses, an ACK is sent if an ENQ is received.
			  Once an ENQ is sent, and an ACK is read, a packet is sent.
			  Once an ENQ is read, an ACK is written, and we move to the
			  wait for packet stage. If none of these paths are followed
			  more than 3 times, the function gives up.

RETURNS:      (BOOL) True if send or receive packet, false otherwise.
-----------------------------------------------------------------------------------------------*/
BOOL WaitForTrandTimeout() {
	DWORD noAckCount = 0;
	while (noAckCount < 3) {
		if (ReadChar(ENQ, getTrand())) {
			if (WriteChar(ACK)) {
				if (waitForPacket(TRX)) {
					WriteChar(ACK);
					return TRUE;
				}
			}
		}
		else {
			WriteChar(ENQ);
			if (ReadChar(ACK, TENQ)) {
				SendPacket();
				return TRUE;
			}
			else {
				noAckCount++;
			}
		}
	}
	return FALSE;
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     getTrand

RETURNS:      (DWORD) a random number between the values specified by the protocol spec.
-----------------------------------------------------------------------------------------------*/
DWORD getTrand() {
	std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni(1, 100); // guaranteed unbiased
	return uni(rng);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     waitForPacket

DESCRIPTION:  Attempts to read a packet 3 times. If timeout is reached while waiting
			  for a packet, it stops trying.

PARAMS:       (DWORD) timeout: The time to wait for the packet.

RETURNS:      (BOOL) True if a good packet is read, false otherwise.
-----------------------------------------------------------------------------------------------*/
BOOL waitForPacket(DWORD timeout) {
	int TRx_attempts = 0;
	while (TRx_attempts++ < 3) {
		CHAR inbuff[1027];
		OVERLAPPED o = { 0 };
		o.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		DWORD bytesRead = 0;
		DWORD readResult = ReadFile(hComm, inbuff, 1027, &bytesRead, &o);
		if (GetLastError() == ERROR_IO_PENDING) {
			if (WaitForSingleObject(o.hEvent, timeout) == WAIT_OBJECT_0) {
				if (GetOverlappedResult(hComm, &o, &bytesRead, FALSE)) {
					if (bytesRead == 1027) {
						if (errorCheckPacket(inbuff)) {
							packetRecievedCount++;
							readPacket(inbuff);
							return TRUE;
						} else {
							badPacketCount++;
						}
					}
				}
			}
			else {
				CancelIo(hComm);
				return FALSE;
			}
		}
		else if (readResult == TRUE) {
			if (GetOverlappedResult(hComm, &o, &bytesRead, FALSE)) {
				if (bytesRead == 1027) {
					if (errorCheckPacket(inbuff)) {
						readPacket(inbuff);
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}


