#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static const char* msg1 = "1234567890abcdefghijklmnopqrstuvwxyz\n";

int main()
{
    int fd = open("test1.txt", O_RDWR | O_CREAT | O_TRUNC, 0664);
    for (int i = 0; i < 10000; i++) write(fd, msg1, strlen(msg1));
    close(fd);
}
