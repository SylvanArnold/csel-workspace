#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/inotify.h>

/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 */
#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_LED      "/sys/class/gpio/gpio10"
#define LED           "10"

int buttons_gpios[] = {0, 2, 3};
#define NUM_BUTTONS (sizeof(buttons_gpios) / sizeof(int))



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

int main(int argc, char* argv[])
{
    int led = open_led();
    pwrite(led, "1", sizeof("1"), 0); // turn on led at the beginning

    int buttons[NUM_BUTTONS];
    
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        printf("Failed to create epoll instance: %s\n", strerror(errno));
        return 1;
    }

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


    struct epoll_event events[NUM_BUTTONS]; // Create an array to store epoll events of size equal to the number of buttons
    int duty_cycle = 50; // Initial duty cycle of 50%
    int period_ms = 1000; // 1 second period

    while (1) {
        int n = epoll_wait(epoll_fd, events, NUM_BUTTONS, period_ms); // Wait for events or timeout
        if (n == -1) {
            printf("Failed to wait for events: %s\n", strerror(errno));
            break;
        }

        // Handle button presses
        for (int i = 0; i < n; i++) {
            printf("Button %d pressed\n", events[i].data.u32);
            if (events[i].data.u32 == 0) { // Button 0 increases duty cycle
                duty_cycle = (duty_cycle + 10 > 100) ? 100 : duty_cycle + 10;
            } else if (events[i].data.u32 == 1) { // Button 1 decreases duty cycle
                duty_cycle = (duty_cycle - 10 < 0) ? 0 : duty_cycle - 10;
            }
             printf("New duty cycle: %d%%\n", duty_cycle);
        }

        // Handle LED blinking
        pwrite(led, "1", 1, 0); // Turn LED on
        usleep(period_ms * 10 * duty_cycle); // On time
        pwrite(led, "0", 1, 0); // Turn LED off
        usleep(period_ms * 10 * (100 - duty_cycle)); // Off time
    }
    

    return 0;
}