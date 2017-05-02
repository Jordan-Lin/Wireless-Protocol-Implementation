#ifndef DATALINK_HPP
#define DATALINK_HPP

#include "Globals.hpp"
#include "crc.hpp"
#include "Session.hpp"

VOID readPacket(const char* packet);
bool errorCheckPacket(const char* packet);
char* generatePacket(const char buffer[]);

#endif // !DATALINK_HPP
