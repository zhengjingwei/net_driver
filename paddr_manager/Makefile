obj-m:=paddr_manager.o
KSRC=/home/arnold2/linux
KBUILD_CFLAGS += -Wno-unused-variable \
				-Wno-unused-label \
				-Wno-unused-function
#CROSS_COMPILE=arm-linux-gcc
PWD := $(shell pwd)
all:
	make -C $(KSRC) M=$(PWD) modules
clean:
	make -C $(KSRC) M=$(PWD) clean
