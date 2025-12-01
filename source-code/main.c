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
#include "bmplib.h"

#include "hardware.h"
#include "display.h"


/*__attribute__((section(".console_vtable")))
void (* const function_table[])(void) = {
    hw_init,
    (void *)lcd_set_backlight
};*/

FATFS FatFs;
FIL fil;
FRESULT fr;
BITMAP bmp;

DWORD data[1536];

uint8_t buffer[115200];

int main()
{
    stdio_init_all();
    hw_init();

    sleep_ms(3000);

    lcd_set_backlight(100);

    f_mount(&FatFs, "", 0);

    fr = f_open(&fil, "BMPTEST2.BMP", FA_READ || FA_OPEN_EXISTING);
    if(fr)
    {
        printf("opening: %d\n", fr);
        return 0;
    }

    bmp.pixel_data = data;
    bmp.bitmask.red_bits = 4;
    bmp.bitmask.green_bits = 4;
    bmp.bitmask.blue_bits = 4;
    bmp.bitmask.alpha_bits = 0;

    fr = bmp_read(&bmp, &fil);

    printf("read file: %d\n", fr);

    draw_to_buffer(buffer, &bmp, 80, 80);

    lcd_write_buffer(buffer);


    while (true) {
        sleep_ms(100);
    }
}
