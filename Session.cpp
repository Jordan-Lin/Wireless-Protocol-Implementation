#include "Session.hpp"
/* ------------------------------------------------------------------------------------------
--  SOURCE FILE: Session.cpp
--
--  PROGRAM:     Wireless Protocol
--
--  FUNCTIONS:
--               run
--               idle
--				 writeBuffer
------------------------------------------------------------------------------------------------*/

HANDLE hComm;
HANDLE sendEvent;
std::thread filewrite;
std::mutex mtx;
std::queue<char> processingBuffer;
std::atomic<bool> connection(false);

/*-----------------------------------------------------------------------------------------------
FUNCTION:     run

DESCRIPTION:  Initializes comm port, creates sendEvent, and starts protocol
              engine (call to idle).

PARAMS:       (CHAR) c: The character to write.

RETURNS:      (int) 0 on completion. 1 on failure to build DCB, 2 on failure
                    to set comm state, and 3 on failure to connect serial port.
-----------------------------------------------------------------------------------------------*/
int run() {
	LPSTR lpszCommName = "COM1";
	DCB mydcb;
	if (BuildCommDCB("96,N,8,1", &mydcb) == false) {
		updateRecieveStatus("Status:");
		updateSendStatus("Status:");
		SetEvent(threadEvent);
		return 1;
	}
	if (SetCommState(hComm, &mydcb)) {
		updateRecieveStatus("Status:");
		updateSendStatus("Status:");
		SetEvent(threadEvent);
		return 2;
	}
	if ((hComm = CreateFile(lpszCommName, GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE) {
		updateRecieveStatus("Status:");
		updateSendStatus("Status:");
		SetEvent(threadEvent);
		return 3;
	}

	sendEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	filewrite = std::thread(writeBuffer);

	idle();
	filewrite.join();
	CloseHandle(hComm);
	updateRecieveStatus("Status:");
	updateSendStatus("Status:");
	SetEvent(threadEvent);
	return 0;
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     idle

DESCRIPTION:  Runs idle sequence. 

RETURNS:      (VOID)
-----------------------------------------------------------------------------------------------*/
VOID idle() {
	COMMPROP cp;
	COMMTIMEOUTS to;
	DWORD lrc;

	memset(&to, 0, sizeof(to));
	SetCommTimeouts(hComm, &to);

	DWORD idleSeqTimeout = 0;
	while (connection) {
		OVERLAPPED o = { 0 };
		o.hEvent = CreateEvent(NULL,  /* lpsa */
			TRUE,  /* manual reset */
			FALSE, /* init state non sig */
			NULL);
		BYTE inbuff[1];
		DWORD nBytesRead = 0;
		BOOL readFileResult = ReadFile(hComm, inbuff, 1, &nBytesRead, &o);
		if (readFileResult == FALSE || (lrc = GetLastError()) == ERROR_IO_PENDING) {
			//not 1 byte in buffer or failed io
			HANDLE handles[2];
			handles[0] = o.hEvent;
			handles[1] = sendEvent;

			DWORD waitResult = WaitForMultipleObjects(2, handles, FALSE, TIDLE);
			if (waitResult - WAIT_OBJECT_0 == 0) {
				//byte arrived in buffer
				if (GetOverlappedResult(hComm, &o, &nBytesRead, FALSE)) {
					idleSeqTimeout = 0;																			// IDLE SEQ CHANGED TO 0
					if (nBytesRead == 1 && inbuff[0] == ENQ) {
						if (WriteChar(ACK)) {
							if (waitForPacket(TRX)) {
								WriteChar(ACK);
							}
						}
					}
				}
			}
			else if (waitResult - WAIT_OBJECT_0 == 1) {
				//ResetEvent(sendEvent);
				//send event triggered
				CancelIo(hComm);
				if (WaitForTrandTimeout()) {
					idleSeqTimeout = 0;
				} else {
					idleSeqTimeout++;
				}
			}
			else if (waitResult == WAIT_TIMEOUT) {
				//TIDLE timedout
				CancelIo(hComm);
				if (ReadChar(ENQ, getTrand()) == FALSE) {
					if (WriteChar(ENQ)) {
						if (ReadChar(ACK, TENQ)) {
							HANDLE waitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	
							WaitForSingleObject(waitEvent, TRX);
							std::cout << "GOT AzCK BRO" << std::endl;
							idleSeqTimeout = 0;
						}
						else {
							idleSeqTimeout++;
						}
					}
				}
				else {
					idleSeqTimeout = 0;																			// IDLE SEQ CHANGED TO 0
					if (WriteChar(ACK)) {
						std::cout << "prepare to read" << std::endl;
						if (waitForPacket(TRX)) {
							WriteChar(ACK);
						}
					}
				}
			}
		}
		else {																				// IDLE SEQ CHANGED TO 0
			if (nBytesRead == 1 && inbuff[0] == ENQ) {
				idleSeqTimeout = 0;
				if (WriteChar(ACK)) {
					if (waitForPacket(TRX)) {
						WriteChar(ACK);
					}

				}
			} else {
				idleSeqTimeout++;
			}
		}

		ResetEvent(o.hEvent);
		PurgeComm(hComm, PURGE_RXCLEAR);
		if (idleSeqTimeout > 5) {
			resetProtocol();
		}
	}
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     writeBuffer

DESCRIPTION:  Writes receiving buffer to files.

RETURNS:      (VOID)
-----------------------------------------------------------------------------------------------*/
VOID writeBuffer() {
	std::ofstream outfile;
	std::ifstream file;
	std::string fileName = "file_";
	std::string currentFile = "";

	while (!processingBuffer.empty() || connection) { 
		unsigned int writecount = 0;
		mtx.lock();
		while (!processingBuffer.empty() && writecount++ < 20) {
			char c = processingBuffer.front();
			if (c == DC1) {
				if (outfile.is_open()) {
					outfile.close();
				}
				int fc = 1;
				while (1) {
					currentFile = fileName + std::to_string(fc) + ".txt";
					file.open(currentFile.c_str()); 
					if (file.good()) {
						file.close();
						fc++;
					}
					else {
						break;
					}
				}
				outfile.open(currentFile);
				updateRecieveStatus("Status: Recieving File...");
				if (!outfile.is_open()) {
					updateRecieveStatus("ERROR: File not created!");
				}
			}
			else if (c == NULL) {
				if (outfile.is_open()) {
					outfile.close();
					updateRecieveStatus("Status: File Recieved");
					displayReceivedFile(currentFile.c_str());
					currentFile = "";
				}
			}
			else {
				if (outfile.is_open()) {
					outfile << c << std::flush;
				}
			}
			processingBuffer.pop();
		}
		mtx.unlock();
	}
	if (outfile.is_open()) {
		outfile.close();
	}
}

