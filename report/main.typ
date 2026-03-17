#import "@preview/red-agora:0.2.0": project

#show: project.with(
  title: "Linux embedded systems",
  subtitle: "Lab report",
  authors: (
    "Sylvan Arnold",
  ),
  school-logo: image("ressources/hesso-logo.svg"), // Replace with [] to remove the school logo
  //company-logo: image("images/company.svg"),
  branch: "Computer Science",
  academic-year: "2026",
  footer-text: "MA_CSEL1", // Text used in left side of the footer
)

= Environment setup

== Steps

I followed the steps as described in the lab instructions. Everything worked as expected.

*NOTE*: I added a rootfs overlay directory to the buildroot configuration so I can keep my custom files (like /etc/fstab) in it and they will be included in the generated rootfs. This way I don't have to remodify the rootfs every time I want to rebuild the image.

== Questions Answers

=== How do we proceed to generate U-boot?

In our case Uboot is already included in the buildroot configuration (has been included via `make menuconfig`). The `make`command will generate the sd card image with the Uboot included.

=== How can we add additional package to the buildroot configuration?

By running `make menuconfig`in the buildroot directory we can access the configuration menu. Then you can go inside `target packages`section and add additional packages to the build. If we run `make` the new package will be included in the build.

=== How do we proceed to modify the kernel configuration?

By running `make linux-menuconfig` in the buildroot directory we can access the kernel configuration menu and modify the kernel configuration.

=== How can we generate our own rootfs?

You can create a rootfs overlay directory, add it in the buildroot configuration (using `menuconfig`) and then when running `make`the overlay directory will be included in the generated rootfs.

=== How can we use emmc instead of sd card?

Change U-boot boot command to load kernel/rootfs/... from emmc instead of sd card (change ext4load mmc 1:1 to ext4load mmc 0:1 for example). Use a USB flashing tool to flash the generated image on the emmc directly.

=== What would be the optimal configuration for developping applications in user space only?

The optimal configuration is cross compiling the application on host machine then syncing the builded application with the target device. The easiest way to sync files is to use a CIFS share on the host machine and mount it on the target device (as we did with workspace directory). Then use ssh to access the target device and run the application. No need to sync the entire rootfs and no need to download the kernel from network at boot.

= Kernel Modules programming

== Exercise 1

=== Create a skeleton module outside the kernel tree

I created a `skeleton.c` and a `Makefile`, inspired by the examples provided in the course. I builded the module using `make` and it generated a `skeleton.ko` file.


=== Test the modinfo command on the generated module

The modinfo tool was not installed. So I added in the buildroot configuration and rebuilt the image. Then I replaced the `/rootfs` directory with the newly builded one.

Here is the result:

```sh
filename:       /workspace/./src/02_modules/exercice01/mymodule.ko
license:        GPL
description:    Module skeleton
author:         Sylvan Arnold <sylvan.arnold@hes-so.ch>
depends:
name:           mymodule
vermagic:       5.15.148 SMP preempt mod_unload aarch64
parm:           text:charp
parm:           elements:int
```
#pagebreak()

=== Install the module and check dmesg

```sh
insmod /workspace/./src/02_modules/exercice01/mymodule.ko
dmesg
```

```sh
[  761.493297] Linux module skeleton loaded
```


The module works as expected.

=== Compare lsmod and cat /proc/modules commands

`cat /proc/modules` result:

```sh
mymodule 16384 0 - Live 0xffff80000110b000 (O)
ipv6 462848 18 [permanent], Live 0xffff800001167000
brcmfmac 253952 0 - Live 0xffff800001128000
brcmutil 20480 1 brcmfmac, Live 0xffff800001122000
cfg80211 393216 1 brcmfmac, Live 0xffff8000010aa000
rfkill 36864 1 cfg80211, Live 0xffff8000010a0000
sunxi_wdt 20480 0 - Live 0xffff800001042000
sun8i_mixer 40960 0 - Live 0xffff800001037000
sun4i_tcon 36864 0 - Live 0xffff80000102d000
sun8i_tcon_top 16384 1 sun4i_tcon, Live 0xffff80000109b000
drm_kms_helper 282624 2 sun8i_mixer,sun4i_tcon, Live 0xffff800001055000
lima 61440 0 - Live 0xffff80000101d000
gpu_sched 36864 1 lima, Live 0xffff80000104b000
drm 585728 6 sun8i_mixer,sun4i_tcon,drm_kms_helper,lima,gpu_sched, Live 0xffff800000f8d000
sun6i_dma 28672 0 - Live 0xffff800000f85000
sun8i_ce 32768 0 - Live 0xffff800000f7c000
crypto_engine 20480 1 sun8i_ce, Live 0xffff800000f76000
crct10dif_ce 20480 1 - Live 0xffff800000f70000
```

