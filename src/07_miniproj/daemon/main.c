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
#define GPIO_DIR      "/sys/class/gpio"

#define BUTTON1_GPIO 0
#define BUTTON2_GPIO 2
#define BUTTON3_GPIO 3
#define POWER_LED_GPIO 362

#define SCREEN_REFRESH_INTERVAL_MS 5000

static int led_fd;
static int button_fds[3];
static int screen_timer_fd;

enum event_type {
    BUTTON1_PRESS,
    BUTTON2_PRESS,
    BUTTON3_PRESS,
    REFRESH_SCREEN,
};

struct event {
    enum event_type type;
    int fd;
};

/* persistent epoll event storage */
static struct event button_events[3];
static struct event timer_event;

/* ---------------- GPIO INIT ---------------- */

static void gpios_init()
{
    syslog(LOG_INFO, "Initializing GPIOs");

    char path[64];
    char value[16];

    int f = open(GPIO_UNEXPORT, O_WRONLY);
    if (f >= 0) {
        snprintf(value, sizeof(value), "%d", POWER_LED_GPIO);
        write(f, value, strlen(value));

        snprintf(value, sizeof(value), "%d", BUTTON1_GPIO);
        write(f, value, strlen(value));

        snprintf(value, sizeof(value), "%d", BUTTON2_GPIO);
        write(f, value, strlen(value));

        snprintf(value, sizeof(value), "%d", BUTTON3_GPIO);
        write(f, value, strlen(value));

        close(f);
    }

    f = open(GPIO_EXPORT, O_WRONLY);
    if (f >= 0) {
        snprintf(value, sizeof(value), "%d", POWER_LED_GPIO);
        write(f, value, strlen(value));

        snprintf(value, sizeof(value), "%d", BUTTON1_GPIO);
        write(f, value, strlen(value));

        snprintf(value, sizeof(value), "%d", BUTTON2_GPIO);
        write(f, value, strlen(value));

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

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", POWER_LED_GPIO);
    led_fd = open(path, O_WRONLY);

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/direction", BUTTON1_GPIO);
    f = open(path, O_WRONLY);
    if (f >= 0) {
        write(f, "in", 2);
        close(f);
    }

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/edge", BUTTON1_GPIO);
    f = open(path, O_WRONLY);
    if (f >= 0) {
        write(f, "both", 4);
        close(f);
    }

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", BUTTON1_GPIO);
    button_fds[0] = open(path, O_RDONLY | O_NONBLOCK);

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/direction", BUTTON2_GPIO);
    f = open(path, O_WRONLY);
    if (f >= 0) {
        write(f, "in", 2);
        close(f);
    }

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/edge", BUTTON2_GPIO);
    f = open(path, O_WRONLY);
    if (f >= 0) {
        write(f, "both", 4);
        close(f);
    }

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", BUTTON2_GPIO);
    button_fds[1] = open(path, O_RDONLY | O_NONBLOCK);

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/direction", BUTTON3_GPIO);
    f = open(path, O_WRONLY);
    if (f >= 0) {
        write(f, "in", 2);
        close(f);
    }

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/edge", BUTTON3_GPIO);
    f = open(path, O_WRONLY);
    if (f >= 0) {
        write(f, "both", 4);
        close(f);
    }

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", BUTTON3_GPIO);
    button_fds[2] = open(path, O_RDONLY | O_NONBLOCK);

    /* clear initial state */
    for (int i = 0; i < 3; i++) {
        char buf[8];
        lseek(button_fds[i], 0, SEEK_SET);
        read(button_fds[i], buf, sizeof(buf));
    }
}

/* ---------------- TIMER ---------------- */

static int create_timer(int interval_ms)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);

    struct itimerspec spec = {
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

/* ---------------- EVENT HANDLER ---------------- */

int process_event(struct event ev)
{
    char buf[4];

    switch (ev.type) {
        case BUTTON1_PRESS:{
            lseek(ev.fd, 0, SEEK_SET);
                char buf[4];
                read(ev.fd, buf, sizeof(buf));
                if (buf[0] == '1')
                    printf("Button 1 pressed\n");
                else
                    printf("Button 1 released\n");
                break;
        }
        case BUTTON2_PRESS:{
            lseek(ev.fd, 0, SEEK_SET);
                char buf[4];
                read(ev.fd, buf, sizeof(buf));
                if (buf[0] == '1')
                    printf("Button 2 pressed\n");
                else
                    printf("Button 2 released\n");
                break;
        }
        case BUTTON3_PRESS:{
            lseek(ev.fd, 0, SEEK_SET);
                char buf[4];
                read(ev.fd, buf, sizeof(buf));
                if (buf[0] == '1')
                    printf("Button 3 pressed\n");
                else
                    printf("Button 3 released\n");
                break;
        }
        case REFRESH_SCREEN: {
            printf("Refreshing screen (fd=%d)\n", ev.fd);

            uint64_t expirations;
            read(screen_timer_fd, &expirations, sizeof(expirations));
            break;
        }

        default:
            return -EINVAL;
    }

    return 0;
}

/* ---------------- MAIN ---------------- */

int main()
{
    gpios_init();

    /*
    ssd1306_init();
    ssd1306_set_position(0,0);
    ssd1306_puts("CSEL1a - SP.07");
    */

    int epoll_fd = epoll_create1(0);
    struct epoll_event events[10];

    screen_timer_fd = create_timer(SCREEN_REFRESH_INTERVAL_MS);

    button_events[0] = (struct event){ .type = BUTTON1_PRESS, .fd = button_fds[0] };
    button_events[1] = (struct event){ .type = BUTTON2_PRESS, .fd = button_fds[1] };
    button_events[2] = (struct event){ .type = BUTTON3_PRESS, .fd = button_fds[2] };

    timer_event = (struct event){ .type = REFRESH_SCREEN, .fd = screen_timer_fd };

    struct epoll_event ev1 = { .events = EPOLLPRI, .data.ptr = &button_events[0] };
    struct epoll_event ev2 = { .events = EPOLLPRI, .data.ptr = &button_events[1] };
    struct epoll_event ev3 = { .events = EPOLLPRI, .data.ptr = &button_events[2] };
    struct epoll_event evt = { .events = EPOLLIN,  .data.ptr = &timer_event };

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, button_fds[0], &ev1);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, button_fds[1], &ev2);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, button_fds[2], &ev3);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, screen_timer_fd, &evt);

    while (1) {
        int n = epoll_wait(epoll_fd, events, 10, -1);

        for (int i = 0; i < n; i++) {
            struct event *ev = (struct event *)events[i].data.ptr;
            process_event(*ev);
        }
    }

    return 0;
}