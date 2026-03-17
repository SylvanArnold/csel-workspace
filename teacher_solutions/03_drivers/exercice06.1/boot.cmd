setenv bootargs console=ttyS0,115200 earlyprintk root=/dev/mmcblk0p2 rootwait

fatload mmc 0 $kernel_addr_r Image
#fatload mmc 0 $fdt_addr_r sun50i-h5-nanopi-neo-plus2.dtb
fatload mmc 0 $fdt_addr_r mydt.dtb

fdt addr $fdt_addr_r
fdt resize
#fdt mknode / mymodule2
#fdt set /mymodule2 compatible mymodule2


booti $kernel_addr_r - $fdt_addr_r
