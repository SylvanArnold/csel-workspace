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
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include "fan.h"
#include "screen.h"
#include "gpios.h"

#define SCREEN_REFRESH_INTERVAL_MS 100

static int screen_timer_fd = -1;

enum event_type {
    BUTTON1_PRESS,
    BUTTON2_PRESS,
    BUTTON3_PRESS,
    REFRESH_SCREEN,
    SOCKET_ACCEPT_CLIENT,
    SOCKET_CLIENT_REQUEST,
};

struct event {
    enum event_type type;
    int fd;
};

/* persistent epoll event storage */
static struct event button_events[NUM_BUTTONS];
static struct event timer_event;


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
    switch (ev.type) {
        case BUTTON1_PRESS:{
            if (is_button_pressed(ev.fd)){
                set_led(true);
                if (fan_is_manual_mode()) {
                    fan_increase_frequency();
                }
            }
            else {
                set_led(false);
            }
            break;
        }
        case BUTTON2_PRESS:{
            if (is_button_pressed(ev.fd)) {
                set_led(true);
                if (fan_is_manual_mode()) {
                    fan_decrease_frequency();
                }
            }
            else {
                set_led(false);
            }
            break;
        }
        case BUTTON3_PRESS:{
            if (is_button_pressed(ev.fd)){
                set_led(true);
                fan_toggle_mode();
            }
            else {
                set_led(false);
            }
            break;
        }
        case REFRESH_SCREEN: {
            uint64_t expirations;
            read(screen_timer_fd, &expirations, sizeof(expirations));
            screen_refresh();
            break;
        }

        default:
            return -EINVAL;
    }

    return 0;
}

static void daemonize(void)
{
    pid_t pid;

    // Fork and let the parent exit, detaching from the terminal 
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "fork failed: %m");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // Become session leader, detaching from the controlling terminal 
    if (setsid() < 0) {
        syslog(LOG_ERR, "setsid failed: %m");
        exit(EXIT_FAILURE);
    }

    // Fork again so the daemon can never reacquire a controlling terminal 
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "second fork failed: %m");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // Set a neutral working directory so we don't hold a mount point open 
    if (chdir("/") < 0) {
        syslog(LOG_ERR, "chdir failed: %m");
        exit(EXIT_FAILURE);
    }

    // Clear the file creation mask 
    umask(0);

    // Redirect stdin/stdout/stderr to /dev/null 
    int devnull = open("/dev/null", O_RDWR);
    if (devnull < 0) {
        syslog(LOG_ERR, "open /dev/null failed: %m");
        exit(EXIT_FAILURE);
    }
    dup2(devnull, STDIN_FILENO);
    dup2(devnull, STDOUT_FILENO);
    dup2(devnull, STDERR_FILENO);
    if (devnull > STDERR_FILENO)
        close(devnull);
}

/* ---------------- MAIN ---------------- */

int main()
{
    openlog("fan-daemon", LOG_PID | LOG_NDELAY, LOG_DAEMON);

    //daemonize();

    gpios_fd_t *gpios = gpios_init();
    fan_init();
    screen_init();

    int epoll_fd = epoll_create1(0);
    struct epoll_event events[10];

    screen_timer_fd = create_timer(SCREEN_REFRESH_INTERVAL_MS);

    button_events[0] = (struct event){ .type = BUTTON1_PRESS, .fd = gpios->button_fds[0] };
    button_events[1] = (struct event){ .type = BUTTON2_PRESS, .fd = gpios->button_fds[1] };
    button_events[2] = (struct event){ .type = BUTTON3_PRESS, .fd = gpios->button_fds[2] };

    timer_event = (struct event){ .type = REFRESH_SCREEN, .fd = screen_timer_fd };

    struct epoll_event ev1 = { .events = EPOLLPRI, .data.ptr = &button_events[0] };
    struct epoll_event ev2 = { .events = EPOLLPRI, .data.ptr = &button_events[1] };
    struct epoll_event ev3 = { .events = EPOLLPRI, .data.ptr = &button_events[2] };
    struct epoll_event evt = { .events = EPOLLIN,  .data.ptr = &timer_event };

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, gpios->button_fds[0], &ev1);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, gpios->button_fds[1], &ev2);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, gpios->button_fds[2], &ev3);
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