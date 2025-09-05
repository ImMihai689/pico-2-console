#ifndef HW_DEF
#define HW_DEF

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

#define THUMB_SW 5
#define THUMB_X 27
#define THUMB_Y 26
// The thumbstick pins on the Pico, X and Y are analog

#define PICO_INFO_LED 16
// The blue color in the info LED on the board

#define HAPTIC 21
// The haptic/vibration motor pin (ACTIVE LOW)

#define BUZZER 22
// The buzzer pin (ACTIVE LOW)(matters in idle, while using it it's PWM)

#define HW_SD_MISO 0
#define HW_SD_MOSI 3
#define HW_SD_SCLK 2
#define HW_SD_CS 1
#define HW_SD_DET 4
// SD card interface pins
// DET is DETect

#define LCD_SCL 14
#define LCD_SDI 15
#define LCD_CS 13
#define LCD_DC 12
#define LCD_RST 11
#define LCD_FMARK 10
#define LCD_BL 6
// LCD interface pins
// SCL - Serial CLock, SDI - Serial Data In
// BL is backlight

#define SCL 9
#define SDA 8
// I2C pins for FRAM and I/O expander

#endif