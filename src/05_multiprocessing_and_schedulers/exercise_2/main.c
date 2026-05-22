#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    size_t size = 15 * 1024 * 1024; // 15 MB

    void *ptr = malloc(size);
    if (ptr == NULL) {
        printf("Allocation FAILED (%zu bytes)\n", size);
        return 1;
    }

    // Zero the memory
    memset(ptr, 0, size);

    printf("Allocation SUCCESS (%zu bytes)\n", size);

    free(ptr);
    return 0;
}