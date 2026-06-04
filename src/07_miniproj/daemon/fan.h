#ifndef FAN_H
#define FAN_H

#include <stdbool.h>

#define FAN_MANUAL_MODE "/sys/devices/platform/fan-controller/manual_mode"
#define FAN_FREQUENCY   "/sys/devices/platform/fan-controller/frequency"
#define FAN_CPU_TEMP "/sys/devices/platform/fan-controller/cpu_temp"

#define FAN_FREQUENCY_MAX 100
#define FAN_FREQUENCY_MIN 1
#define FAN_FREQUENCY_STEP 1

void fan_init();
void fan_toggle_mode();
void fan_set_frequency(int freq);
int fan_read_frequency();
void fan_increase_frequency();
void fan_decrease_frequency();
bool fan_is_manual_mode();
int fan_get_cpu_temp();

#endif 