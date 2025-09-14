#include "hw_thumb.h"

void hw_thumb_init()
{
    gpio_init(THUMB_SW);
    gpio_set_pulls(THUMB_SW, true, false);
    gpio_set_dir(THUMB_SW, false);

    adc_init();
    adc_gpio_init(THUMB_X);
    adc_gpio_init(THUMB_Y);
}

uint hw_thumb_read_x()
{
    adc_select_input(1);
    return adc_read();
}

uint hw_thumb_read_y()
{
    adc_select_input(0);
    return adc_read();
}

uint hw_thumb_read_sw()
{
    return gpio_get(THUMB_SW);
}