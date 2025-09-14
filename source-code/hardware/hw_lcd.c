#include "hw_lcd.h"

const uint8_t init_commands[] = {
    0x01,           // Soft reset
    0x11,           // Sleep out
    0x3A, 0x55,     // Pixel format (RGB 565)
    0x20,           // Invert color
    0x29            // Display On
};

void hw_lcd_init()
{
    
    gpio_init(LCD_BL);
    gpio_set_dir(LCD_BL, true);
    gpio_put(LCD_BL, true);

    gpio_init_mask(1 << LCD_BL | 1 << LCD_CS | 1 << LCD_DC | 1 << LCD_FMARK | 1 << LCD_RST);
    gpio_set_dir_out_masked(1 << LCD_BL | 1 << LCD_CS | 1 << LCD_DC | 1 << LCD_RST);
    gpio_set_dir_in_masked(1 << LCD_FMARK);
    gpio_set_pulls(LCD_FMARK, true, false);

    gpio_put(LCD_BL, false);
    gpio_put(LCD_RST, true);
    gpio_put(LCD_CS, true);

    gpio_set_function_masked(1 << LCD_SDI | 1 << LCD_SCL, GPIO_FUNC_SPI);
    
    spi_init(LCD_SPI, 1 * 1000 * 1000);

    sleep_ms(10);
    gpio_put(LCD_RST, false);
    sleep_us(100);
    gpio_put(LCD_RST, true);
    sleep_ms(120);

    gpio_put(LCD_DC, false);
    gpio_put(LCD_CS, false);
    spi_write_blocking(LCD_SPI, init_commands, 3);
    gpio_put(LCD_DC, true);
    spi_write_blocking(LCD_SPI, init_commands + 3, 1);
    gpio_put(LCD_DC, false);
    spi_write_blocking(LCD_SPI, init_commands + 4, 2);
    gpio_put(LCD_CS, true);

    gpio_put(LCD_BL, true);


}