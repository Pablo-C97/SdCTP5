obj-m += cdd_sensor.o


KDIR ?= /home/$(USER)/raspberry/linux-headers-rpi


CROSS ?= /home/$(USER)/arm-gnu-toolchain-13.2.Rel1-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-

PWD := $(shell pwd)

all:
	make ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KDIR) M=$(PWD) modules

clean:
	make ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KDIR) M=$(PWD) clean