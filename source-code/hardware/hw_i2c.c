#include "hw_i2c.h"

void hw_i2c_init()
{
    i2c_init(HW_I2C_INST, 100 * 1000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_set_pulls(SDA, true, false);
    gpio_set_pulls(SCL, true, false);
}

uint8_t hw_i2c_read_pins()
{
    uint8_t fin = 0;
    i2c_read_blocking(HW_I2C_INST, HW_I2C_IO_ADDR, &fin, 1, false);
    return fin;
}