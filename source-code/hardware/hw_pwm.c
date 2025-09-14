#include "hw_pwm.h"

void hw_pwm_init()
{
    gpio_set_function(PICO_INFO_LED, GPIO_FUNC_PWM);
    gpio_set_function(HAPTIC, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);


    pwm_set_clkdiv(0, 200.0f);
    pwm_set_wrap(0, 100);
    pwm_set_chan_level(0, 0, 101);
    pwm_set_enabled(0, true);

    pwm_set_wrap(2, 10);
    pwm_set_chan_level(2, 1, 11);
    pwm_set_enabled(2, true);

    pwm_set_wrap(3, 10);
    pwm_set_chan_level(3, 0, 11);
    pwm_set_enabled(3, true);

    //timer_hardware_alarm_claim(timer0_hw, BUZZER_ALARM);
}

uint info_led_last_timeout = 0;

void hw_pwm_set_info_led(uint val, uint ms)
{
    if(ms == 0)
        if(time_us_32() >= info_led_last_timeout)
            pwm_set_chan_level(PICO_INFO_LED_SLICE_CHAN >> 1, PICO_INFO_LED_SLICE_CHAN & 1, 101);
    else
    {
        if(val > 100) val = 100;
        pwm_set_chan_level(PICO_INFO_LED_SLICE_CHAN >> 1, PICO_INFO_LED_SLICE_CHAN & 1, 100 - val);
        info_led_last_timeout = time_us_32() + (ms * 1000);
    }
}

const hw_pwm_note_t *next_note;
void note_alarm_callback(uint alarm_num);

void play_note_internal(const hw_pwm_note_t *note)
{
    timer_hardware_alarm_set_target(timer0_hw, BUZZER_ALARM, (time_us_32() + note->us));
    timer_hardware_alarm_set_callback(timer0_hw, BUZZER_ALARM, note_alarm_callback);
    
    uint sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint ref = sys_clk / (note->freq * 0xFFFF);
    if(ref == 0) ref = 1;
    uint wrap = sys_clk / (ref * note->freq);

    pwm_set_clkdiv_int_frac4(BUZZER_SLICE_CHAN >> 1, ref, 0);
    pwm_set_wrap(BUZZER_SLICE_CHAN >> 1, wrap);
    pwm_set_chan_level(BUZZER_SLICE_CHAN >> 1, BUZZER_SLICE_CHAN & 1, wrap >> 1);
}

void note_alarm_callback(uint alarm_num)
{
    if(next_note == NULL)
    {
        pwm_set_wrap(BUZZER_SLICE_CHAN >> 1, 100);
        pwm_set_chan_level(BUZZER_SLICE_CHAN >> 1, BUZZER_SLICE_CHAN & 1, 101);
        return;
    }
    if(next_note->us == 0)
    {
        pwm_set_wrap(BUZZER_SLICE_CHAN >> 1, 100);
        pwm_set_chan_level(BUZZER_SLICE_CHAN >> 1, BUZZER_SLICE_CHAN & 1, 101);
        return;
    }
    else
    {
        play_note_internal(next_note++);
    }
}

void hw_pwm_play_note(const hw_pwm_note_t *note)
{
    play_note_internal(note);
    next_note = NULL;
}

void hw_pwm_play_notes(const hw_pwm_note_t *notes)
{
    play_note_internal(notes);
    next_note = notes + 1;
}

