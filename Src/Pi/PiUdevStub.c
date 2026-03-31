/*****************************************************************************
** Stub udev / hotplug for MinGW / Windows — full implementation is PiUdev.c (Linux).
*****************************************************************************/

#include "PiUdev.h"

int piInitUdev(void)
{
	return 1;
}

int piDestroyUdev(void)
{
	return 0;
}

void piScanDevices(void)
{
}

unsigned char udev_format_unit(char **fileNames, int sector, int sectorSize, int track, int head)
{
    return 0;
}
