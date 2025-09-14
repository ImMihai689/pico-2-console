#ifndef HW_I2C
#define HW_I2C

// Default I2C instance for the console
#define HW_I2C_INST &i2c0_inst

#define HW_I2C_FRAM_ADDR 0x50
#define HW_I2C_IO_ADDR 0x20
// Device addresses (FRAM IC and I/O extender IC)

#define BUT_A_I2C 5
#define BUT_B_I2C 6
#define BUT_X_I2C 2
#define BUT_Y_I2C 1
#define BUT_MENU_I2C 0
#define BUT_START_I2C 4
#define BUT_SHLD_R_I2C 3
#define BUT_SHLD_L_I2C 7
// What pins buttons A, B, X, Y, MENU, START, SHOULDER R and SHOULDER L
// are on the I2C I/O expander IC

#define SCL 9
#define SDA 8
// I2C pins for FRAM and I/O expander

#include "hardware/i2c.h"
#include "hardware/gpio.h"

void hw_i2c_init();

uint8_t hw_i2c_read_pins();

#endif