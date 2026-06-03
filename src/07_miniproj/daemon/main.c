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

#define FAN_MANUAL_MODE "/sys/devices/platform/fan-controller/manual_mode"
#define FAN_FREQUENCY   "/sys/devices/platform/fan-controller/frequency"
#define FAN_CPU_TEMP "/sys/devices/platform/fan-controller/cpu_temp"

#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_DIR      "/sys/class/gpio"

#define BUTTON1_GPIO 0
#define BUTTON2_GPIO 2
#define BUTTON3_GPIO 3
#define NUM_BUTTONS 3
static const int BUTTON_GPIOS[NUM_BUTTONS] = { BUTTON1_GPIO, BUTTON2_GPIO, BUTTON3_GPIO };

#define POWER_LED_GPIO 362


#define SCREEN_REFRESH_INTERVAL_MS 100
#define FAN_FREQUENCY_MAX 100
#define FAN_FREQUENCY_MIN 1
#define FAN_FREQUENCY_STEP 1

static int led_fd;
static int button_fds[NUM_BUTTONS];
static int screen_timer_fd;

static bool fan_manual_mode = false;
static int fan_manual_fd;
static int fan_freq_fd;
static int fan_temp_fd;

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
static struct event button_events[NUM_BUTTONS];
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

    snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", POWER_LED_GPIO);
    led_fd = open(path, O_WRONLY);

    // configure buttons
    for (int i = 0; i < NUM_BUTTONS; i++) {
        int gpio = BUTTON_GPIOS[i];

        snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/direction", gpio);
        f = open(path, O_WRONLY);
        if (f >= 0) {
            write(f, "in", 2);
            close(f);
        }

        snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/edge", gpio);
        f = open(path, O_WRONLY);
        if (f >= 0) {
            write(f, "both", 4);
            close(f);
        }

        snprintf(path, sizeof(path), GPIO_DIR "/gpio%d/value", gpio);
        button_fds[i] = open(path, O_RDONLY | O_NONBLOCK);
    }

    /* clear initial state */
    for (int i = 0; i < NUM_BUTTONS; i++) {
        char buf[8];
        lseek(button_fds[i], 0, SEEK_SET);
        read(button_fds[i], buf, sizeof(buf));
    }
}

static void fan_init()
{
    fan_manual_fd = open(FAN_MANUAL_MODE, O_RDWR);
    fan_freq_fd = open(FAN_FREQUENCY, O_RDWR);
    fan_temp_fd   = open(FAN_CPU_TEMP,    O_RDONLY);

    char buf[20];
    read(fan_manual_fd, buf, 1);
    fan_manual_mode = (buf[0] == '1');
}

static void fan_toggle_mode()
{
    fan_manual_mode = !fan_manual_mode;
    const char *val = fan_manual_mode ? "1" : "0";
    write(fan_manual_fd, val, 1);
}

static void fan_set_frequency(int freq)
{
    char buf[20];
    snprintf(buf, sizeof(buf), "%d", freq);
    write(fan_freq_fd, buf, strlen(buf));
}

int fan_read_frequency()
{
    char buf[20] = {0};

    lseek(fan_freq_fd, 0, SEEK_SET);

    ssize_t n = read(fan_freq_fd, buf, sizeof(buf)-1);
    if (n < 0) {
        perror("read frequency");
        return -1;
    }

    return atoi(buf);
}

static void fan_increase_frequency()
{
    int freq = fan_read_frequency();
    freq += FAN_FREQUENCY_STEP;
    if (freq > FAN_FREQUENCY_MAX) freq = FAN_FREQUENCY_MAX;
    fan_set_frequency(freq);
}

static void fan_decrease_frequency()
{
    int freq = fan_read_frequency();
    freq -= FAN_FREQUENCY_STEP;
    if (freq < FAN_FREQUENCY_MIN) freq = FAN_FREQUENCY_MIN;
    fan_set_frequency(freq);
}

static void screen_init()
{
    ssd1306_init();
    ssd1306_clear_display();
    ssd1306_set_position (0,0);
    ssd1306_puts("CSEL1a - SP.07");
    ssd1306_set_position (0,1);
    ssd1306_puts("  Demo - SW");
    ssd1306_set_position (0,2);
    ssd1306_puts("--------------");
    ssd1306_set_position (0,3);
    ssd1306_puts("Mode: Unknown");
    ssd1306_set_position (0,4);
    ssd1306_puts("--------------");
    ssd1306_set_position (0,5);
    ssd1306_puts("Temp: xx'C");
    ssd1306_set_position (0,6);
    ssd1306_puts("Freq: xxHz");
}

static void screen_refresh()
{
    char freq_buf[20] = {0};
    lseek(fan_freq_fd, 0, SEEK_SET);
    read(fan_freq_fd, freq_buf, sizeof(freq_buf) - 1);

    ssd1306_set_position(0, 3);
    ssd1306_puts(fan_manual_mode ? "Mode: Manual " : "Mode: Auto   ");

    // Read cpu_temp (milli-Celsius) and convert to whole degrees
    char temp_buf[20] = {0};
    lseek(fan_temp_fd, 0, SEEK_SET);
    ssize_t n = read(fan_temp_fd, temp_buf, sizeof(temp_buf) - 1);
    
    ssd1306_set_position(0, 5);
    if (n > 0) {
        int temp_mc = atoi(temp_buf);
        char display[20];
        snprintf(display, sizeof(display), "Temp: %d'C  ", temp_mc / 1000);
        ssd1306_puts(display);
    } else {
        ssd1306_puts("Temp: --'C  ");
    }

    ssd1306_set_position(0, 6);
    ssd1306_puts("Freq: ");
    ssd1306_puts(freq_buf);
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

bool is_button_pressed(int fd)
{
    char buf[4];
    lseek(fd, 0, SEEK_SET);
    read(fd, buf, sizeof(buf));
    return buf[0] == '1';
}

void set_led(bool on)
{
    if (led_fd >= 0) {
        const char *val = on ? "1" : "0";
        write(led_fd, val, 1);
    }
}

/* ---------------- EVENT HANDLER ---------------- */

int process_event(struct event ev)
{
    switch (ev.type) {
        case BUTTON1_PRESS:{
            if (is_button_pressed(ev.fd)){
                set_led(true);
                if (fan_manual_mode) {
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
                if (fan_manual_mode) {
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

    gpios_init();
    fan_init();
    screen_init();

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