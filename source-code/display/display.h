#ifndef DISPLAY_H
#define DISPLAY_H

#include "pico/types.h"
#include "hardware.h"
#include "bmplib.h"

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
/// @param intensity any value in [0, 100], where 0 is off, and 100 is full on
void lcd_set_backlight(int intensity);

/// @brief Set the LCD's mode. (slow, 10ms)
/// @param mode If true, normal resolution (240x320) with 12-bit color.
///             If false, half resolution (120x160) with 16-bit color.
void lcd_set_mode(bool mode);

/// @brief Check if DMA is busy writting to the display
/// @return True if busy
bool lcd_is_writting();

/// @brief Fill the screen with black, without a display buffer, using DMA
/// @note Returns after *starting* transfer
void lcd_clear_screen(void);

/// @brief Write the data buffer to the screen
/// @param buff The pixel data array
/// @attention Make sure the data buffer is the right size!
void lcd_write_buffer(uint8_t *buff);

void draw_to_buffer(uint8_t *buff, BITMAP *bmp, uint16_t x, uint16_t y);



#endif