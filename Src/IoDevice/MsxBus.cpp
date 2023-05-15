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
#include "zmx.h"
#include "zmxbus.h"
extern void ledSetSlot1Busy();
extern void ledSetSlot2Busy();
extern void checkInt(void);
};
#define ZMX_DRIVER "./zmxbus" ZEMMIX_EXT 
typedef unsigned char uint8_t;
static ReadfnPtr msxread;
static WritefnPtr msxwrite;
static InitfnPtr msxinit;
static ResetfnPtr resetz;	
static void *hDLL = 0;	
class CMSXBUS
{
public:
	static void initialize() {
		if (hDLL)
			CloseZemmix(hDLL);
		hDLL = OpenZemmix((char*)ZMX_DRIVER, RTLD_LAZY);
		if (!hDLL)
		{
			printf("DLL open error!!\n");
			exit(1);
		}	
		msxread = (ReadfnPtr)GetZemmixFunc(hDLL, (char*)MSXREAD);
		msxwrite = (WritefnPtr)GetZemmixFunc(hDLL, (char*)MSXWRITE);
		msxinit = (InitfnPtr)GetZemmixFunc(hDLL, (char*)MSXINIT);
		resetz = (ResetfnPtr)GetZemmixFunc(hDLL, (char*)MSXRESET);            
		msxinit("");
		resetz();		
	}
	CMSXBUS(int slot = 0) { this->slot = slot;};
	~CMSXBUS() {};

    inline int readMemory(UInt16 address) 
	{
		uint8_t b = 0xff;
		b = msxread(RD_SLOT1+slot, address);		
		return b; 
	};
    inline int writeMemory(UInt16 address, UInt8 value) 
	{ 
		msxwrite(WR_SLOT1+slot, address, value);
		return 0; 
	};
	inline int readIo(UInt16 port) 
	{ 
		uint8_t b = 0xff;
		b = msxread(RD_IO, port);
		return b; 
	};
    inline int writeIo(UInt16 port, UInt8 value) 
	{ 
		msxwrite(WR_IO, port, value);
		return 0; 
	};
private:
	int slot;
};

static CMSXBUS* MSXBUSs[2] = { NULL, NULL };

static void InitializeMSXBUSs()
{
    if (MSXBUSs[0] == NULL) {
		CMSXBUS::initialize();
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
