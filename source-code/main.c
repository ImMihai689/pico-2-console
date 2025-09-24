#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"

#include "ff.h"

#include "hardware.h"
#include "display.h"

int main()
{
    stdio_init_all();

    sleep_ms(3000);

    //set_sys_clock_khz(DESIGN_SYS_FREQ_KHZ, true);
    printf("clock: %d\n", frequency_count_khz(0x09));

    printf("tilemap str size: %d\n", sizeof(tileset_t));

    sizeof(tilemap_t);

    multicore_launch_core1(hw_init);
    uint32_t result;
    if(!multicore_fifo_pop_timeout_us(5 * 1000 * 1000, &result))
        info_led_set(100, 10 * 1000);
    if(result != 'k')
        info_led_set(100, 10 * 1000);
    
    lcd_set_backlight(16);

    
    
    

    while (true) {
        //printf("Sw: %d,  X: %d,  Y: %d\n", thumb_read_sw(), thumb_read_x(), thumb_read_y());
        sleep_ms(100);
    }
}
