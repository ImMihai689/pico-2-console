#ifndef HW_MAIN
#define HW_MAIN

#include "hw_defines.h"
#include "hardware/gpio.h"

#include "hw_i2c.h"

// Initialize all hardware
void hw_init()
{
    gpio_init(PICO_INFO_LED);
    gpio_set_dir(PICO_INFO_LED, true);
    gpio_set_drive_strength(PICO_INFO_LED, GPIO_DRIVE_STRENGTH_4MA);
    gpio_put(PICO_INFO_LED, true); // Active LOW

    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, true);
    gpio_set_drive_strength(BUZZER, GPIO_DRIVE_STRENGTH_4MA);
    gpio_put(BUZZER, true); // Active LOW

    gpio_init(HAPTIC);
    gpio_set_dir(HAPTIC, true);
    gpio_set_drive_strength(HAPTIC, GPIO_DRIVE_STRENGTH_4MA);
    gpio_put(HAPTIC, true); // Active LOW
}

#endif