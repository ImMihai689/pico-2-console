#ifndef HW_LCD
#define HW_LCD


#define LCD_SCL 14
#define LCD_SDI 15
#define LCD_CS 13
#define LCD_DC 12
#define LCD_RST 11
#define LCD_FMARK 10
#define LCD_BL 6
// LCD interface pins
// SCL - Serial CLock, SDI - Serial Data In
// BL is backlight

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/time.h"

#define LCD_SPI spi1

void hw_lcd_init();


#endif