export PATH := /buildroot/output/host/usr/sbin$\
	:/buildroot/output/host/usr/bin/$\
	:/buildroot/output/host/sbin$\
	:/buildroot/output/host/bin/$\
	:$(PATH)

# Part executed when called from kernel build system:
ifneq ($(KERNELRELEASE),)
obj-m += mymodule.o		## name of the generated module

mymodule-objs := skeleton.o	## list of objects needed for that module

# Part executed when called from standard make in module source directory:
else
include ../../kernel_settings
PWD := $(shell pwd)

all: dtb
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(CPU) CROSS_COMPILE=$(TOOLS) modules


DTB = mydt.dtb
DTS = mydt.dts
INCL+=-I. -I$(KDIR)/include -I$(KDIR)/arch/arm64/boot/dts

dtb: $(DTB)
$(DTB) : $(DTS)
	ln -s $(KDIR)/arch/arm/boot/dts arm
	-cpp $(INCL) -E -P -x assembler-with-cpp $(DTS) | dtc -I dts -O dtb -o $(DTB) -
	rm arm

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.dtb

install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install

endif

