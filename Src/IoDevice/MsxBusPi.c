/*****************************************************************************
**
** Msx Slot Access Code for Raspberry Pi 
** https://github.com/meesokim/msxslot
**
** RPMC(Raspberry Pi MSX Clone) core module
**
** Copyright (C) 2016 Miso Kim meeso.kim@gmail.com
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

#define RPMC_V5
   
#include <stdio.h>
#include <stdlib.h>  
#include <fcntl.h>
#include <sys/mman.h>
<<<<<<< HEAD
#include <bcm2835.h>
=======
//#include <bcm2835.h>
>>>>>>> bcb4a8949565583469feb7a0bc3302640bf28f85
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t mutex;
<<<<<<< HEAD

int setup_io();
void frontled(unsigned char byte);
int msxread(int slot, unsigned short addr);
void msxwrite(int slot, unsigned short addr, unsigned char byte);
int msxreadio(unsigned short addr);
void msxwriteio(unsigned short addr, unsigned char byte);
void clear_io();
void setup_gclk();

=======

int setup_io();
void frontled(unsigned char byte);
int msxread(int slot, unsigned short addr);
void msxwrite(int slot, unsigned short addr, unsigned char byte);
int msxreadio(unsigned short addr);
void msxwriteio(unsigned short addr, unsigned char byte);
void clear_io();
void setup_gclk();

>>>>>>> bcb4a8949565583469feb7a0bc3302640bf28f85
#define MSX_READ 0x10
#define MSX_WRITE 0

#define RD0		0
#define RD1		1
#define RD2		2
#define RD3		3
#define RD4		4
#define RD5		5
#define RD6		6
#define RD7		7
#define RA8		8
#define RA9		9
#define RA10	10
#define RA11	11
#define RA12	12
#define RA13	13
#define RA14	14
#define RA15	15
#define RC16	16
#define RC17	17
#define RC18	18
#define RC19	19
#define RC20	20
#define RC21	21
#define RC22	22
#define RC23	23
#define RC24	24
#define RC25	25
#define RC26	26
#define RC27	27

#define INT_PIN		RC24
#define SW1_PIN		RC27

#define GPIO_GPSET0     7
#define GPIO_GPSET1     8

#define GPIO_GPCLR0     10
#define GPIO_GPCLR1     11

#define GPIO_GPLEV0     13
#define GPIO_GPLEV1     14

volatile unsigned int* gpio;

inline void GPIO_SET(unsigned int b)
{
	gpio[GPIO_GPSET0] = b;
}

inline void GPIO_CLR(unsigned int b)
{
	gpio[GPIO_GPCLR0] = b;
}

int fd;
volatile unsigned int *gpio;

enum {
   SLOT1,
   SLOT3,
   IO
};


int msxreadio(unsigned short addr) {
	return ioctl(fd, MSX_READ | IO, addr);	
}

void msxwriteio(unsigned short addr, unsigned char byte) {
	ioctl(fd, MSX_WRITE | IO, addr | (byte << 16));
}

int msxread(int slot, unsigned short addr) {
	return ioctl(fd, MSX_READ | SLOT1, addr);
}

void msxwrite(int slot, unsigned short addr, unsigned char byte) {
	ioctl(fd, MSX_WRITE | SLOT1, addr | (byte << 16));
}

int setup_io() {
	fd = open("/dev/msxbus", O_RDWR);
	if (!bcm2835_init()) return -1;
	gpio = bcm2835_regbase(BCM2835_REGBASE_GPIO);
	return 0;
}

void clear_io() {
	close(fd);

}
 
void checkInt()
{
//	if (!(bcm2835_gpio_lev(INT_PIN)))
//	{
//		boardSetInt(0x10000);
//	}
} 
 
void msxinit()
{
	const struct sched_param priority = {1};
	sched_setscheduler(0, SCHED_FIFO, &priority);  
//	stick_this_thread_to_core(0);
	if (setup_io() == -1)
    {
        printf("GPIO init error\n");
        exit(0);
    }
    frontled(0x0);
	printf("MSX BUS initialized\n");
}

void msxclose()
{
	clear_io();
}

int msx_pack_check()
{
	//return !(bcm2835_gpio_lev(SW1_PIN));
	return 0;
}
void frontled(unsigned char byte)
{
#define SRCLK (1<<RC22)
#define RCLK (1<<RC23)
#define SER (1<<RC26)
    static unsigned char oldbyte = 0;
    if (oldbyte != byte)
    {
        oldbyte = byte;
        pthread_mutex_lock(&mutex);
        GPIO_CLR(SRCLK | RCLK | SER);
        for (int i = 0; i < 8; i++)
        {
            if ((byte >> i) & 1)
                GPIO_SET(SER);
            else
                GPIO_CLR(SER);
            GPIO_SET(SRCLK);
            GPIO_CLR(SRCLK);
        }
        GPIO_SET(RCLK);
        pthread_mutex_unlock(&mutex);	    
    }
}
