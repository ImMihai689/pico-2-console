#ifndef HARDWARE_H
#define HARDWARE_H

// not really sure why this is needed, it worked before without it
#include "pico/types.h"

#define HW_SD_MISO 0
#define HW_SD_MOSI 3
#define HW_SD_SCLK 2
#define HW_SD_CS 1
#define HW_SD_DET 4
// SD card interface pins
// DET is DETect


#define HW_I2C_INST &i2c0_inst

#define HW_I2C_FRAM_ADDR 0x50
#define HW_I2C_IO_ADDR 0x20
// Device addresses (FRAM IC and I/O extender IC)

#define BUT_A_I2C 5
#define BUT_B_I2C 6
#define BUT_X_I2C 2
#define BUT_Y_I2C 1
#define BUT_MENU_I2C 0
#define BUT_START_I2C 4
#define BUT_SHLD_R_I2C 3
#define BUT_SHLD_L_I2C 7
// What pins buttons A, B, X, Y, MENU, START, SHOULDER R and SHOULDER L
// are on the I2C I/O expander IC

#define SCL 9
#define SDA 8
// I2C pins for FRAM and I/O expander

#define LCD_SPI spi1
#define LCD_SCL 14
#define LCD_SDI 15
#define LCD_CS 13
#define LCD_DC 12
#define LCD_RST 11
#define LCD_FMARK 10
#define LCD_BL 6
#define LCD_BL_PIO pio0
#define LCD_BL_PIO_SM 0
// LCD interface pins
// SCL - Serial CLock, SDI - Serial Data In
// BL is backlight, it uses PIO PWM because the same channel and slice is also used by the buzzer

#define PICO_INFO_LED 16
#define PICO_INFO_LED_SLICE 0
#define PICO_INFO_LED_CHAN 0
// The blue color in the info LED on the board

#define HAPTIC 21
#define HAPTIC_SLICE 2
#define HAPTIC_CHAN 1
// The haptic/vibration motor pin (ACTIVE LOW)

#define BUZZER 22
#define BUZZER_SLICE 3
#define BUZZER_CHAN 0
#define BUZZER_ALARM 0
// The buzzer pin (ACTIVE LOW)(matters in idle, while using it it's PWM)
// And what system alarm to use to play notes on the buzzer

#define THUMB_SW 5
#define THUMB_X 27
#define THUMB_Y 26
// The thumbstick pins on the Pico, X and Y are anavoid log


typedef struct
{
    uint freq;  // Frequency to play
    uint us;    // For how long to play the note
} note_t;

/// @brief Fast wait function (fast as in, pretty small) (so I don't have to include pico/time.h)
/// @param us How many microseconds to wait
void wait_us(uint us);

/// @brief Initialize all of the hardware on the console.
///        Call before any other hardware function.
///        MUST be called on core1, this function doesn't return
void hw_init(void);


/// @brief Read the buttons on the console
/// @return All 8 button states encoded in each bit of the char. Each button's is noted in the BUT_[...]_I2C macros
char buttons_read(void);

// ToDo: MAKE FRAM CHIP WORK (used 5V chip)
/// @brief Write data to the console's FRAM
/// @param addr The first address to write the data to
/// @param data The data to write
/// @param len How many bytes to write
void fram_write(uint addr, const char *data, uint len);

// ToDo: MAKE FRAM CHIP WORK (used 5V chip)
/// @brief Read data from the console's FRAM
/// @param addr The first address from where to read the data
/// @param data The destination array, where the data will be written
/// @param len How many bytes to read
void fram_read(uint addr, char *data, uint len);

/// @brief Light up the info LED for a duration of time
/// @param val The LED brightness [0, 100]%
/// @param ms The amount of time to light up the LED (aprox.)
void info_led_set(uint val, uint ms);

/// @brief Play a single note
/// @param note Pointer to the note
void buzzer_play_note(const note_t *note);

/// @brief Play a series of notes
/// @param notes The array of notes to play (last note is termination and has duration 0)
void buzzer_play_notes(const note_t *notes);

/// @brief Read the thumbstick X axis
/// @return A number from 0 to 4095 (14 bit resolution), 0 - full left, 4095 - full right
uint thumb_read_x(void);

/// @brief Read the thumbstick Y axis
/// @return A number from 0 to 4095 (14 bit resolution), 0 - full down, 4095 - full up
uint thumb_read_y(void);

/// @brief Read the thumbstick switch (button)
/// @return True if pressed, false if not
bool thumb_read_sw(void);

/// @brief Set the brighness of the LCD backlight
/// @param intensity any value in [0, 32], where 0 is off, and 32 is full on
void lcd_set_backlight(int intensity);

/// @brief Set the writting rect of the LCD to the rectangle of upper left corner at (x0, y0) and lower right corner at (x1, y1).
///        0 <= x0 <= x1 < 320 and 0 <= y0 <= y1 < 240. Coordinates will be flipped/truncated if they aren't corect.
/// @param x0 upper left x coordinate
/// @param y0 upper left y coordinate
/// @param x1 lower right x coordinate
/// @param y1 lower right y coordinate
void lcd_set_write_rect(uint x0, uint y0, uint x1, uint y1);

/// @brief Set the LCD's color mode (slow function, takes 10ms) (by default 565-bit colors)
/// @param mode If false, 12-444-bit colors (4k colors). If true, 16-565-bit colors(65k colors)
void lcd_set_color_mode(bool mode);

#endif