#ifndef DISPLAY_H
#define DISPLAY_H

#include "pico/types.h"
#include "hardware.h"
#include "graphics.h"

#define LCD_WIDTH 320
#define LCD_HEIGHT 240

#define LCD_RAMWR 0x2C


/// @brief Set the brighness of the LCD backlight
/// @param intensity any value in [0, 32], where 0 is off, and 32 is full on
void lcd_set_backlight(int intensity);

/// @brief Set the LCD's color mode (slow function, takes 10ms) (by default 565-bit colors)
/// @param mode If false, 12-444-bit colors (4k colors). If true, 16-565-bit colors(65k colors)
void lcd_set_color_mode(bool mode);

/// @brief Draw a bitmap on the display
/// @param bitmap The bitmap reference
void lcd_draw_bitmap(const bitmap_565_t *bitmap);

#endif