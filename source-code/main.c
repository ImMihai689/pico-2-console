#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "ff.h"

#include "hw_main.h"

const hw_pwm_note_t test_notes[] = {
    {.freq = 262, .us = 50000},
    {.freq = 294, .us = 50000},
    {.freq = 330, .us = 50000},
    {.freq = 349, .us = 50000},
    {.freq = 392, .us = 50000},
    {.freq = 440, .us = 50000},
    {.freq = 494, .us = 50000},
    {.freq = 523, .us = 50000},
    {.freq = 0, .us = 0},
};

int main()
{
    stdio_init_all();

    hw_init();

    

    while (true) {
        //hw_pwm_play_notes(test_notes);
        sleep_ms(5000);
    }
}
