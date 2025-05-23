/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperMsxBus.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2025-03-09 18:38:44 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2018-2025 Miso Kim
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
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include "romMapperMsxBus.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "zmx.h"
#include "zmxbus.h"
#include "SCC.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static FILE* f = NULL;

typedef struct {
    int deviceHandle;
    int slot;
    int sslot;
    int cart;
    SccMode sccMode;
    SCC* scc;
	int sccEnable;
} RomMapperMsxBus;

static ReadfnPtr msxread;
static WritefnPtr msxwrite;
static InitfnPtr msxinit;
static ResetfnPtr resetz;	
static StatusfnPtr msxstatus;
static void *hDLL = 0;	

static void saveState(RomMapperMsxBus* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperMsxBus");
    saveStateClose(state);
}

static void loadState(RomMapperMsxBus* rm)
{
    SaveState* state = saveStateOpenForRead("mapperMsxBus");
    saveStateClose(state);
}


static void destroy(RomMapperMsxBus* rm)
{
    if (hDLL)
        CloseZemmix(hDLL);    
    ioPortUnregisterUnused(rm->cart);
    slotUnregister(rm->slot, rm->sslot, 0);
    deviceManagerUnregister(rm->deviceHandle);
    free(rm);
}

static UInt8 readIo(RomMapperMsxBus* rm, UInt16 port)
{
   return msxread(RD_IO, port);
}

static void writeIo(RomMapperMsxBus* rm, UInt16 port, UInt8 value)
{
    msxwrite(WR_IO, port, value);
}

static UInt8 read(RomMapperMsxBus* rm, UInt16 address) 
{
#if 0
    if (rm->sccEnable  && ((address >= 0x9800 && address < 0xa000)) {
//		printf("rm->sccEnable = %d\n", rm->sccEnable);
        return sccRead(rm->scc, (UInt8)(address & 0xff));
    }
#endif	
    UInt8 ret;
    if (address > 0xbfff)
	ret = 0xff;
    else
        ret = msxread(rm->cart > 1 ? RD_SLTSL2 : RD_SLTSL1, address);
    //printf("r%04x: %02x\n", address, ret);
    return ret;
}

static void write(RomMapperMsxBus* rm, UInt16 address, UInt8 value) 
{
#if 0
	if (((address & 0x97ff) == 0x9000) && ((value & 0x3f) == 0x3f))
	{
		rm->sccEnable = 1;
	}
    else if (rm->sccEnable && (address >= 0x9800) && (address <= 0x98ff)) {
        sccWrite(rm->scc, address & 0xff, value);
    }
#endif
    // printf("w%04x: %02x\n", address, value);
    return msxwrite(rm->cart > 1 ? WR_SLTSL2 : WR_SLTSL1, address, value);
}

static void reset(RomMapperMsxBus* rm)
{
    printf("reset of RomMapperMsxBus\n");
    // sccReset(rm->scc);
    resetz(5);
}


static const int mon_ports[] = {}; // 0x7c, 0x7d, 0x7e, 0x7f, 0xa0, 0xa1, 0xa2, 0xa3, 0 };

#define ZMX_DRIVER ZEMMIX_BUS

static void initialize() {
    if (hDLL)
        CloseZemmix(hDLL);
    hDLL = OpenZemmix((char*)ZMX_DRIVER, RTLD_LAZY);
    if (!hDLL)
    {
        printf("DLL open error!! %s\n", ZMX_DRIVER);
        exit(1);
    }
    printf("Zemmix initialized\n");	
    msxread = (ReadfnPtr)GetZemmixFunc(hDLL, (char*)MSXREAD);
    msxwrite = (WritefnPtr)GetZemmixFunc(hDLL, (char*)MSXWRITE);
    msxinit = (InitfnPtr)GetZemmixFunc(hDLL, (char*)MSXINIT);
    resetz = (ResetfnPtr)GetZemmixFunc(hDLL, (char*)MSXRESET);   
    msxstatus = (StatusfnPtr)GetZemmixFunc(hDLL, (char*)MSXSTATUS);
    msxinit(0);
    resetz(0);
    sleep(1);
    resetz(1);		
}

int romMapperMsxBusCreate(int cartSlot, int slot, int sslot) 
{
    DeviceCallbacks callbacks = { (void *)destroy, (void *)reset, (void *)saveState, (void *)loadState };
    RomMapperMsxBus* rm;
    int i;
	
    rm = malloc(sizeof(RomMapperMsxBus));

    rm->slot     	= slot;
    rm->sslot    	= sslot;
    rm->cart 	    = cartSlot;
	// rm->scc         = sccCreate(boardGetMixer());
    rm->sccMode     = SCC_COMPATIBLE;
	rm->sccEnable	= 0;
    initialize();
    rm->deviceHandle = deviceManagerRegister(ROM_MSXBUS, &callbacks, rm);
    slotRegister(slot, sslot, 0, 8, read, read, write, reset, rm);
    for (i = 0; i < 8; i++) {   
        slotMapPage(rm->slot, rm->sslot, i, NULL, 0, 0);
    }
    for(i = 1; i < 255; i++)
        ioPortRegisterUnused(i, readIo, writeIo, rm);
    printf("MSXBus created. cartSlot=%d slot=%d sslot=%d\n", cartSlot, slot, sslot);
    for(i = 0; mon_ports[i] > 0; i++)
        ioMonPortRegister(mon_ports[i], NULL, writeIo, rm);
    resetz(1);
    return 1;
}

