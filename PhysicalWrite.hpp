#ifndef PHYSICALWRITE_HPP
#define PHYSICALWRITE_HPP

#include "Globals.hpp"
#include "crc.hpp"
#include "PhysicalRead.hpp"

BOOL WriteChar(CONST CHAR c);
BOOL Write(CONST LPCSTR outbuff, DWORD length);
VOID SendPacket();


#endif
