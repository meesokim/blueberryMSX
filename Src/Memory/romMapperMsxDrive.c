/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperGameReader.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-03-30 18:38:44 $
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
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include "romMapperMsxDrive.h"
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
    int cart;
    int slot;
    int sslot;
    int cartSlot;
    SccMode sccMode;
    SccType sccType;
    SCC* scc;
	int sccEnable;
} RomMapperMsxDrive;

static ReadfnPtr msxread;
static WritefnPtr msxwrite;
static InitfnPtr msxinit;
static ResetfnPtr resetz;	
static void *hDLL = 0;	

static void saveState(RomMapperMsxDrive* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperMsxDrive");
    saveStateClose(state);
}

static void loadState(RomMapperMsxDrive* rm)
{
    SaveState* state = saveStateOpenForRead("mapperMsxDrive");
    saveStateClose(state);
}


static void destroy(RomMapperMsxDrive* rm)
{
    if (hDLL)
        CloseZemmix(hDLL);
    ioPortUnregisterUnused(rm->cartSlot);
    slotUnregister(rm->slot, rm->sslot, 0);
    deviceManagerUnregister(rm->deviceHandle);
    sccDestroy(rm->scc);

//    fclose(f);
    f = NULL;

    free(rm);
}

static UInt8 readIo(RomMapperMsxDrive* rm, UInt16 port)
{
    return msxread(RD_IO, port);
}

static void writeIo(RomMapperMsxDrive* rm, UInt16 port, UInt8 value)
{
    msxwrite(WR_IO, port, value);
}

static UInt8 read(RomMapperMsxDrive* rm, UInt16 address) 
{
#if 0
    if (rm->sccEnable  && ((address >= 0x9800 && address < 0xa000)) {
//		printf("rm->sccEnable = %d\n", rm->sccEnable);
        return sccRead(rm->scc, (UInt8)(address & 0xff));
    }
#endif	
    // printf("msxread(slot:%d,address=%x)\n", rm->slot, address);
    return msxread(rm->cart > 1 ? RD_SLTSL2 : RD_SLTSL1, address);
}

static void write(RomMapperMsxDrive* rm, UInt16 address, UInt8 value) 
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
    // printf("msxwrite(slot:%d,address=%04x,value=%02x)\n", rm->slot, address, value);
    return msxwrite(rm->cart > 1 ? WR_SLTSL2 : WR_SLTSL1, address, value);
}

static void reset(RomMapperMsxDrive* rm)
{
    printf("reset of RomMapperMsxDrive\n");
    // sccReset(rm->scc);
    resetz(5);
}

static const int mon_ports[] = {}; // 0x7c, 0x7d, 0x7e, 0x7f, 0xa0, 0xa1, 0xa2, 0xa3, 0 };

#define ZMX_DRIVER "./zmxdrive" ZEMMIX_EXT 

static void initialize() {
    if (hDLL)
        CloseZemmix(hDLL);
    hDLL = OpenZemmix((char*)ZEMMIX_DRIVE, RTLD_LAZY);
    if (!hDLL)
    {
        printf("DLL open error!! %s\n", ZEMMIX_DRIVE);
        exit(1);
    }	
    msxread = (ReadfnPtr)GetZemmixFunc(hDLL, (char*)MSXREAD);
    msxwrite = (WritefnPtr)GetZemmixFunc(hDLL, (char*)MSXWRITE);
    msxinit = (InitfnPtr)GetZemmixFunc(hDLL, (char*)MSXINIT);
    resetz = (ResetfnPtr)GetZemmixFunc(hDLL, (char*)MSXRESET);            
    msxinit(getenv("SDCARD"));
    resetz(5);		
}

int romMapperMsxDriveCreate(int cartSlot, int slot, int sslot) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    RomMapperMsxDrive* rm;
    int i;
	
    rm = malloc(sizeof(RomMapperMsxDrive));
    printf("cartSlot:%d, slot:%d,sslot:%d\n", cartSlot, slot, sslot);
	// rm->scc         = sccCreate(boardGetMixer());

    rm->deviceHandle = deviceManagerRegister(ROM_MSXDRIVE, &callbacks, rm);
    slotRegister(slot, sslot, 0, 8, read, read, write, destroy, rm);
    initialize();
	// if (rm->msxDrive)
    // {
	// 	printf("MSXDrive created. cartSlot=%d slot=%d sslot=%d\n", cartSlot, slot, sslot);
    // //     if (cartSlot == 0) {
    // // //		for(i = 1; i < 255; i++)
    // // //			ioPortRegisterUnused(i, readIo, writeIo, rm);
    //     msxDriveWrite(rm->msxDrive, 0x4000, 1);
    // //         // printf("MSXDrive created. cartSlot=%d slot=%d sslot=%d\n", cartSlot, slot, sslot);
    // //         for(i = 0; mon_ports[i] > 0; i++)
    // //             ioMonPortRegister(mon_ports[i], NULL, writeIo, rm);
    // //     }

    // }
    rm->cart        = cartSlot;
    rm->slot     	= slot;
    rm->sslot    	= sslot;
    rm->sccMode     = SCC_COMPATIBLE;
    rm->sccType     = SCC_EXTENDED;
	rm->sccEnable	= 0;

    for (i = 0; i < 8; i++) {   
        slotMapPage(rm->slot, rm->sslot, i, NULL, 0, 0);
    }

    return 1;
}

