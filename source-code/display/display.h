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
void lcd_init(void);

/// @brief Write a command to LCD with optional arguments using SPI (blocking)
/// @param cmd The command
/// @param args The array of arguments
/// @param num_args The number of arguments
void lcd_write_command(uint8_t cmd, const uint8_t *args, uint num_args);

/// @brief Set the brighness of the LCD backlight
/// @param intensity any value in [0, 32], where 0 is off, and 32 is full on
void lcd_set_backlight(int intensity);

/// @brief Set the LCD's mode. (slow, 10ms)
/// @param mode If true, normal resolution (240x320) with 12-bit color.
///             If false, half resolution (120x160) with 16-bit color.
void lcd_set_mode(bool mode);

/// @brief Check if DMA is busy writting to the display
/// @return True if busy
inline bool lcd_is_writting();

/// @brief Fill the screen with black, without a display buffer, using DMA
void lcd_clear_screen(void);



#endif