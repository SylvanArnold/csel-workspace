#include <fcntl.h> // For open() function, library to perform file control operations
#include <stdint.h> 
#include <stdio.h> 
#include <stdlib.h> // For standard input/output and general utilities
#include <string.h>
#include <sys/errno.h> // For error handling, provides access to the errno variable and strerror() function
#include <sys/mman.h> // For memory management, provides access to the mmap() function for mapping files or devices into memory
#include <unistd.h> // For close() function, provides access to the POSIX operating system API

int main()
{
    /* open memory file descriptor */
    int fd = open("/dev/mem", O_RDWR);
    if (fd < 0) {
        printf("Could not open /dev/mem: error=%i\n", fd);
        return -1;
    }

    size_t psz     = getpagesize(); //Get the system's memory page size
    off_t dev_addr = 0x01c14200; // Chip ID register address
    off_t ofs      = dev_addr % psz; // Get offset from the page boundary
    off_t offset   = dev_addr - ofs; // Get the page-aligned address for mapping
    printf(
        "psz=%lx, addr=%lx, offset=%lx, ofs=%lx\n", psz, dev_addr, offset, ofs);

    /* map to user space nanopi internal registers */
    // 0 means that the kernel chooses the address at which to create the mapping
    // PROT_READ | PROT_WRITE means that the memory can be read and written
    // MAP_SHARED means that updates to the mapping are shared with other processes that map this object
    // fd tells the kernel which file to map, in this case /dev/mem
    // offset is the offset in the file where the mapping starts, in this case the page-aligned address of the chip ID register 
    volatile uint32_t* regs =
        mmap(0, psz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);

    if (regs == MAP_FAILED)  // (void *)-1
    {
        printf("mmap failed, error: %i:%s \n", errno, strerror(errno));
        return -1;
    }

    // Read the chip ID registers using the mapped memory
    uint32_t chipid[4] = {
        [0] = *(regs + (ofs + 0x00) / sizeof(uint32_t)),
        [1] = *(regs + (ofs + 0x04) / sizeof(uint32_t)),
        [2] = *(regs + (ofs + 0x08) / sizeof(uint32_t)),
        [3] = *(regs + (ofs + 0x0c) / sizeof(uint32_t)),
    };

    printf("NanoPi NEO Plus2 chipid=%08x'%08x'%08x'%08x\n",
           chipid[0],
           chipid[1],
           chipid[2],
           chipid[3]);

    munmap((void*)regs, psz);
    close(fd);

    return 0;
}
