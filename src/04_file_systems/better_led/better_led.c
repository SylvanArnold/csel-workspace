#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>

#include <stdbool.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>


/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 */
#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_LED      "/sys/class/gpio/gpio10"
#define LED           "10"


#define BUTTON1_GPIO 0
#define BUTTON2_GPIO 2
#define BUTTON3_GPIO 3

#define NUM_BUTTONS 3

int buttons_gpios[NUM_BUTTONS] = {BUTTON1_GPIO, BUTTON2_GPIO, BUTTON3_GPIO};


#define MIN_BLINK_PERIOD_MS 50
#define MAX_BLINK_PERIOD_MS 500
#define BLINK_STEP_MS 50

// Function prototypes
void set_timer_period(int tfd);
void set_led_state(int led, int state);

// Shared data between threads
int blink_frequ_ms = MAX_BLINK_PERIOD_MS;
pthread_mutex_t lock; // Protects access to shared data

static int open_led()
{
    // unexport pin out of sysfs (reinitialization)
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // export pin to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // config pin (set as output)
    f = open(GPIO_LED "/direction", O_WRONLY);
    write(f, "out", 3);
    close(f);

    // open gpio value attribute
    f = open(GPIO_LED "/value", O_RDWR);
    return f;
}

// Thread function for handling button events and updating the blink frequency
void* button_thread_func(void* arg)
{
    int epoll_fd = *((int*)arg); // Get the epoll file descriptor passed from main

    struct epoll_event events[NUM_BUTTONS]; // Buffer to store events returned by epoll_wait max size set to buttons number

    while (1) {
        int n = epoll_wait(epoll_fd, events, NUM_BUTTONS, -1);
        if (n == -1) {
            printf("Failed to wait for events: %s\n", strerror(errno));
            break;
        }

        for (int i = 0; i < n; i++) {
            pthread_mutex_lock(&lock);
            if (events[i].data.u32 == 0) { // Button 1 increases frequency (decrease period)
                blink_frequ_ms = (blink_frequ_ms - BLINK_STEP_MS < MIN_BLINK_PERIOD_MS) ? MIN_BLINK_PERIOD_MS : blink_frequ_ms - BLINK_STEP_MS;
            } else if (events[i].data.u32 == 1) { // Button 2 resets frequency
                blink_frequ_ms = MAX_BLINK_PERIOD_MS;
            } else if (events[i].data.u32 == 2) { // Button 3 decreases frequency (increase period)
                blink_frequ_ms = (blink_frequ_ms + BLINK_STEP_MS > MAX_BLINK_PERIOD_MS) ? MAX_BLINK_PERIOD_MS : blink_frequ_ms + BLINK_STEP_MS;
            }
            printf("New period: %d ms\n", blink_frequ_ms);
            syslog(LOG_INFO, "New period: %d ms", blink_frequ_ms);
            pthread_mutex_unlock(&lock);
        }
    }
    return NULL;
}

// Thread function for controlling the LED blinking
void* led_thread_func(void* arg)
{
    int led = *((int*)arg);
    int led_state = 0;

    uint64_t expirations;

    // Create a timer file descriptor for controlling the LED blinking
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd == -1) {
        printf("Failed to create LED timer file descriptor");
        return NULL;
    }


    while (1) {
        set_timer_period(tfd); // Update timer period based on current frequency
        if (read(tfd, &expirations, sizeof(expirations)) == -1) {
            printf("Failed to read from LED timer file descriptor: %s\n", strerror(errno));
            break;
        }
        led_state = !led_state; // Toggle LED state
        set_led_state(led, led_state);
    }
    return NULL;
}

void set_timer_period(int tfd)
{
    struct itimerspec timer;

    pthread_mutex_lock(&lock);
    timer.it_value.tv_sec = blink_frequ_ms / 1000;
    timer.it_value.tv_nsec = (blink_frequ_ms % 1000) * 1000000;
    timer.it_interval = timer.it_value;  // periodic
    pthread_mutex_unlock(&lock);

    if (timerfd_settime(tfd, 0, &timer, NULL) == -1) {
        printf("Failed to set LED timer period: %s\n", strerror(errno));
    }
}

void set_led_state(int led, int state)
{
    pwrite(led, state ? "1" : "0", 1, 0);
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    // Open syslog for logging
    openlog("better_led", LOG_PID | LOG_CONS, LOG_USER);

    int led = open_led();
    set_led_state(led, 1); // Turn LED on initially

    int buttons[NUM_BUTTONS];
    
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        printf("Failed to create epoll instance: %s\n", strerror(errno));
        return 1;
    }

    // Initialise each button GPIO: set as input, configure edge detection, and add to epoll instance
    for (int i = 0; i < NUM_BUTTONS; i++) {
        char buf[64];
        int len;


        // Unexport GPIO to reset if it was already exported
        int f = open(GPIO_UNEXPORT, O_WRONLY);
        len = sprintf(buf, "%d", buttons_gpios[i]);
        write(f, buf, len);
        close(f);

        // Export GPIO
        f = open(GPIO_EXPORT, O_WRONLY);
        len = sprintf(buf, "%d", buttons_gpios[i]);
        write(f, buf, len);
        close(f);

        // Set direction to "in"
        len = sprintf(buf, "/sys/class/gpio/gpio%d/direction", buttons_gpios[i]);
        f = open(buf, O_WRONLY);
        write(f, "in", 2);
        close(f);

        // Set edge to "rising"
        len = sprintf(buf, "/sys/class/gpio/gpio%d/edge", buttons_gpios[i]);
        f = open(buf, O_WRONLY);
        write(f, "rising", 6);
        close(f);

        // Open value file
        len = sprintf(buf, "/sys/class/gpio/gpio%d/value", buttons_gpios[i]);
        buttons[i] = open(buf, O_RDONLY | O_NONBLOCK);

        struct epoll_event event; // Create an epoll event for this button
        event.events = EPOLLPRI | EPOLLET; // Trigger on rising edge and use edge-triggered mode
        event.data.u32 = i; // Store the button index in the event data
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, buttons[i], &event) == -1) { // Link the button's file descriptor to the epoll instance with the event configuration
            printf("Failed to add file descriptor to epoll instance: %s\n", strerror(errno));
            return 1;
        }
    }

    pthread_t button_thread, led_thread;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("Mutex init failed\n");
        return 1;
    }

    // Create threads for handling button events and controlling the LED
    pthread_create(&button_thread, NULL, button_thread_func, &epoll_fd);
    pthread_create(&led_thread, NULL, led_thread_func, &led);

    // Wait for threads to finish
    pthread_join(button_thread, NULL);
    pthread_join(led_thread, NULL);

    // Clean up
    pthread_mutex_destroy(&lock);
    close(led);
    closelog();

    return 0;
}