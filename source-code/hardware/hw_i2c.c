#include "hw_i2c.h"

void hw_i2c_init()
{
    i2c_init(HW_I2C_INST, 10000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_set_pulls(SDA, true, false);
    gpio_set_pulls(SCL, true, false);
}