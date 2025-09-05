#ifndef HW_I2C
#define HW_I2C

// Default I2C instance for the console
#define HW_I2C_INST &i2c0_inst

#define HW_I2C_FRAM_ADDR 0x50
#define HW_I2C_IO_ADDR 0x20
// Device addresses (FRAM IC and I/O extender IC)

#include "hw_defines.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

void hw_i2c_init();

uint8_t hw_i2c_read_pins();

#endif