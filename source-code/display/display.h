#ifndef DISPLAY_H
#define DISPLAY_H

#include "pico/types.h"
#include "hardware.h"
#include "graphics.h"

#define LCD_DMA_DATA 1
#define LCD_DMA_CTRL 2
// The data channel streams data to the SPI peripheral,
// while the control channel configures the data channel
// for flexible tilemap usage

#define LCD_WIDTH 320
#define LCD_HEIGHT 240

#define LCD_RAMWR 0x2C

/// @brief To be called by core1 during hw_init()
void lcd_init();

/// @brief Write a command to LCD with optional arguments (blocking)
/// @param cmd The command
/// @param args The array of arguments
/// @param num_args The number of arguments
void lcd_write_command(uint8_t cmd, const uint8_t *args, uint num_args);

/// @brief Set the brighness of the LCD backlight
/// @param intensity any value in [0, 32], where 0 is off, and 32 is full on
void lcd_set_backlight(int intensity);

/// @brief Set the LCD's color mode (slow function, takes 10ms) (by default 565-bit colors)
/// @param mode If false, 12-444-bit colors (4k colors). If true, 16-565-bit colors(65k colors)
void lcd_set_color_mode(bool mode);



#endif