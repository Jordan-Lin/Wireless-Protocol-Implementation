#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <iostream>
#include <string>
#include <deque>
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <fstream>
#include <vector>
#include <queue>
#include <random>
#include <inttypes.h>
#include <atomic>
#include <future>
#include <chrono>
#include <string.h>
#include <fstream>

#define TIDLE 1000
//#define TACK 125
#define TENQ 125
#define TTX 1000
#define TRX TTX * 3
//#define TWAIT TRX * 3

#define EOT		0x04
#define ENQ		0x05
#define SYN		0x16
#define ACK		0x06
#define DC1		0x11

#define DATA_SIZE 1024
#define PACKET_SIZE 1027

extern HANDLE hComm;
extern HANDLE sendEvent;
extern HANDLE threadEvent;
extern std::atomic<bool> connection;
extern std::queue<char*> sendingBuffer;

extern std::atomic<int> ackCountR;
extern std::atomic<int> ackCountS;
extern std::atomic<int> packetSentCount;
extern std::atomic<int> packetRecievedCount;
extern std::atomic<int> badPacketCount;

#endif // !GLOBALS_HPP
