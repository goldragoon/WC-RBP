KERNEL_VER := 4.19.66-v7+
KERNEL_DIR = /lib/modules/$(KERNEL_VER)/build

obj-m := sonar_ioctl.o
PWD := $(shell pwd)
ccflags-y += -lgcc

all :
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean :
	make -C $(KERNEL_DIR) M=$(PWD) clean
