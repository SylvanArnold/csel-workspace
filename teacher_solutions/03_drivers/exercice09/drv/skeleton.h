#ifndef SKELETON_H
#define SKELETON_H

#include <linux/ioctl.h>

struct skeleton_config {
    int id;
    long ref;
    char name[30];
    char descr[30];
};

#define SKELETON_IOMAGIC 'g'
#define SKELETON_IO_RESET _IO(SKELETON_IOMAGIC, 0)
#define SKELETON_IO_WR_REF _IOW(SKELETON_IOMAGIC, 1, struct skeleton_config)
#define SKELETON_IO_RD_REF _IOR(SKELETON_IOMAGIC, 2, struct skeleton_config)
#define SKELETON_IO_WR_VAL _IOW(SKELETON_IOMAGIC, 3, int)
#define SKELETON_IO_RD_VAL _IOR(SKELETON_IOMAGIC, 4, int)

#endif
