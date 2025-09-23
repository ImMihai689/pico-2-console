#include "display.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pio.pio.h"

#include <stdio.h>

void dma_handler(void)
{
    if(dma_irqn_get_channel_status(DMA_IRQ_2, LCD_DMA_DATA))
    {
        dma_irqn_acknowledge_channel(DMA_IRQ_2, LCD_DMA_DATA);
    }
}

void lcd_init()
{
    dma_claim_mask(1 << LCD_DMA_CTRL | 1 << LCD_DMA_DATA);
    // default config is actually exactly the configuration needed for this
    dma_channel_config c = dma_channel_get_default_config(LCD_DMA_CTRL);
    dma_channel_configure(
        LCD_DMA_CTRL,
        &c,
        &dma_hw->ch[LCD_DMA_DATA].al3_read_addr_trig,
        NULL,
        1,
        false
    );
    
    
    c = dma_channel_get_default_config(LCD_DMA_DATA);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    // The IRQ will be used to increment the writting column,
    // due to ST7789 "limitations"
    channel_config_set_irq_quiet(&c, true);
    channel_config_set_dreq(&c, SPI_DREQ_NUM(LCD_SPI, true));
    channel_config_set_chain_to(&c, LCD_DMA_CTRL);
    dma_channel_configure(
        LCD_DMA_DATA,
        &c,
        &spi_get_hw(LCD_SPI)->dr,
        NULL,
        384,    // transfer size in bytes of one 16x16 tile in 444bit color mode
        false
    );

    dma_irqn_set_channel_enabled(2, LCD_DMA_DATA, true);
    irq_set_exclusive_handler(DMA_IRQ_2, dma_handler);
    irq_set_priority(DMA_IRQ_2, 0x40);
    

    uint offset = pio_add_program(LCD_PIO, &bl_pwm_program);
    pio_sm_config bl_cfg = bl_pwm_program_get_default_config(offset);
    sm_config_set_clkdiv(&bl_cfg, 2500.0f);
    sm_config_set_sideset_pin_base(&bl_cfg, LCD_BL);
    pio_sm_init(LCD_PIO, LCD_BL_SM, offset, &bl_cfg);
    pio_sm_set_consecutive_pindirs(LCD_PIO, LCD_BL_SM, LCD_BL, 1, true);
    pio_sm_put_blocking(LCD_PIO, LCD_BL_SM, 255);
    pio_sm_set_enabled(LCD_PIO, LCD_BL_SM, true);

    /// test this
    LCD_PIO->input_sync_bypass |= (1 << LCD_SDI | 1 << LCD_SCL);
    uint rx_offset = pio_add_program_at_offset(LCD_PIO, &lcd_rx_program, 16);
    pio_sm_config rx_cfg = lcd_rx_program_get_default_config(rx_offset);
    sm_config_set_clkdiv_int_frac(&rx_cfg, 14, 0);
    sm_config_set_in_pin_base(&rx_cfg, LCD_SDI);
    sm_config_set_sideset_pin_base(&rx_cfg, LCD_SCL);
    pio_sm_init(LCD_PIO, LCD_RX_SM, rx_offset, &rx_cfg);
    pio_sm_set_consecutive_pindirs(LCD_PIO, LCD_RX_SM, LCD_SDI, 1, false);
    pio_sm_set_consecutive_pindirs(LCD_PIO, LCD_RX_SM, LCD_SCL, 1, true);
    pio_sm_set_enabled(LCD_PIO, LCD_RX_SM, true);

    gpio_init(LCD_BL);
    gpio_set_dir(LCD_BL, true);
    gpio_put(LCD_BL, false);

    gpio_init_mask(1 << LCD_BL | 1 << LCD_CS | 1 << LCD_DC | 1 << LCD_FMARK | 1 << LCD_RST);
    gpio_set_dir_out_masked(1 << LCD_BL | 1 << LCD_CS | 1 << LCD_DC | 1 << LCD_RST);
    gpio_set_dir_in_masked(1 << LCD_FMARK);
    gpio_set_pulls(LCD_FMARK, true, false);

    gpio_put(LCD_BL, false);
    gpio_put(LCD_RST, true);
    gpio_put(LCD_CS, true);

    gpio_set_function_masked(1 << LCD_SDI | 1 << LCD_SCL, GPIO_FUNC_SPI);
    
    spi_init(LCD_SPI, 100 * 1000 * 1000);

    wait_us(1000);
    lcd_write_command(0x01, NULL, 0);   // Soft reset, no args
    wait_us(150 * 1000);
    lcd_write_command(0x11, NULL, 0);   // Sleep ou, no args
    wait_us(10 * 1000);
    lcd_write_command(0x3A, (const uint8_t[]){0x53}, 1);    // Color mode - 16bit color, to boot
    wait_us(10 * 1000);
    lcd_write_command(0x36, (const uint8_t[]){0xB0}, 1);    // Memory data access control
    lcd_write_command(0x2A, (const uint8_t[]){0, 0, 1, 319 & 0xFF}, 4);    // Column address set
    lcd_write_command(0x2B, (const uint8_t[]){0, 0, 0, 239}, 4);    // Row address set
    lcd_write_command(0xC6, (const uint8_t[]){0x1E}, 1);    // Frame Rate Control (40 fps)
    lcd_write_command(0x35, (const uint8_t[]){0x00}, 1);    // Tearing Effect Line On
    //lcd_write_command(0x21, NULL, 0);
    //wait_us(10 * 1000);
    lcd_write_command(0x13, NULL, 0);
    wait_us(10 * 1000);
    lcd_write_command(0x29, NULL, 0);
    wait_us(10 * 1000);
    
    uint16_t pixel = 73; // 0xFFFF;
    uint8_t ramwr = 0x2C;

    gpio_put(LCD_CS, false);
    gpio_put(LCD_DC, false);
    spi_write_blocking(LCD_SPI, &ramwr, 1);
    gpio_put(LCD_DC, true);
    for(int i = 0; i < 115200; i++)
    {
        spi_write_blocking(LCD_SPI, (uint8_t *)&pixel, 2);
    }
    gpio_put(LCD_CS, true);

    gpio_put(LCD_BL, false);
    gpio_set_function(LCD_BL, GPIO_FUNC_SIO);

    gpio_init(LCD_FMARK);
    gpio_pull_up(LCD_FMARK);
}

