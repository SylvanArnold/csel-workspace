/**
 * @file    gpios.h
 * @brief   GPIO control interface for the fan controller daemon
 * @author  Sylvan Arnold
 */

#ifndef GPIOS_H
#define GPIOS_H

#include <stdbool.h>

#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_DIR      "/sys/class/gpio"

#define BUTTON1_GPIO 0
#define BUTTON2_GPIO 2
#define BUTTON3_GPIO 3
#define NUM_BUTTONS 3
static const int BUTTON_GPIOS[NUM_BUTTONS] = { BUTTON1_GPIO, BUTTON2_GPIO, BUTTON3_GPIO };

#define POWER_LED_GPIO 362

typedef struct gpios_fds {
    int led_fd;
    int button_fds[NUM_BUTTONS];
} gpios_fd_t;

gpios_fd_t* gpios_init();
void set_led(bool on);
bool is_button_pressed(int fd);

#endif // GPIOS_H