`lsmod` result:

```sh
Module                  Size  Used by
mymodule               16384  0
ipv6                  462848  18
brcmfmac              253952  0
brcmutil               20480  1 brcmfmac
cfg80211              393216  1 brcmfmac
rfkill                 36864  1 cfg80211
sunxi_wdt              20480  0
sun8i_mixer            40960  0
sun4i_tcon             36864  0
sun8i_tcon_top         16384  1 sun4i_tcon
drm_kms_helper        282624  2 sun8i_mixer,sun4i_tcon
lima                   61440  0
gpu_sched              36864  1 lima
drm                   585728  6 gpu_sched,sun8i_mixer,drm_kms_helper,lima,sun4i_tcon
sun6i_dma              28672  0
sun8i_ce               32768  0
crypto_engine          20480  1 sun8i_ce
crct10dif_ce           20480  1
```

#linebreak()

The two commands are giving similar results. The `lsmod` command is a user-friendly way to display the loaded modules, while `cat /proc/modules` gives more detailed information about the modules.


#pagebreak()

== Exercise 2

=== Adapt rootfs to include modprobe

I already installed modprobe in the rootfs in the previous exercise. The module has to be installed in the rootfs as well. To do that we can add an install target in the module makefile:

```makefile
install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install
```

Then, when we run `make install` in the module directory, the module will be installed in the rootfs.

=== Adapt previous module to accept and log parameters

Parameters can be declared as static variables in the module code:

```c
#include <linux/moduleparam.h>  /* needed for module parameters */

static char* text = "dummy text";
module_param(text, charp, 0664);
static int  elements = 1;
module_param(elements, int, 0);
```

Then we can log them in the console using pr_info: 

```c
pr_info("  text: %s\n", text);
pr_info("  elements: %d\n", elements);
```

Then the parameters can be passed to the module at load time using `modprobe`:

```sh
modprobe mymodule text="hello world" elements=42
```

The parameters are then displayed in dmesg:

```sh
[ 4564.063892] Linux module 01 skeleton loaded
[ 4564.068128]   text: hello world
[ 4564.068128]   elements: 42
```

#pagebreak()

== Exercise 3

```sh
cat /proc/sys/kernel/printk
7       4       1       7
```

It displays the current settings of printk function. The first value is the current log level, the second value is the default log level attributed to prints that does not specify it, the third value is the minimum log level allowed and the fourth value is the default log level set at boot.

#pagebreak()

== Exercice 4

=== 

=== Installation/removal results

Here the dsmeg result of my module installation/removal:

```sh
[   97.216060] Sylvan Module Loaded
[   97.219318] Creating list of 20 elements
[  130.972082] Deleting element 0
[  130.972108] Deleting element 1
[  130.975174] Deleting element 2
[  130.978268] Deleting element 3
[  130.981373] Deleting element 4
[  130.984451] Deleting element 5
[  130.987516] Deleting element 6
[  130.990569] Deleting element 7
[  130.993633] Deleting element 8
[  130.996696] Deleting element 9
[  130.999759] Deleting element 10
[  131.002812] Deleting element 11
[  131.005964] Deleting element 12
[  131.009112] Deleting element 13
[  131.012264] Deleting element 14
[  131.015413] Deleting element 15
[  131.018551] Deleting element 16
[  131.021693] Deleting element 17
[  131.024843] Deleting element 18
[  131.027994] Deleting element 19
[  131.031133] Deleted 20 elements
[  131.037425] Sylvan Module Unloaded
```

== Exercise 5
