#include "display.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/pio.h"




void lcd_set_backlight(int intensity)
{
    if(intensity < 1)
    {
        gpio_put(LCD_BL, false);
        gpio_set_function(LCD_BL, GPIO_FUNC_SIO);
        return;
    }
    if(intensity > 31)
    {
        gpio_put(LCD_BL, true);
        gpio_set_function(LCD_BL, GPIO_FUNC_SIO);
        return;
    }

    pio_sm_put_blocking(LCD_BL_PIO, LCD_BL_PIO_SM, intensity);
    gpio_set_function(LCD_BL, GPIO_FUNC_PIO0);
}

void lcd_set_write_rect(uint x0, uint y0, uint x1, uint y1)
{
    if(x0 > x1)
    {
        x0 ^= x1;
        x1 ^= x0;
        x0 ^= x1;
    }
    if(y0 > y1)
    {
        y0 ^= y1;
        y1 ^= y0;
        y0 ^= y1;
    }
    if(x1 >= 320)
        x1 = 319;
    if(y1 >= 240)
        y1 = 239;
    
    lcd_write_command(0x2A, (const uint8_t[]){x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF}, 4);   // Column address set
    lcd_write_command(0x2B, (const uint8_t[]){0, y0, 0, y1}, 4);    // Row address set
}

void lcd_set_color_mode(bool mode)
{
    lcd_write_command(0x3A, (const uint8_t[]){0x55}, 1);    // Color mode - 16bit color, to boot
    wait_us(10 * 1000);
}

void lcd_draw_bitmap(const bitmap_565_t *bitmap)
{
    lcd_set_write_rect(bitmap->x0, bitmap->y0, bitmap->x1, bitmap->y1);
    lcd_write_command(LCD_RAMWR, (uint8_t *)&bitmap->data, 2 * bitmap->pixels); // Can only send 1 byte at a time, but pixels are 2 bytes
}