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
// Will be given by the user in the right size
uint8_t *display_buffer;

bool raise_cs_on_irq = false;

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
    

    uint offset = pio_can_add_program_at_offset(LCD_PIO, &bl_pwm_program, 0);
    pio_sm_config bl_cfg = bl_pwm_program_get_default_config(offset);
    sm_config_set_clkdiv(&bl_cfg, 2500.0f);
    sm_config_set_sideset_pin_base(&bl_cfg, LCD_BL);
    pio_sm_init(LCD_PIO, LCD_BL_SM, offset, &bl_cfg);
    pio_sm_set_consecutive_pindirs(LCD_PIO, LCD_BL_SM, LCD_BL, 1, true);
    pio_sm_put_blocking(LCD_PIO, LCD_BL_SM, 255);   // the PWM period
    pio_sm_exec(LCD_PIO, LCD_BL_SM, pio_encode_pull(false, false));
    pio_sm_exec(LCD_PIO, LCD_BL_SM, pio_encode_out(pio_isr, 32));
    pio_sm_set_enabled(LCD_PIO, LCD_BL_SM, true);

    /// test this
    LCD_PIO->input_sync_bypass |= (1 << LCD_SDI | 1 << LCD_SCL);
    uint rx_offset = pio_add_program_at_offset(LCD_PIO, &lcd_rx_program, 7);
    pio_sm_config rx_cfg = lcd_rx_program_get_default_config(rx_offset);
    sm_config_set_clkdiv_int_frac(&rx_cfg, 14, 0);
    sm_config_set_in_pin_base(&rx_cfg, LCD_SDI);
    sm_config_set_sideset_pin_base(&rx_cfg, LCD_SCL);
    pio_sm_init(LCD_PIO, LCD_RX_SM, rx_offset, &rx_cfg);
    // the pindirs are set by the TX state machine, since mainly that is used
    //pio_sm_set_consecutive_pindirs(LCD_PIO, LCD_RX_SM, LCD_SDI, 1, false);
    //pio_sm_set_consecutive_pindirs(LCD_PIO, LCD_RX_SM, LCD_SCL, 1, true);
    // not really sure that this SM is gonna be used
    //pio_sm_set_enabled(LCD_PIO, LCD_RX_SM, true);

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
    lcd_write_command(0x3A, (const uint8_t[]){0x53}, 1);    // Color mode - 16bit color, to boot
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
    if(intensity > 255)
    {
        gpio_put(LCD_BL, true);
        gpio_set_function(LCD_BL, GPIO_FUNC_SIO);
        return;
    }

    pio_sm_put_blocking(LCD_PIO, LCD_BL_SM, intensity);
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

void lcd_set_mode(bool mode)
{
    lcd_mode = mode;
    if(mode)
    {
        gpio_set_function_masked(1 << LCD_SDI | 1 << LCD_SCL, GPIO_FUNC_SPI);
        lcd_write_command(0x3A, (const uint8_t[]){0x53}, 1);    // Color mode - 12bit color
        wait_us(10 * 1000);
    }
    else
    {
        gpio_set_function_masked(1 << LCD_SDI | 1 << LCD_SCL, GPIO_FUNC_SPI);
        lcd_write_command(0x3A, (const uint8_t[]){0x55}, 1);    // Color mode - 16bit color
        wait_us(10 * 1000);
        gpio_set_function_masked(1 << LCD_SDI | 1 << LCD_SCL, PIO_FUNCSEL_NUM(LCD_PIO, LCD_SCL));
    }
}

inline bool lcd_is_writting()
{
    return dma_hw->ch[LCD_DMA_DATA].al1_ctrl & DMA_CH0_CTRL_TRIG_BUSY_BITS;
}


/// TODO: add dma irq handler to raise CS pin upon completion of transmission
void lcd_clear_screen()
{
    gpio_set_function_masked(1 << LCD_SDI | 1 << LCD_SCL, GPIO_FUNC_SPI);

    lcd_write_command(0x2A, (const uint8_t[]){0, 0, 1, 319 & 0xFF}, 4);    // Column address set
    lcd_write_command(0x2B, (const uint8_t[]){0, 0, 0, 239}, 4);    // Row address set

    gpio_put(LCD_CS, false);
    gpio_put(LCD_DC, false);

    spi_get_hw(LCD_SPI)->dr = 0x2C;
    while(!spi_is_readable(LCD_SPI))
        tight_loop_contents();
    (void)spi_get_hw(LCD_SPI)->dr;

    spi_get_hw(LCD_SPI)->cr0 |= SPI_SSPCR0_DSS_BITS & (0x0f << SPI_SSPCR0_DSS_LSB);

    gpio_put(LCD_DC, true);

    uint32_t data = 0;

    dma_channel_config c = dma_channel_get_default_config(LCD_DMA_DATA);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_irq_quiet(&c, false);
    channel_config_set_dreq(&c, SPI_DREQ_NUM(LCD_SPI, true));
    channel_config_set_write_increment(&c, false);
    channel_config_set_read_increment(&c, false);

    if(lcd_mode)
    {
        raise_cs_on_irq = true;
        dma_channel_configure(
            LCD_DMA_DATA,
            &c,
            &spi_get_hw(LCD_SPI)->dr,
            &data,
            57600,
            true
        );
    }
    else
    {
        raise_cs_on_irq = true;
        dma_channel_configure(
            LCD_DMA_DATA,
            &c,
            &spi_get_hw(LCD_SPI)->dr,
            &data,
            76800,
            true
        );
    }

    while(dma_channel_is_busy(LCD_DMA_DATA))
        tight_loop_contents();
    
    spi_get_hw(LCD_SPI)->cr0 |= SPI_SSPCR0_DSS_BITS & (0x07 << SPI_SSPCR0_DSS_LSB);

    if(lcd_mode)
        gpio_set_function_masked(1 << LCD_SDI | 1 << LCD_SCL, GPIO_FUNC_SPI);
    else
        gpio_set_function_masked(1 << LCD_SDI | 1 << LCD_SCL, PIO_FUNCSEL_NUM(LCD_PIO, LCD_SCL));

}

