#include "display.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "display.pio.h"

#include <stdio.h>

// True for full resolution, 12bit color
// False for half resolution, 16bit color
bool lcd_mode = true;

bool raise_cs_on_irq = false;

#define BUFFER_BPP 12

#ifndef HALF_RESOLUTION
#define WIDTH 320
#define HEIGHT 240
#define BUFFER_SIZE 115200

#else
#define WIDTH 160
#define HEIGHT 120
#define BUFFER_SIZE 28800

#endif



#define expand_color(col, old, new) ((col & ((1 << old) - 1)) << (new - old))


void lcd_dma_handler(void)
{
    if(dma_irqn_get_channel_status(DMA_IRQ_2, LCD_DMA_DATA))
    {
        dma_irqn_acknowledge_channel(DMA_IRQ_2, LCD_DMA_DATA);
        if(raise_cs_on_irq)
        {
            gpio_put(LCD_CS, true);
            raise_cs_on_irq = false;
        }
    }
}

void lcd_init()
{
    // read that again
    /*
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
    irq_set_exclusive_handler(DMA_IRQ_2, lcd_dma_handler);
    irq_set_priority(DMA_IRQ_2, 0x40);
    */

    uint offset = pio_add_program_at_offset(LCD_PIO, &bl_pwm_program, 0);
    bl_pwm_program_init(LCD_PIO, LCD_BL_SM, offset, LCD_BL);
    pio_sm_set_enabled(LCD_PIO, LCD_BL_SM, false);
    pio_sm_put_blocking(LCD_PIO, LCD_BL_SM, 100);
    pio_sm_exec(LCD_PIO, LCD_BL_SM, pio_encode_pull(false, false));
    pio_sm_exec(LCD_PIO, LCD_BL_SM, pio_encode_out(pio_isr, 32));
    pio_sm_set_enabled(LCD_PIO, LCD_BL_SM, true);
    pio_sm_put_blocking(LCD_PIO, LCD_BL_SM, 34);


    uint tx_offset = pio_add_program_at_offset(LCD_PIO, &lcd_tx_double_program, 12);
    pio_sm_config tx_cfg = lcd_tx_double_program_get_default_config(tx_offset);
    sm_config_set_out_pins(&tx_cfg, LCD_SDI, 1);
    sm_config_set_sideset_pins(&tx_cfg, LCD_SCL);
    sm_config_set_clkdiv(&tx_cfg, 2.5f);
    pio_sm_set_consecutive_pindirs(LCD_PIO, LCD_RX_SM, LCD_SDI, 1, true);
    pio_sm_set_consecutive_pindirs(LCD_PIO, LCD_RX_SM, LCD_SCL, 1, true);
    pio_sm_init(LCD_PIO, LCD_TX_SM, tx_offset, &tx_cfg);

    gpio_set_function(LCD_BL, GPIO_FUNC_SIO);
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
    lcd_write_command(0x3A, (const uint8_t[]){0x53}, 1);    // Color mode - 12bit color, to boot
    wait_us(10 * 1000);
    lcd_write_command(0x36, (const uint8_t[]){0xB0}, 1);    // Memory data access control
    lcd_write_command(0xC6, (const uint8_t[]){0x1E}, 1);    // Frame Rate Control (40 fps)
    lcd_write_command(0x35, (const uint8_t[]){0x00}, 1);    // Tearing Effect Line On
    //lcd_write_command(0x21, NULL, 0);
    //wait_us(10 * 1000);
    lcd_write_command(0x13, NULL, 0);
    wait_us(10 * 1000);
    lcd_write_command(0x29, NULL, 0);
    wait_us(10 * 1000);
    
    lcd_set_mode(true);
    lcd_clear_screen();

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
    if(intensity > 99)
    {
        gpio_put(LCD_BL, true);
        gpio_set_function(LCD_BL, GPIO_FUNC_SIO);
        return;
    }

    pio_sm_put_blocking(LCD_PIO, LCD_BL_SM, intensity);
    gpio_set_function(LCD_BL, GPIO_FUNC_PIO0);
}


void lcd_set_mode(bool mode)
{
    lcd_mode = mode;
}

inline bool lcd_is_writting()
{
    return ((dma_hw->ch[LCD_DMA_DATA].al1_ctrl & DMA_CH0_CTRL_TRIG_BUSY_BITS) != 0);
}


