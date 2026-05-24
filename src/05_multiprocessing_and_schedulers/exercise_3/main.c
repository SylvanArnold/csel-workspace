#include <unistd.h>
#include <stdlib.h>

int main(void)
{
    // fork the process to create a child
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    while(1){

    }
    return EXIT_SUCCESS;
}