void lcd_write_command(uint8_t cmd, const uint8_t *args, uint num_args)
{
    gpio_put(LCD_CS, false);
    gpio_put(LCD_DC, false);
    spi_write_blocking(LCD_SPI, &cmd, 1);
    gpio_put(LCD_DC, true);
    if(num_args)
        spi_write_blocking(LCD_SPI, args, num_args);
    gpio_put(LCD_CS, true);
}

void lcd_set_backlight(int intensity)
{
    if(intensity < 1)
    {
        gpio_put(LCD_BL, false);
        gpio_set_function(LCD_BL, GPIO_FUNC_SIO);
        return;
    }
    if(intensity > 255)
    {
        gpio_put(LCD_BL, true);
        gpio_set_function(LCD_BL, GPIO_FUNC_SIO);
        return;
    }

    pio_sm_put_blocking(LCD_PIO, LCD_BL_SM, intensity);
    gpio_set_function(LCD_BL, GPIO_FUNC_PIO0);
}

void lcd_read_tile()
{
    lcd_write_command(0x2A, (const uint8_t[]){0, 0, 0, 15}, 4);   // Column address set
    lcd_write_command(0x2B, (const uint8_t[]){0, 0, 0, 15}, 4);    // Row address set
    lcd_write_command(0x3A, (const uint8_t[]){0x53}, 1);    // Row address set

    uint8_t pixelr = 0b11111111;// 0xFFFF;
    uint8_t pixelg = 0b00001111;// 0xFFFF;
    uint8_t pixelb = 0b00001111;// 0xFFFF;
    uint8_t ramwr = 0x2C;

    gpio_put(LCD_CS, false);
    gpio_put(LCD_DC, false);
    spi_write_blocking(LCD_SPI, &ramwr, 1);
    gpio_put(LCD_DC, true);
    for(int i = 0; i < 128; i++)
    {
        spi_write_blocking(LCD_SPI, &pixelr, 1);
        spi_write_blocking(LCD_SPI, &pixelg, 1);
        spi_write_blocking(LCD_SPI, &pixelb, 1);
    }
    gpio_put(LCD_CS, true);
    


    gpio_put(LCD_CS, false);
    gpio_put(LCD_DC, false);
    uint8_t cmd = 0x2E;
    spi_write_blocking(LCD_SPI, &cmd, 1);
    gpio_put(LCD_DC, true);
    pio_gpio_init(LCD_PIO, LCD_SDI);
    pio_gpio_init(LCD_PIO, LCD_SCL);

    uint remain = 0;

    pio_sm_put_blocking(LCD_PIO, LCD_RX_SM, 3072);

    //while(remain < 384)
    //{
    //    ((uint8_t *)tile->pixels)[remain++] = (uint8_t)pio_sm_get_blocking(LCD_PIO, LCD_RX_SM);
    //}

    gpio_set_function(LCD_SCL, GPIO_FUNC_SPI);
    gpio_set_function(LCD_SDI, GPIO_FUNC_SPI);

    gpio_put(LCD_CS, true);
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

