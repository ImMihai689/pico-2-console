#ifndef HW_MAIN
#define HW_MAIN

#include "hw_defines.h"
#include "hardware/gpio.h"

#include "hw_pwm.h"
#include "hw_i2c.h"
#include "hw_thumb.h"
#include "hw_lcd.h"

// Initialize all hardware
void hw_init()
{
    hw_lcd_init();

    hw_pwm_init();

    hw_i2c_init();

    hw_thumb_init();
}

#endif