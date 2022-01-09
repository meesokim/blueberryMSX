//
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013
   
#include <stdio.h>
#include <stdlib.h>  
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>

int fd;

enum {
   SLOT1,
   SLOT3,
   IO
};

#define MSX_READ 0x10
#define MSX_WRITE 0

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

void setup_io() {
	fd = open("/dev/msxbus", O_RDWR);
}

void clear_io() {
	close(fd);
}

int main(int argc, char **argv)
{
	int g,rep,i,addr, page=4, c= 0,addr0;
	char byte, byte0, io;
	int offset = 0x4000;
	int size = 0x8000;
	FILE *fp = 0;
	struct timespec t1, t2;
	double elapsedTime = 0;
	int binary = 0;
	io = 0;
	int slot = 0;
	if (argc > 1)
	{
		if (strcmp(argv[1], "io"))
		fp = fopen(argv[1], "wb");
		else
		io = 1;
	}
	if (argc > 2)
	{
		offset = atoi(argv[2]);
	}
	if (argc > 3)
	{
		size = atoi(argv[3]);
	}

	// Set up gpi pointer for direct register access
	setup_io();
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
	if (io > 0)
	{
		for(i = 0; i < 256; i++)
		{
			if (i % 16 == 0)
				printf("%02x: ", i);
			printf("%02x ", msxreadio(i));
			if (i > 0 && i % 16 == 15)
				printf("\n");
		}
		exit(0);
	}
	msxwrite(1, 0x6000, 3);
	offset = 0x4000;
	for(addr=offset; addr < offset + size; addr ++)
	{
	#if 0
		addr0 = 0xffff & (addr + (rand() % 2));//0xffff & (0x4000 + rand());
		printf("%04x:%02x\n", addr0, 0xff & msxread(1, addr0));
		addr0 = 0xffff & (addr + (rand() % 2));//0xffff & (0x4000 + rand());
		printf("%04x:%02x\n", addr0, 0xff & msxread(1, addr0));
	#else		
		if (addr > 0xbfff)
		{
			if (!(addr & 0x1fff)) {
			msxwrite(slot, 0x6000, page++);
			printf("page:%d, address=0x%04x\n", page-1, addr );
			}
			byte = msxread(1, 0x6000 + (addr & 0x1ffff));
		}
		else
		{
			byte = msxread(slot, addr);
		}
		if (fp)
			fwrite(&byte, 1, 1, fp);
		else
		{
	#if 1		 
		if (addr % 16 == 0)
				printf("\n%04x:", addr);
	#if 1		  
		c = 0;
		for(i=0;i<10;i++)
		{
			byte0 = msxread(slot, addr);
			if (byte != byte0)
				c = 1;
		}
		if (c)  
			printf("\e[31m%02x \e[0m", byte);
		else
			printf("%02x ", byte);
	#else
		printf("%02x ", byte);
	#endif	
	#endif
		}
	#endif
	}
	printf("\n");
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t2);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000000000.0;      // sec to ns
	elapsedTime += (t2.tv_nsec - t1.tv_nsec) ;   // us to ns	
	if (!binary) {
		printf("elapsed time: %10.2fs, %10.2fns/i\n", elapsedTime/100000000, elapsedTime / size);
	}	
	clear_io();
	return 0;

} // main

