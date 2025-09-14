#ifndef HW_PWM
#define HW_PWM

#define PICO_INFO_LED 16
#define PICO_INFO_LED_SLICE_CHAN 0
// The blue color in the info LED on the board

#define HAPTIC 21
#define HAPTIC_SLICE_CHAN 5
// The haptic/vibration motor pin (ACTIVE LOW)

#define BUZZER 22
#define BUZZER_SLICE_CHAN 6
#define BUZZER_ALARM 3
// The buzzer pin (ACTIVE LOW)(matters in idle, while using it it's PWM)
// And what system alarm to use to play notes on the buzzer

#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"


typedef struct
{
    uint freq;  // Frequency to play
    uint us;    // For how long to play the note
} hw_pwm_note_t;


void hw_pwm_init();

/// @brief Light up the info LED for a duration of time
/// @param val The LED brightness [0, 100]%
/// @param ms The amount of time to light up the LED (aprox.)
void hw_pwm_set_info_led(uint val, uint ms);

/// @brief Play a single note
/// @param note Pointer to the note
void hw_pwm_play_note(const hw_pwm_note_t *note);

/// @brief Play a series of notes
/// @param notes The array of notes to play (last note is termination and has duration 0)
void hw_pwm_play_notes(const hw_pwm_note_t *notes);

#endif