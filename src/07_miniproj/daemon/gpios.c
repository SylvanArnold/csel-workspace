#include "gpios.h"
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// Contain all file descriptors for the GPIOs
static gpios_fd_t gpios = {
    .led_fd = -1,
    .button_fds = {-1, -1, -1},
};

gpios_fd_t* gpios_init()
{
    syslog(LOG_INFO, "Initializing GPIOs");

    char path[64];
    char value[16];

    int f = open(GPIO_UNEXPORT, O_WRONLY);
    if (f >= 0) {
        snprintf(value, sizeof(value), "%d", POWER_LED_GPIO);
        write(f, value, strlen(value));

        for (int i = 0; i < NUM_BUTTONS; i++) {
            snprintf(value, sizeof(value), "%d", BUTTON_GPIOS[i]);
            write(f, value, strlen(value));
        }

        close(f);
    }

    f = open(GPIO_EXPORT, O_WRONLY);
    if (f >= 0) {
        snprintf(value, sizeof(value), "%d", POWER_LED_GPIO);
        write(f, value, strlen(value));

        for (int i = 0; i < NUM_BUTTONS; i++) {
            snprintf(value, sizeof(value), "%d", BUTTON_GPIOS[i]);
            write(f, value, strlen(value));
        }

        snprintf(value, sizeof(value), "%d", BUTTON3_GPIO);
        write(f, value, strlen(value));
        close(f);
    }

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/direction", POWER_LED_GPIO);
    f = open(path, O_WRONLY);
    if (f >= 0) {
        write(f, "out", 3);
        close(f);
    }

    // open LED value file for writing
    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", POWER_LED_GPIO);
    gpios.led_fd = open(path, O_WRONLY);

    // configure buttons
    for (int i = 0; i < NUM_BUTTONS; i++) {
        int gpio = BUTTON_GPIOS[i];

        // set direction to input
        snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/direction", gpio);
        f = open(path, O_WRONLY);
        if (f >= 0) {
            write(f, "in", 2);
            close(f);
        }

        // configure edge detection for both rising and falling edges
        snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/edge", gpio);
        f = open(path, O_WRONLY);
        if (f >= 0) {
            write(f, "both", 4);
            close(f);
        }

        // open value file for non-blocking read
        snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", gpio);
        gpios.button_fds[i] = open(path, O_RDONLY | O_NONBLOCK);
    }

    /* clear initial state */
    for (int i = 0; i < NUM_BUTTONS; i++) {
        char buf[8];
        lseek(gpios.button_fds[i], 0, SEEK_SET);
        read(gpios.button_fds[i], buf, sizeof(buf));
    }
    return &gpios;
}

void set_led(bool on)
{
    if (gpios.led_fd >= 0) {
        const char *val = on ? "1" : "0";
        write(gpios.led_fd, val, 1);
    }
}

bool is_button_pressed(int fd)
{
    char buf[4];
    lseek(fd, 0, SEEK_SET);
    read(fd, buf, sizeof(buf));
    return buf[0] == '1';
}