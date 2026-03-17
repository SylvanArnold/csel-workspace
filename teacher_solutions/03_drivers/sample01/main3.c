#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FILE_SZ (5 * 1024 * 1024)

static const char* msg1 = "1234567890abcdefghijklmnopqrstuvwxyz\n";

int main()
{
    int fd     = open("test3.txt", O_RDWR | O_CREAT | O_TRUNC, 0664);
    char* file = mmap(0, FILE_SZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    lseek(fd, FILE_SZ, SEEK_SET);
    write(fd, "", 1);
    printf("the file after mapping...\n");

    for (int i = 0; i < 10000; i += 1)
        memcpy(file + i * strlen(msg1), msg1, strlen(msg1));

    printf("the file after filling...\n");
    close(fd);

    return 0;
}
