obj-m := msxbusdrv.o

all: test
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	rm -f test
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

test: msxbus_test.c
	gcc -o $@ $<

install: 
	sudo insmod msxbusdrv.ko
	
