obj-m:=AuditModule.o
AuditModule-objs   :=sdthook.o syscalltable.o netlinkp.o
KDIR   := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(RM) -rf .*.cmd *.mod.c *.o *.ko .tmp*
	sudo rmmod AuditModule.ko
	dmesg | tail -n 10
test:
	make
	sudo insmod AuditModule.ko
show:
	dmesg | tail -n 50
