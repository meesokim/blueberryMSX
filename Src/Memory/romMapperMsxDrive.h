/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperMsxBus.h,v $
**
** $Revision: 1.5 $
**
** $Date: 2018-08-24 18:38:44 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2014-2006 Daniel Vik
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
#ifndef ROMMAPPER_MSX_DRIVE_H
#define ROMMAPPER_MSX_DRIVE_H
 
#include "MsxTypes.h"

typedef enum {
    SCC_MIRRORED = 0,
    SCC_EXTENDED,
    SCC_SNATCHER,
    SCC_SDSNATCHER,
    SCCP_EXTENDED
} SccType;


int romMapperMsxDriveCreate(int cartSlot, int slot, int sslot);

#endif
