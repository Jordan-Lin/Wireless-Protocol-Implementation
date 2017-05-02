#ifndef PHYSICALREAD_HPP
#define PHYSICALREAD_HPP

#include "Globals.hpp"
#include "crc.hpp"
#include "PhysicalWrite.hpp"
#include "Datalink.hpp"

BOOL WaitForTrandTimeout();
BOOL WaitForAck();
DWORD getTrand();
BOOL ReadChar(CONST CHAR expectedChar, DWORD timeout);
BOOL waitForPacket(DWORD timeout);


#endif