/// TODO: add dma irq handler to raise CS pin upon completion of transmission
void lcd_clear_screen()
{
    gpio_set_function_masked((1 << LCD_SDI) | (1 << LCD_SCL), GPIO_FUNC_SPI);

    lcd_write_command(0x2A, (const uint8_t[]){0, 0, 1, 319 & 0xFF}, 4);    // Column address set
    lcd_write_command(0x2B, (const uint8_t[]){0, 0, 0, 239}, 4);           // Row address set

    gpio_put(LCD_CS, false);
    gpio_put(LCD_DC, false);

    spi_get_hw(LCD_SPI)->dr = 0x2C;
    while(!spi_is_readable(LCD_SPI))
        tight_loop_contents();
    (void)spi_get_hw(LCD_SPI)->dr;

    gpio_put(LCD_DC, true);

    uint32_t data = 0;

    dma_channel_config c = dma_channel_get_default_config(LCD_DMA_DATA);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_irq_quiet(&c, false);
    channel_config_set_dreq(&c, SPI_DREQ_NUM(LCD_SPI, true));
    channel_config_set_write_increment(&c, false);
    channel_config_set_read_increment(&c, false);

    raise_cs_on_irq = true;

    dma_channel_configure(
        LCD_DMA_DATA,
        &c,
        &spi_get_hw(LCD_SPI)->dr,
        &data,
        115200,
        true
    );
    

    while(dma_channel_is_busy(LCD_DMA_DATA))
        tight_loop_contents();
    
    gpio_put(LCD_CS, true);
    

}

void lcd_write_buffer(uint8_t *buff)
{
    gpio_set_function_masked((1 << LCD_SDI) | (1 << LCD_SCL), GPIO_FUNC_SPI);

    lcd_write_command(0x2A, (const uint8_t[]){0, 0, 1, 319 & 0xFF}, 4);    // Column address set
    lcd_write_command(0x2B, (const uint8_t[]){0, 0, 0, 239}, 4);           // Row address set

    gpio_put(LCD_CS, false);
    gpio_put(LCD_DC, false);

    spi_get_hw(LCD_SPI)->dr = 0x2C;
    while(!spi_is_readable(LCD_SPI))
        tight_loop_contents();
    (void)spi_get_hw(LCD_SPI)->dr;

    gpio_put(LCD_DC, true);

    dma_channel_config c = dma_channel_get_default_config(LCD_DMA_DATA);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_irq_quiet(&c, false);
    channel_config_set_dreq(&c, SPI_DREQ_NUM(LCD_SPI, true));
    channel_config_set_write_increment(&c, false);
    channel_config_set_read_increment(&c, true);

    raise_cs_on_irq = true;

    dma_channel_configure(
        LCD_DMA_DATA,
        &c,
        &spi_get_hw(LCD_SPI)->dr,
        buff,
        115200,
        true
    );
    

    while(dma_channel_is_busy(LCD_DMA_DATA))
        tight_loop_contents();
    
    gpio_put(LCD_CS, true);
}


void draw_to_buffer(uint8_t *buff, BITMAP *bmp, uint16_t x, uint16_t y)
{
    uint8_t red, green, blue, alpha;


    for(int i = y; i < y + bmp->image_height; i++)
    {
        for(int j = x; j < x + bmp->image_width; j++)
        {
            uint32_t off = ((i * WIDTH + j) * 3);
            uint8_t bit = off & 1;
            off = off >> 1;

            uint32_t color = bmp_get_pixel(bmp, j - x, i - y);

            red = expand_color(color, bmp->bitmask.red_bits, 4);
            color >>= bmp->bitmask.red_bits;
            green = expand_color(color, bmp->bitmask.green_bits, 4);
            color >>= bmp->bitmask.green_bits;
            blue = expand_color(color, bmp->bitmask.blue_bits, 4);
            color >>= bmp->bitmask.blue_bits;

            // to-do: fix this to be able to use more than 1 bit of alpha
            if(bmp->bitmask.alpha_bits)
                alpha = (uint8_t)(color != 0);
            else
                alpha = 1;

            if(alpha)
            {
                if(bit == 0)
                {
                    buff[off] = green | (red << 4);
                    buff[off + 1] = (buff[off + 1] & 0x0F) | (blue << 4);
                }
                else
                {
                    buff[off] = (buff[off] & 0xF0) | red;
                    buff[off + 1] = blue | (green << 4); 
                }
            }
        }
    }
}
