obj-m := msxbusdrv.o
HOSTOS=$(shell uname -a | awk '{ print $2 }')

ifeq ($(findstring raspberrypi, $(HOSTOS)), raspberrypi)
	KERNEL=/lib/modules/$(shell uname -r)/build
	COMPILE=
else
	KERNEL=~/linux
	COMPILE=CROSS_COMPILE=arm-linux-gnueabihf-
endif

CC=$(PREFIX)gcc
CXX=$(PREFIX)g++

all: test
	make ARCH=arm $(COMPILE) -C $(KERNEL) M=$(PWD) modules

clean:
	rm -f test
	make -C $(KERNEL) M=$(PWD) clean

test: msxbus_test.c
	$(CC) -o $@ $<

install: 
	sudo insmod msxbusdrv.ko
	
