#pragma once
/**********************************************************************
*
* Filename:    crc.h
*
* Description: A header file describing the various CRC standards.
*
* Notes:
*
*
* Copyright (c) 2000 by Michael Barr.  This software is placed into
* the public domain and may be used for any purpose.  However, this
* notice must not be changed or removed and no warranty is either
* expressed or implied by its publication or distribution.
**********************************************************************/

#ifndef CRC_H
#define CRC_H

/*
* Select the CRC standard from the list that follows.
*/
#define CRC16


#if defined(CRC16)

typedef unsigned short  crc;

#define CRC_NAME			"CRC-16"
#define POLYNOMIAL			0x8005
#define INITIAL_REMAINDER	0x0000
#define FINAL_XOR_VALUE		0x0000
#define REFLECT_DATA		TRUE
#define REFLECT_REMAINDER	TRUE
#define CHECK_VALUE			0xBB3D

#else

#endif

crc calculateCRC(char const message[], int nBytes);


#endif /* _crc_h */
