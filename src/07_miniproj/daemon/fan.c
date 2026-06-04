#include "fan.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static bool fan_manual_mode = false;
static int fan_manual_fd = -1;
static int fan_freq_fd = -1;
static int fan_temp_fd = -1;

void fan_init()
{
    fan_manual_fd = open(FAN_MANUAL_MODE, O_RDWR);
    fan_freq_fd = open(FAN_FREQUENCY, O_RDWR);
    fan_temp_fd   = open(FAN_CPU_TEMP,    O_RDONLY);

    char buf[20];
    read(fan_manual_fd, buf, 1);
    fan_manual_mode = (buf[0] == '1');
}

bool fan_is_manual_mode()
{
    return fan_manual_mode;
}

int fan_get_cpu_temp()
{
    char buf[20] = {0};
    lseek(fan_temp_fd, 0, SEEK_SET);
    ssize_t n = read(fan_temp_fd, buf, sizeof(buf) - 1);
    if (n < 0) {
        perror("read cpu temp");
        return -1;
    }
    return atoi(buf);
}

int fan_get_frequency()
{
    char buf[20] = {0};
    lseek(fan_freq_fd, 0, SEEK_SET);
    ssize_t n = read(fan_freq_fd, buf, sizeof(buf) - 1);
    if (n < 0) {
        perror("read frequency");
        return -1;
    }
    return atoi(buf);
}

void fan_toggle_mode()
{
    fan_manual_mode = !fan_manual_mode;
    const char *val = fan_manual_mode ? "1" : "0";
    write(fan_manual_fd, val, 1);
}

void fan_set_frequency(int freq)
{
    char buf[20];
    snprintf(buf, sizeof(buf), "%d", freq);
    write(fan_freq_fd, buf, 20);
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

void fan_increase_frequency()
{
    int freq = fan_read_frequency();
    freq += FAN_FREQUENCY_STEP;
    if (freq > FAN_FREQUENCY_MAX) freq = FAN_FREQUENCY_MAX;
    fan_set_frequency(freq);
}

void fan_decrease_frequency()
{
    int freq = fan_read_frequency();
    freq -= FAN_FREQUENCY_STEP;
    if (freq < FAN_FREQUENCY_MIN) freq = FAN_FREQUENCY_MIN;
    fan_set_frequency(freq);
}