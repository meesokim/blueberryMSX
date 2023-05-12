/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/MSXBUS.cpp,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-03-30 18:38:40 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
extern "C" {
#include "MsxBus.h"
#include "RomLoader.h"
extern void ledSetSlot1Busy();
extern void ledSetSlot2Busy();
extern void checkInt(void);
};
//#define FAKE_ROM
enum {
	// -- cmd0: read memory no slot
	RD_MEM,
	// -- cmd1: read memory slot#1 
	RD_SLOT1,
	// -- cmd2: read memory slot#2
	RD_SLOT2,
	// -- cmd3: read io 
	RD_IO, 
	// -- cmd4: write memory no slot 
	WR_MEM,
	// -- cmd5: write memory slot#1
	WR_SLOT1,
	// -- cmd6: write memory slot#2
	WR_SLOT2,
	// -- cmd7: write io	
	WR_IO
};

typedef unsigned char uint8_t;
class CMSXBUS
{
public:
	CMSXBUS(int slot = 0) { this->slot = slot;};
	~CMSXBUS() {};

    inline int readMemory(UInt16 address) 
	{
		uint8_t b = 0xff;
		return b; 
	};
    inline int writeMemory(UInt16 address, UInt8 value) 
	{ 
		return 0; 
	};
	inline int readIo(UInt16 port) 
	{ 
		uint8_t b = 0xff;
		return b; 
	};
    inline int writeIo(UInt16 port, UInt8 value) 
	{ 
		return 0; 
	};
private:
	int slot;
};


static CMSXBUS* MSXBUSs[2] = { NULL, NULL };

static void InitializeMSXBUSs()
{
    if (MSXBUSs[0] == NULL) {
        MSXBUSs[0] = new CMSXBUS(0);
	printf("MSXBUSs[0]=%d\n", MSXBUSs[0]);
	MSXBUSs[1] = new CMSXBUS(1);
	printf("MSXBUSs[1]=%d\n", MSXBUSs[1]);
    }
}

static void DeinitializeMSXBUSs()
{
    if (MSXBUSs[0]!= NULL) {
#ifndef WIN32		
		// msxclose();
#endif		
        delete MSXBUSs[0];
		delete MSXBUSs[1];
    }
}



/////////////////////////////////////////////////////////////
//
// Public C interface

extern "C" MbHandle* msxBusCreate(int cartSlot, int slot)
{
	InitializeMSXBUSs();
	printf("msxBusCreate %d\n", cartSlot, slot);
	return (MbHandle*)MSXBUSs[cartSlot];
	return 0;
}

extern "C" void msxBusDestroy(MbHandle* mpHandle)
{
    DeinitializeMSXBUSs();
}

extern "C" int msxBusRead(MbHandle* mpHandle, UInt16 address)
{
    return ((CMSXBUS*)mpHandle)->readMemory(address);
}

extern "C" int msxBusWrite(MbHandle* mpHandle, UInt16 address, UInt8 value)
{
    return ((CMSXBUS*)mpHandle)->writeMemory(address, value);
}

extern "C" int msxBusReadIo(MbHandle* mpHandle, UInt16 port)
{
    return ((CMSXBUS*)mpHandle)->readIo(port);
}

extern "C" int msxBusWriteIo(MbHandle* mpHandle, UInt16 port, UInt8 value)
{
    return ((CMSXBUS*)mpHandle)->writeIo(port, value);
}

extern "C" int msxBusSupported()
{
#ifdef WII
    return 0;
#else
    return 1;
#endif
}
