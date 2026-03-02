obj-m += ksysfs-logger.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(CURDIR)
EXTRA_CFLAGS = -Wall -g

SUDO ?= sudo

.PHONY: all clean load unload

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf .cache

load: all
	-$(SUDO) rmmod ksysfs-logger
	$(SUDO) insmod ksysfs-logger.ko

unload:
	-$(SUDO) rmmod ksysfs-logger