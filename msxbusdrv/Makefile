obj-m := msxbusdrv.o
HOSTOS=`uname -a`

ifeq ($(HOSTOS), Raspberry)
	KERNEL=/lib/modules/$(shell uname -r)/build
	PREFIX=
else
	KERNEL=~/linux
	PREFIX=arm-linux-gnueabihf-
endif

CC=$(PREFIX)gcc
CXX=$(PREFIX)g++

all: test
	make ARCH=arm CROSS_COMPILE=$(PREFIX) -C $(KERNEL) M=$(PWD) modules

clean:
	rm -f test
	make -C $(KERNEL) M=$(PWD) clean

test: msxbus_test.c
	$(CC) -o $@ $<

install: 
	sudo insmod msxbusdrv.ko
	
