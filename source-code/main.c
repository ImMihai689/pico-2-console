#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "ff.h"

#include "hw_main.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

// Data will be copied from src to dst
const char src[] = "Hello, world! (from DMA)";
char dst[count_of(src)];

#include "blink.pio.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}



int main()
{
    stdio_init_all();

    hw_init();
    printf("test 1\n");

    sleep_ms(4000);
    printf("test 2\n");

    

    hw_i2c_init();
    printf("test 3\n");

    uint8_t tst[] = {0, 0};
    int res = i2c_write_blocking(HW_I2C_INST, HW_I2C_FRAM_ADDR, tst, 2, false);
    printf("attemt comm fram: %d\n", res);

    while (true) {
        printf("test 4\n");
        char dest = 0xFF;
        if(i2c_write_blocking(HW_I2C_INST, HW_I2C_IO_ADDR, &dest, 1, false))
            printf("write failed\n");
        printf("test 5\n");
        if(i2c_read_blocking(HW_I2C_INST, HW_I2C_IO_ADDR, &dest, 1, false))
            printf("read failed\n");
        printf("buttons: %d\n", dest);
        sleep_ms(1000);
    }
}
