#include "screen.h"
#include "ssd1306.h"
#include "fan.h"
#include <stdio.h>
#include <stdbool.h>

void screen_init()
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

void screen_refresh()
{
    int freq = fan_read_frequency();
    int temp_mc = fan_get_cpu_temp();
    bool manual = fan_is_manual_mode();

    ssd1306_set_position(0, 3);
    ssd1306_puts(manual ? "Mode: Manual " : "Mode: Auto   ");
    
    ssd1306_set_position(0, 5);
    if (temp_mc >= 0) {
        char display[20];
        snprintf(display, sizeof(display), "Temp: %d'C  ", temp_mc / 1000);
        ssd1306_puts(display);
    } else {
        ssd1306_puts("Temp: --'C  ");
    }

    ssd1306_set_position(0, 6);
    ssd1306_puts("Freq: ");
    char freq_buf[20];
    snprintf(freq_buf, sizeof(freq_buf), "%dHz", freq);
    ssd1306_puts(freq_buf);
}