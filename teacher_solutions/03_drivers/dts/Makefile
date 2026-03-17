# Part executed when called from kernel build system:
ifneq ($(KERNELRELEASE),)
obj-m += mymodule.o		## name of the generated module

ccflags-y += -Wno-declaration-after-statement -std=gnu11
mymodule-objs := skeleton.o	## list of objects needed for that module

# Part executed when called from standard make in module source directory:
else
include ../../kernel_settings
PWD := $(shell pwd)

INCL+=-I. -I$(KDIR)/include -I$(KDIR)/arch/arm64/boot/dts
DTB = mydt.dtb
DTS = $(DTB:.dtb=.dts)

all: dtb boot
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(CPU) CROSS_COMPILE=$(TOOLS) modules

dtb: $(DTB)
$(DTB) : $(DTS)
	ln -s $(KDIR)/arch/arm/boot/dts arm
	-cpp $(INCL) -E -P -x assembler-with-cpp $(DTS) | dtc -I dts -O dtb -o $(DTB) -
	rm arm

boot:
	mkimage -T script -A arm -C none -d boot.cmd boot.ovl

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.dtb *.ovl

install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install

endif

