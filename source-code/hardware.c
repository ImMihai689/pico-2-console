#include "hardware.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pio.pio.h"
#include "pico/multicore.h"

/// Raw read of lower 32 bits of default timer
#define TIME_US PICO_DEFAULT_TIMER_INSTANCE()->timerawl


void wait_us(uint us)
{
    uint time = TIME_US + us;
    while(PICO_DEFAULT_TIMER_INSTANCE()->timerawl <= time)
        tight_loop_contents();
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

uint info_led_target_off = 0;

void info_led_update_private()
{
    if(info_led_target_off == 0)
        return;
    uint time = TIME_US;
    if(time >= info_led_target_off)
    {
        pwm_set_chan_level(PICO_INFO_LED_SLICE, PICO_INFO_LED_CHAN, 101);
    }
}

void hw_init()
{
    /// PICO INFO LED, HAPTIC and BUZZER (all with PWM)

    gpio_set_function(PICO_INFO_LED, GPIO_FUNC_PWM);
    gpio_set_function(HAPTIC, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);

    pwm_set_clkdiv(PICO_INFO_LED_SLICE, 200.0f);
    pwm_set_wrap(PICO_INFO_LED_SLICE, 100);
    pwm_set_chan_level(PICO_INFO_LED_SLICE, PICO_INFO_LED_CHAN, 101);
    pwm_set_enabled(PICO_INFO_LED_SLICE, true);

    pwm_set_wrap(HAPTIC_SLICE, 10);
    pwm_set_chan_level(HAPTIC_SLICE, HAPTIC_CHAN, 11);
    pwm_set_enabled(HAPTIC_SLICE, true);

    pwm_set_wrap(BUZZER_SLICE, 10);
    pwm_set_chan_level(BUZZER_SLICE, BUZZER_CHAN, 11);
    pwm_set_enabled(BUZZER_SLICE, true);

    /// THUMBSTICK -- will have to resolder connector

    gpio_init(THUMB_SW);
    gpio_set_pulls(THUMB_SW, true, false);
    gpio_set_dir(THUMB_SW, false);

    adc_init();
    adc_gpio_init(THUMB_X);
    adc_gpio_init(THUMB_Y);

    /// BUTTONS and FRAM (all with I2C)

    i2c_init(HW_I2C_INST, 100 * 1000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_set_pulls(SDA, true, false);
    gpio_set_pulls(SCL, true, false);

    /// LCD

    uint offset = pio_add_program(LCD_BL_PIO, &bl_pwm_program);
    pio_sm_config bl_cfg = bl_pwm_program_get_default_config(offset);
    sm_config_set_clkdiv(&bl_cfg, 2500.0f);
    sm_config_set_sideset_pin_base(&bl_cfg, LCD_BL);
    pio_sm_init(LCD_BL_PIO, LCD_BL_PIO_SM, offset, &bl_cfg);
    pio_sm_set_consecutive_pindirs(LCD_BL_PIO, LCD_BL_PIO_SM, LCD_BL, 1, true);
    pio_sm_set_enabled(LCD_BL_PIO, LCD_BL_PIO_SM, true);

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
    
    spi_init(LCD_SPI, 1 * 1000 * 1000);

    wait_us(1000);
    lcd_write_command(0x01, NULL, 0);   // Soft reset, no args
    wait_us(150 * 1000);
    lcd_write_command(0x11, NULL, 0);   // Sleep ou, no args
    wait_us(10 * 1000);
    lcd_write_command(0x3A, (const uint8_t[]){0x55}, 1);    // Color mode - 16bit color, to boot
    wait_us(10 * 1000);
    lcd_write_command(0x36, (const uint8_t[]){0xB0}, 1);    // Memory data access control
    lcd_write_command(0x2A, (const uint8_t[]){0, 0, 1, 320 & 0xFF}, 4);    // Column address set
    lcd_write_command(0x2B, (const uint8_t[]){0, 0, 0, 240}, 4);    // Row address set
    //lcd_write_command(0x21, NULL, 0);
    //wait_us(10 * 1000);
    lcd_write_command(0x13, NULL, 0);
    wait_us(10 * 1000);
    lcd_write_command(0x29, NULL, 0);
    wait_us(10 * 1000);
    
    uint16_t pixel = 0;// 0xFFFF;
    uint8_t ramwr = 0x2C;

    gpio_put(LCD_CS, false);
    gpio_put(LCD_DC, false);
    spi_write_blocking(LCD_SPI, &ramwr, 1);
    gpio_put(LCD_DC, true);
    for(int i = 0; i < 240 * 320; i++)
    {
        spi_write_blocking(LCD_SPI, (uint8_t *)&pixel, 2);
    }
    gpio_put(LCD_CS, true);

    gpio_put(LCD_BL, false);
    gpio_set_function(LCD_BL, GPIO_FUNC_SIO);

    multicore_fifo_push_blocking('k');

    while(true)
    {
        info_led_update_private();
    }
}

void info_led_set(uint val, uint ms)
{
    // ToDo: make this work
    if(val > 100) val = 100;
    pwm_set_chan_level(PICO_INFO_LED_SLICE, PICO_INFO_LED_CHAN, 100 - val);
    info_led_target_off = TIME_US + (ms * 1000);
}

const note_t *next_note;
void note_alarm_callback(uint alarm_num);

void buzzer_play_note_private(const note_t *note)
{
    timer_hardware_alarm_set_target(timer0_hw, BUZZER_ALARM, (time_us_32() + note->us));
    timer_hardware_alarm_set_callback(timer0_hw, BUZZER_ALARM, note_alarm_callback);
    
    uint sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint ref = sys_clk / (note->freq * 0xFFFF);
    if(ref == 0) ref = 1;
    uint wrap = sys_clk / (ref * note->freq);

    pwm_set_clkdiv_int_frac4(BUZZER_SLICE, ref, 0);
    pwm_set_wrap(BUZZER_SLICE, wrap);
    pwm_set_chan_level(BUZZER_SLICE, BUZZER_CHAN, wrap >> 1);
}

void note_alarm_callback(uint alarm_num)
{
    if(next_note == NULL)
    {
        pwm_set_wrap(BUZZER_SLICE, 100);
        pwm_set_chan_level(BUZZER_SLICE, BUZZER_CHAN, 101);
        return;
    }
    if(next_note->us == 0)
    {
        pwm_set_wrap(BUZZER_SLICE, 100);
        pwm_set_chan_level(BUZZER_SLICE, BUZZER_CHAN, 101);
        return;
    }
    else
    {
        buzzer_play_note_private(next_note++);
    }
}

void buzzer_play_note(const note_t *note)
{
    buzzer_play_note_private(note);
    next_note = NULL;
}

void buzzer_play_notes(const note_t *notes)
{
    buzzer_play_note_private(notes);
    next_note = notes + 1;
}

char buttons_read()
{
    char fin = 0;
    i2c_read_blocking(HW_I2C_INST, HW_I2C_IO_ADDR, &fin, 1, false);
    return fin;
}



uint thumb_read_x()
{
    adc_select_input(1); // THUMB_X 27 is ADC channel 1
    return -(adc_read() >> 2);
}

uint thumb_read_y()
{
    adc_select_input(0); // THUMB_Y 26 is ADC channel 0
    return (adc_read() >> 2);
}

bool thumb_read_sw()
{
    return !gpio_get(THUMB_SW);
}