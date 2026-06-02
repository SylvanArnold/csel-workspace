#include "ssd1306.h"
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_DIR     "/sys/class/gpio"
#define BUTTON1_GPIO 0
#define BUTTON2_GPIO 2
#define BUTTON3_GPIO 3
#define POWER_LED_GPIO 362

#define SCREEN_REFRESH_INTERVAL_MS 5000

static int led_fd;
static int button_fds[3];
static int screen_timer_fd;

// Event types handled by the daemon
enum event_type {
    BUTTON1_PRESS,
    BUTTON2_PRESS,
    BUTTON3_PRESS,
    REFRESH_SCREEN,
};

/*
    Configure all GPIOS and open file decriptors
*/
static void gpios_init(){
    syslog(LOG_INFO, "Initializing GPIOs");
    char path[64];
    char value[16];
    int len;
    // unexport pins out of sysfs (reinitialization)
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    len = snprintf(value, sizeof(value), "%d", POWER_LED_GPIO);
    write(f, value, "write unexport power led");
    len = snprintf(value, sizeof(value), "%d", BUTTON1_GPIO);
    write(f, value, "write unexport button1");
    len = snprintf(value, sizeof(value), "%d", BUTTON2_GPIO);
    write(f, value, "write unexport button2");
    len = snprintf(value, sizeof(value), "%d", BUTTON3_GPIO);
    write(f, value, "write unexport button3");
    close(f);

    // export pins to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    len = snprintf(value, sizeof(value), "%d", POWER_LED_GPIO);
    write(f, value, "write export power led");
    len = snprintf(value, sizeof(value), "%d", BUTTON1_GPIO);
    write(f, value, "write export button1");
    len = snprintf(value, sizeof(value), "%d", BUTTON2_GPIO);
    write(f, value, "write export button2");
    len = snprintf(value, sizeof(value), "%d", BUTTON3_GPIO);
    write(f, value, "write export button3");
    close(f);

    // Configure pins directions and open file descriptors
    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/direction", POWER_LED_GPIO);
    f = open(path, O_WRONLY, "open power led direction");
    write(f, "out", "write power led direction");
    close(f);

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", POWER_LED_GPIO);
    led_fd = open(path, O_WRONLY, "open power led value");
    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/direction", BUTTON1_GPIO);
    f = open(path, O_WRONLY, "open button1 direction");
    write(f, "in", "write button1 direction");
    close(f);

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/edge", BUTTON1_GPIO);
    f = open(path, O_WRONLY, "open button1 edge");
    write(f, "both", "write button1 edge");
    close(f);

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", BUTTON1_GPIO);
    button_fds[0] = open(path, O_RDONLY | O_NONBLOCK, "open button1 value");

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/direction", BUTTON2_GPIO);
    f = open(path, O_WRONLY, "open button2 direction");
    write(f, "in", "write button2 direction");
    close(f);

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/edge", BUTTON2_GPIO);
    f = open(path, O_WRONLY, "open button2 edge");
    write(f, "both", "write button2 edge");
    close(f);

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", BUTTON2_GPIO);
    button_fds[1] = open(path, O_RDONLY | O_NONBLOCK, "open button2 value");

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/direction", BUTTON3_GPIO);
    f = open(path, O_WRONLY, "open button3 direction");
    write(f, "in", "write button3 direction");
    close(f);

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/edge", BUTTON3_GPIO);
    f = open(path, O_WRONLY, "open button3 edge");
    write(f, "both", "write button3 edge");
    close(f);

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", BUTTON3_GPIO);
    button_fds[2] = open(path, O_RDONLY | O_NONBLOCK, "open button3 value");

    for (int i = 0; i < 3; i++) {
        char buf[8];
        lseek(button_fds[i], 0, SEEK_SET);
        read(button_fds[i], buf, sizeof(buf));
    }

    return;
}

// Function to create and configure a timerfd
static int create_timer(int interval_ms)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);

    struct itimerspec spec = {
        // Fire immediately, then use the steady interval.
        .it_value.tv_sec = 0,
        .it_value.tv_nsec = 1,
        .it_interval = {
            .tv_sec = interval_ms / 1000,
            .tv_nsec = (interval_ms % 1000) * 1000000,
        }
    };

    timerfd_settime(tfd, 0, &spec, NULL);
    return tfd;
}



int process_event(enum event_type event) {
    switch (event) {
        case BUTTON1_PRESS: {
            printf("Button 1 pressed\n");
            break;
        }
        case BUTTON2_PRESS:{
            printf("Button 2 pressed\n");
            break;
        }
        case BUTTON3_PRESS: {
            printf("Button 3 pressed\n");
        }
            break;
        case REFRESH_SCREEN:{
            printf("Refreshing screen\n");
            uint64_t expirations = 0;
            // Clear timerfd readiness so epoll doesn't retrigger immediately.
            read(screen_timer_fd, &expirations, sizeof(expirations));
            break;
        }
        default:
            return -EINVAL; // Invalid event type
    }
    return 0; // Success
}


int main()
{
    gpios_init();
    /*
    ssd1306_init();

    ssd1306_set_position (0,0);
    ssd1306_puts("CSEL1a - SP.07");
    ssd1306_set_position (0,1);
    ssd1306_puts("  Demo - SW");
    ssd1306_set_position (0,2);
    ssd1306_puts("--------------");

    ssd1306_set_position (0,3);
    ssd1306_puts("Temp: 35'C");
    ssd1306_set_position (0,4);
    ssd1306_puts("Freq: 1Hz");
    ssd1306_set_position (0,5);
    ssd1306_puts("Duty: 50%");
    */

    int epoll_fd = epoll_create1(0); // Create epoll instance
    struct epoll_event events[10]; // Events buffer
    screen_timer_fd = create_timer(SCREEN_REFRESH_INTERVAL_MS);

    // Configure epoll events for buttons and timer
    struct epoll_event button1_press = {
        .events = EPOLLPRI | EPOLLET, 
        .data.u32 = BUTTON1_PRESS, 
    };
    struct epoll_event button2_press = {
        .events = EPOLLPRI | EPOLLET, 
        .data.u32 = BUTTON2_PRESS, 
    };
    struct epoll_event button3_press = {
        .events = EPOLLPRI | EPOLLET, 
        .data.u32 = BUTTON3_PRESS, 
    };
    struct epoll_event refresh_screen = {
        .events = EPOLLIN, 
        .data.u32 = REFRESH_SCREEN,
    };

    // Add button file descriptors and timerfd to epoll instance
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, button_fds[0], &button1_press);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, button_fds[1], &button2_press);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, button_fds[2], &button3_press);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, screen_timer_fd, &refresh_screen);

    while (1) {
        int n = epoll_wait(epoll_fd, events, 10, -1); // Wait for events
        for (int i = 0; i < n; i++) {
            process_event(events[i].data.u32); // Process each event
        }
    }
    return 0;
}



