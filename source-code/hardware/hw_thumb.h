#ifndef HW_THUMB
#define HW_THUMB

#define THUMB_SW 5
#define THUMB_X 27
#define THUMB_Y 26
// The thumbstick pins on the Pico, X and Y are analog

#include "hardware/adc.h"
#include "hardware/gpio.h"

void hw_thumb_init();

uint hw_thumb_read_x();

uint hw_thumb_read_y();

uint hw_thumb_read_sw();

#endif