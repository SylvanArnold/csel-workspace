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

The modinfo tool was not installed. So I added it in the buildroot configuration and rebuilt the image. Then I replaced the `/rootfs` directory with the newly builded one.

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

== Exercise 4

=== Memory allocation in module

In the module code, we can use `kmalloc` function to allocate memory in kernel space. The allocated memory can be freed using `kfree` function. Linux/list.h provides a linked list implementation.

=== My solution

I created a code that creates a linked list of x elements at module installation and deletes it at module removal. Each element is a structure that contains an integer, a text and a list_head structure. The integer is initialized with the index of the element and the text is initialized with a default value.

Here the dsmeg result of my module installation/removal:

```sh
[   97.216060] Sylvan Module Loaded
[   97.219318] Creating list of 10 elements
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
[  131.031133] Deleted 10 elements
[  131.037425] Sylvan Module Unloaded
```

== Exercise 5

I started to request the memory regions I needed:

```c
	ressouces[0] = request_mem_region (0x01c14200, 0x10, "Chip ID");
	ressouces[1] = request_mem_region (0x01c25080, 0x4, "CPU Temperature");
	ressouces[2] = request_mem_region (0x01c30050, 0x8, "MAC Address");
```

But all these requests were failing. These memory regions seems to be already reserved. Thats what I found by running `cat /proc/iomem`:

```sh
01c14000-01c143ff : 1c14000.eeprom eeprom@1c14000
01c25000-01c253ff : 1c25000.thermal-sensor thermal-sensor@1c25000
01c30000-01c3ffff : 1c30000.ethernet ethernet@1c30000
```

So I changed to code to read these registers without claiming them. I used `ioremap` to map the physical memory addresses to virtual addresses and then I read the values using `ioread32`.

Here is the dmesg result:

```sh
[  551.289950] Chip ID: 82800001'94004704'5036c304'0425090e
[  551.295290] CPU Temperature: 38753 m°C
[  551.299132] MAC Address: 02:01:83:c6:1a:9b
```

I controlled chip temperature with `cat /sys/class/thermal/thermal_zone0/temp` and also checked my MAC address with `ifconfig`. It matched the values read in the register.

== Exercise 6

I created a thread in the module that prints a message every 5 seconds. I used `kthread_run` to create and run the thread and `kthread_stop` to stop it at module removal. The thread function is a simple loop that prints a message and sleeps for 5 seconds with `ssleep`.

Here is the dmesg result of the installation and removal of the module:

```sh
[ 3995.237292] Exercice 6 module loaded, reading registers...
[ 3995.243064] Thread is running
[ 4000.479559] tick
[ 4005.599597] tick
[ 4010.719563] tick
[ 4015.839569] tick
[ 4020.959569] tick
[ 4026.079559] tick
[ 4031.199560] tick
[ 4036.319555] tick
[ 4036.321465] Exercice 6 Module Unloaded, thread stopped
```

== Exercise 7

I used a waitqueue as requested in the instructions. At first I had some issue to understand how to use it, as I was confusing waitqueues with message queues. I created two threads, one that sleeps for 5 seconds and then wakes up the other thread using `wake_up_interruptible` function. The second thread is sleeping on the waitqueue using `wait_event_interruptible` function and is woken up by the first thread or if the module is being unloaded.

The dmesg result:

```sh
[ 1750.174073] Exercice 7 module loaded, reading registers...
[ 1750.179928] Thread 1 is running
[ 1750.179967] Thread 2 is running
[ 1755.359354] Thread 1: notifying thread 2
[ 1755.363350] Thread 2: received notification from thread 1
[ 1760.479348] Thread 1: notifying thread 2
[ 1760.483296] Thread 2: received notification from thread 1
[ 1765.599346] Thread 1: notifying thread 2
[ 1765.603288] Thread 2: received notification from thread 1
...
[ 1780.872832] Exercice 7 Module Unloaded, threads stopped
```

== Exercise 8

I created a single interupt handler for the 3 buttons. To distinguish the buttons I created an *enum* that represents the 3 buttons and I passed the corresponding value for each button. For an unknown reason my module failed to request irq for the button k1 (but it worked for k2 and k3). Maybe the pin id is wrong.

The dmesg result:

```sh
[10209.745763] Failed to request IRQ for GPIO 0
[10209.750235] Exercice 8 module loaded
[10248.209782] Interrupt from K2
[10254.388959] Interrupt from K3
[10264.665848] Interrupt from K2
```

= Device Drivers

== Exercise 1

I didn't know how to create the Makefile for a memory mapped device driver so I analysed and used the one provided in the solution. I wrote some comments in the Makefile to explain what each part is doing.

Same for the main.c file. I didn't knew where to start as it was my first time writing a memory mapped device driver. I analysed and commented the solution to understand how it works.

The result of the running app:

```sh
psz=1000, addr=1c14200, offset=1c14000, ofs=200
NanoPi NEO Plus2 chipid=82800001'94004704'5036c304'0425090e
```

The example makes me understand the importance of aligning the memory addresses to the page size when using `mmap` to map physical memory to user space.

== Exercise 2

I created a character device driver based on the skeleton example in the course. I implemented the `open`, `release`, `read`and `write`functions.

The module has an internal `char*` pointer initialised with null value. When the user writes to the device, the pointer is allocated with `kmalloc` and the value is copied from user space to kernel space using `copy_from_user`. The old value is also freed using `kfree` before allocating the new one if it is not the first write. When the user reads from the device, the value is copied from kernel space to user space using `copy_to_user`.

I installed the module with `modprobe`, Inspected `dmesg` to check the major and minor numbers that were attributed:

```sh
[  533.612204] skeleton: allocated char device region with major 511 and minor 0
```

Then I created a device file with `mknod`:

```sh
mknod /dev/skeleton c 511 0
ls -l /dev/skeleton
```

The result:

```sh
crw-r--r-- 1 root root 511, 0 Jan  1 00:16 /dev/skeleton
```

Then I tested the device with `echo` and `cat` commands:

```sh
echo "Hello world" > /dev/skeleton
cat /dev/skeleton
```

It worked as expected. Event after multipl writes.

== Exercise 3

To set the devices numbers I added a `device_n` module parameter with default value of 1 that user can change at load time. Then in the init function I used a loop that creates n devices. Each device has it's own data storage.

To do so I created a dynamic array of skeleton_dev structure that contains the data pointer and the cdev structure.

