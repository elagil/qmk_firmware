#include "quantum.h"
#include "backlight.h"
#include "backlight_driver_common.h"
#include "debug.h"

#ifdef SPLIT_KEYBOARD
#    include "split_util.h"
#endif

// This logic is a bit complex, we support 3 setups:
//
//   1. Hardware PWM when backlight is wired to a PWM pin.
//      Depending on this pin, we use a different output compare unit.
//   2. Software PWM with hardware timers, but the used timer
//      depends on the Audio setup (Audio wins over Backlight).
//   3. Full software PWM, driven by the matrix scan, if both timers are used by Audio.

#if (defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB647__) || defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB1287__) || defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__)) && (BACKLIGHT_PIN == B5 || BACKLIGHT_PIN == B6 || BACKLIGHT_PIN == B7)
#    define HARDWARE_PWM
#    define ICRx ICR1
#    define TCCRxA TCCR1A
#    define TCCRxB TCCR1B
#    define TIMERx_OVF_vect TIMER1_OVF_vect
#    define TIMSKx TIMSK1
#    define TOIEx TOIE1

#    if BACKLIGHT_PIN == B5
#        define COMxx0 COM1A0
#        define COMxx1 COM1A1
#        define OCRxx OCR1A
#    elif BACKLIGHT_PIN == B6
#        define COMxx0 COM1B0
#        define COMxx1 COM1B1
#        define OCRxx OCR1B
#    elif BACKLIGHT_PIN == B7
#        define COMxx0 COM1C0
#        define COMxx1 COM1C1
#        define OCRxx OCR1C
#    endif
#elif (defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB647__) || defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB1287__) || defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__)) && (BACKLIGHT_PIN == C4 || BACKLIGHT_PIN == C5 || BACKLIGHT_PIN == C6)
#    define HARDWARE_PWM
#    define ICRx ICR3
#    define TCCRxA TCCR3A
#    define TCCRxB TCCR3B
#    define TIMERx_OVF_vect TIMER3_OVF_vect
#    define TIMSKx TIMSK3
#    define TOIEx TOIE3

#    if BACKLIGHT_PIN == C4
#        if (defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__))
#            error This MCU has no C4 pin!
#        else
#            define COMxx0 COM3C0
#            define COMxx1 COM3C1
#            define OCRxx OCR3C
#        endif
#    elif BACKLIGHT_PIN == C5
#        if (defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__))
#            error This MCU has no C5 pin!
#        else
#            define COMxx0 COM3B0
#            define COMxx1 COM3B1
#            define OCRxx OCR3B
#        endif
#    elif BACKLIGHT_PIN == C6
#        define COMxx0 COM3A0
#        define COMxx1 COM3A1
#        define OCRxx OCR3A
#    endif
#elif (defined(__AVR_ATmega16U2__) || defined(__AVR_ATmega32U2__)) && (BACKLIGHT_PIN == B7 || BACKLIGHT_PIN == C5 || BACKLIGHT_PIN == C6)
#    define HARDWARE_PWM
#    define ICRx ICR1
#    define TCCRxA TCCR1A
#    define TCCRxB TCCR1B
#    define TIMERx_OVF_vect TIMER1_OVF_vect
#    define TIMSKx TIMSK1
#    define TOIEx TOIE1

#    if BACKLIGHT_PIN == B7
#        define COMxx0 COM1C0
#        define COMxx1 COM1C1
#        define OCRxx OCR1C
#    elif BACKLIGHT_PIN == C5
#        define COMxx0 COM1B0
#        define COMxx1 COM1B1
#        define OCRxx OCR1B
#    elif BACKLIGHT_PIN == C6
#        define COMxx0 COM1A0
#        define COMxx1 COM1A1
#        define OCRxx OCR1A
#    endif
#elif defined(__AVR_ATmega32A__) && (BACKLIGHT_PIN == D4 || BACKLIGHT_PIN == D5)
#    define HARDWARE_PWM
#    define ICRx ICR1
#    define TCCRxA TCCR1A
#    define TCCRxB TCCR1B
#    define TIMERx_OVF_vect TIMER1_OVF_vect
#    define TIMSKx TIMSK
#    define TOIEx TOIE1

#    if BACKLIGHT_PIN == D4
#        define COMxx0 COM1B0
#        define COMxx1 COM1B1
#        define OCRxx OCR1B
#    elif BACKLIGHT_PIN == D5
#        define COMxx0 COM1A0
#        define COMxx1 COM1A1
#        define OCRxx OCR1A
#    endif
#elif defined(__AVR_ATmega328P__) && (BACKLIGHT_PIN == B1 || BACKLIGHT_PIN == B2)
#    define HARDWARE_PWM
#    define ICRx ICR1
#    define TCCRxA TCCR1A
#    define TCCRxB TCCR1B
#    define TIMERx_OVF_vect TIMER1_OVF_vect
#    define TIMSKx TIMSK1
#    define TOIEx TOIE1

#    if BACKLIGHT_PIN == B1
#        define COMxx0 COM1A0
#        define COMxx1 COM1A1
#        define OCRxx OCR1A
#    elif BACKLIGHT_PIN == B2
#        define COMxx0 COM1B0
#        define COMxx1 COM1B1
#        define OCRxx OCR1B
#    endif
#elif !defined(B5_AUDIO) && !defined(B6_AUDIO) && !defined(B7_AUDIO)
// Timer 1 is not in use by Audio feature, Backlight can use it
#    pragma message "Using hardware timer 1 with software PWM"
#    define HARDWARE_PWM
#    define BACKLIGHT_PWM_TIMER
#    define ICRx ICR1
#    define TCCRxA TCCR1A
#    define TCCRxB TCCR1B
#    define TIMERx_COMPA_vect TIMER1_COMPA_vect
#    define TIMERx_OVF_vect TIMER1_OVF_vect
#    if defined(__AVR_ATmega32A__)  // This MCU has only one TIMSK register
#        define TIMSKx TIMSK
#    else
#        define TIMSKx TIMSK1
#    endif
#    define TOIEx TOIE1

#    define OCIExA OCIE1A
#    define OCRxx OCR1A
#elif !defined(C6_AUDIO) && !defined(C5_AUDIO) && !defined(C4_AUDIO)
#    pragma message "Using hardware timer 3 with software PWM"
// Timer 3 is not in use by Audio feature, Backlight can use it
#    define HARDWARE_PWM
#    define BACKLIGHT_PWM_TIMER
#    define ICRx ICR1
#    define TCCRxA TCCR3A
#    define TCCRxB TCCR3B
#    define TIMERx_COMPA_vect TIMER3_COMPA_vect
#    define TIMERx_OVF_vect TIMER3_OVF_vect
#    define TIMSKx TIMSK3
#    define TOIEx TOIE3

#    define OCIExA OCIE3A
#    define OCRxx OCR3A
#elif defined(BACKLIGHT_CUSTOM_DRIVER)
error("Please set 'BACKLIGHT_DRIVER = custom' within rules.mk")
#else
error("Please set 'BACKLIGHT_DRIVER = software' within rules.mk")
#endif

#ifdef SPLIT_KEYBOARD
#if (defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB647__) || defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB1287__) || defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__)) && (BACKLIGHT_PIN_RIGHT == B5 || BACKLIGHT_PIN_RIGHT == B6 || BACKLIGHT_PIN_RIGHT == B7)
#    define HARDWARE_PWM_RIGHT
#    define ICRx_RIGHT ICR1
#    define TCCRxA_RIGHT TCCR1A
#    define TCCRxB_RIGHT TCCR1B
#    define TIMERx_OVF_vect_RIGHT TIMER1_OVF_vect
#    define TIMSKx_RIGHT TIMSK1
#    define TOIEx_RIGHT TOIE1

#    if BACKLIGHT_PIN_RIGHT == B5
#        define COMxx0_RIGHT COM1A0
#        define COMxx1_RIGHT COM1A1
#        define OCRxx_RIGHT OCR1A
#    elif BACKLIGHT_PIN_RIGHT == B6
#        define COMxx0_RIGHT COM1B0
#        define COMxx1_RIGHT COM1B1
#        define OCRxx_RIGHT OCR1B
#    elif BACKLIGHT_PIN_RIGHT == B7
#        define COMxx0_RIGHT COM1C0
#        define COMxx1_RIGHT COM1C1
#        define OCRxx_RIGHT OCR1C
#    endif
#elif (defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB647__) || defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB1287__) || defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__)) && (BACKLIGHT_PIN_RIGHT == C4 || BACKLIGHT_PIN_RIGHT == C5 || BACKLIGHT_PIN_RIGHT == C6)
#    define HARDWARE_PWM_RIGHT
#    define ICRx_RIGHT ICR3
#    define TCCRxA_RIGHT TCCR3A
#    define TCCRxB_RIGHT TCCR3B
#    define TIMERx_OVF_vect_RIGHT TIMER3_OVF_vect
#    define TIMSKx_RIGHT TIMSK3
#    define TOIEx_RIGHT TOIE3

#    if BACKLIGHT_PIN_RIGHT == C4
#        if (defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__))
#            error This MCU has no C4 pin!
#        else
#            define COMxx0_RIGHT COM3C0
#            define COMxx1_RIGHT COM3C1
#            define OCRxx_RIGHT OCR3C
#        endif
#    elif BACKLIGHT_PIN_RIGHT == C5
#        if (defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__))
#            error This MCU has no C5 pin!
#        else
#            define COMxx0_RIGHT COM3B0
#            define COMxx1_RIGHT COM3B1
#            define OCRxx_RIGHT OCR3B
#        endif
#    elif BACKLIGHT_PIN_RIGHT == C6
#        define COMxx0_RIGHT COM3A0
#        define COMxx1_RIGHT COM3A1
#        define OCRxx_RIGHT OCR3A
#    endif
#elif (defined(__AVR_ATmega16U2__) || defined(__AVR_ATmega32U2__)) && (BACKLIGHT_PIN_RIGHT == B7 || BACKLIGHT_PIN_RIGHT == C5 || BACKLIGHT_PIN_RIGHT == C6)
#    define HARDWARE_PWM_RIGHT
#    define ICRx_RIGHT ICR1
#    define TCCRxA_RIGHT TCCR1A
#    define TCCRxB_RIGHT TCCR1B
#    define TIMERx_OVF_vect_RIGHT TIMER1_OVF_vect
#    define TIMSKx_RIGHT TIMSK1
#    define TOIEx_RIGHT TOIE1

#    if BACKLIGHT_PIN_RIGHT == B7
#        define COMxx0_RIGHT COM1C0
#        define COMxx1_RIGHT COM1C1
#        define OCRxx_RIGHT OCR1C
#    elif BACKLIGHT_PIN_RIGHT == C5
#        define COMxx0_RIGHT COM1B0
#        define COMxx1_RIGHT COM1B1
#        define OCRxx_RIGHT OCR1B
#    elif BACKLIGHT_PIN_RIGHT == C6
#        define COMxx0_RIGHT COM1A0
#        define COMxx1_RIGHT COM1A1
#        define OCRxx_RIGHT OCR1A
#    endif
#elif defined(__AVR_ATmega32A__) && (BACKLIGHT_PIN_RIGHT == D4 || BACKLIGHT_PIN_RIGHT == D5)
#    define HARDWARE_PWM_RIGHT
#    define ICRx_RIGHT ICR1
#    define TCCRxA_RIGHT TCCR1A
#    define TCCRxB_RIGHT TCCR1B
#    define TIMERx_OVF_vect_RIGHT TIMER1_OVF_vect
#    define TIMSKx_RIGHT TIMSK
#    define TOIEx_RIGHT TOIE1

#    if BACKLIGHT_PIN_RIGHT == D4
#        define COMxx0_RIGHT COM1B0
#        define COMxx1_RIGHT COM1B1
#        define OCRxx_RIGHT OCR1B
#    elif BACKLIGHT_PIN_RIGHT == D5
#        define COMxx0_RIGHT COM1A0
#        define COMxx1_RIGHT COM1A1
#        define OCRxx_RIGHT OCR1A
#    endif
#elif defined(__AVR_ATmega328P__) && (BACKLIGHT_PIN_RIGHT == B1 || BACKLIGHT_PIN_RIGHT == B2)
#    define HARDWARE_PWM_RIGHT
#    define ICRx_RIGHT ICR1
#    define TCCRxA_RIGHT TCCR1A
#    define TCCRxB_RIGHT TCCR1B
#    define TIMERx_OVF_vect_RIGHT TIMER1_OVF_vect
#    define TIMSKx_RIGHT TIMSK1
#    define TOIEx_RIGHT TOIE1

#    if BACKLIGHT_PIN_RIGHT == B1
#        define COMxx0_RIGHT COM1A0
#        define COMxx1_RIGHT COM1A1
#        define OCRxx_RIGHT OCR1A
#    elif BACKLIGHT_PIN_RIGHT == B2
#        define COMxx0_RIGHT COM1B0
#        define COMxx1_RIGHT COM1B1
#        define OCRxx_RIGHT OCR1B
#    endif
#elif !defined(B5_AUDIO_RIGHT) && !defined(B6_AUDIO_RIGHT) && !defined(B7_AUDIO_RIGHT)
// Timer 1 is not in use by Audio feature, Backlight can use it
#    pragma message "Using hardware timer 1 with software PWM"
#    define HARDWARE_PWM_RIGHT
#    define BACKLIGHT_PWM_TIMER_RIGHT
#    define ICRx_RIGHT ICR1
#    define TCCRxA_RIGHT TCCR1A
#    define TCCRxB_RIGHT TCCR1B
#    define TIMERx_COMPA_vect_RIGHT TIMER1_COMPA_vect
#    define TIMERx_OVF_vect_RIGHT TIMER1_OVF_vect
#    if defined(__AVR_ATmega32A__)  // This MCU has only one TIMSK register
#        define TIMSKx_RIGHT TIMSK
#    else
#        define TIMSKx_RIGHT TIMSK1
#    endif
#    define TOIEx_RIGHT TOIE1

#    define OCIExA_RIGHT OCIE1A
#    define OCRxx_RIGHT OCR1A
#elif !defined(C6_AUDIO_RIGHT) && !defined(C5_AUDIO_RIGHT) && !defined(C4_AUDIO_RIGHT)
#    pragma message "Using hardware timer 3 with software PWM"
// Timer 3 is not in use by Audio feature, Backlight can use it
#    define HARDWARE_PWM_RIGHT
#    define BACKLIGHT_PWM_TIMER_RIGHT
#    define ICRx_RIGHT ICR1
#    define TCCRxA_RIGHT TCCR3A
#    define TCCRxB_RIGHT TCCR3B
#    define TIMERx_COMPA_vect_RIGHT TIMER3_COMPA_vect
#    define TIMERx_OVF_vect_RIGHT TIMER3_OVF_vect
#    define TIMSKx_RIGHT TIMSK3
#    define TOIEx_RIGHT TOIE3

#    define OCIExA_RIGHT OCIE3A
#    define OCRxx_RIGHT OCR3A
#elif defined(BACKLIGHT_CUSTOM_DRIVER_RIGHT)
error("Please set 'BACKLIGHT_DRIVER_RIGHT = custom' within rules.mk")
#else
error("Please set 'BACKLIGHT_DRIVER_RIGHT = software' within rules.mk")
#endif

#endif

#ifndef BACKLIGHT_PWM_TIMER  // pwm through software

static inline void enable_pwm(void) {
#    if BACKLIGHT_ON_STATE == 1
    TCCRxA |= _BV(COMxx1);
#    else
    TCCRxA |= _BV(COMxx1) | _BV(COMxx0);
#    endif
}

static inline void disable_pwm(void) {
#    if BACKLIGHT_ON_STATE == 1
    TCCRxA &= ~(_BV(COMxx1));
#    else
    TCCRxA &= ~(_BV(COMxx1) | _BV(COMxx0));
#    endif
}

#endif

#ifdef SPLIT_KEYBOARD
#ifndef BACKLIGHT_PWM_TIMER_RIGHT

static inline void enable_pwm_right(void) {
#    if BACKLIGHT_ON_STATE == 1
    TCCRxA_RIGHT |= _BV(COMxx1_RIGHT);
#    else
    TCCRxA_RIGHT |= _BV(COMxx1_RIGHT) | _BV(COMxx0_RIGHT);
#    endif
}

static inline void disable_pwm_right(void) {
#    if BACKLIGHT_ON_STATE == 1
    TCCRxA_RIGHT &= ~(_BV(COMxx1_RIGHT));
#    else
    TCCRxA_RIGHT &= ~(_BV(COMxx1_RIGHT) | _BV(COMxx0_RIGHT));
#    endif
}
#endif
#endif

#ifdef BACKLIGHT_PWM_TIMER

// The idea of software PWM assisted by hardware timers is the following
// we use the hardware timer in fast PWM mode like for hardware PWM, but
// instead of letting the Output Match Comparator control the led pin
// (which is not possible since the backlight is not wired to PWM pins on the
// CPU), we do the LED on/off by oursleves.
// The timer is setup to count up to 0xFFFF, and we set the Output Compare
// register to the current 16bits backlight level (after CIE correction).
// This means the CPU will trigger a compare match interrupt when the counter
// reaches the backlight level, where we turn off the LEDs,
// but also an overflow interrupt when the counter rolls back to 0,
// in which we're going to turn on the LEDs.
// The LED will then be on for OCRxx/0xFFFF time, adjusted every 244Hz.

// Triggered when the counter reaches the OCRx value
ISR(TIMERx_COMPA_vect) { backlight_pins_off(); }

// Triggered when the counter reaches the TOP value
// this one triggers at F_CPU/65536 =~ 244 Hz
ISR(TIMERx_OVF_vect) {
#    ifdef BACKLIGHT_BREATHING
    if (is_breathing()) {
        breathing_task();
    }
#    endif
    // for very small values of OCRxx (or backlight level)
    // we can't guarantee this whole code won't execute
    // at the same time as the compare match interrupt
    // which means that we might turn on the leds while
    // trying to turn them off, leading to flickering
    // artifacts (especially while breathing, because breathing_task
    // takes many computation cycles).
    // so better not turn them on while the counter TOP is very low.
    if (OCRxx > 256) {
        backlight_pins_on();
    }
}

#endif

#ifdef BACKLIGHT_PWM_TIMER_RIGHT
// Triggered when the counter reaches the OCRx value
ISR(TIMERx_COMPA_vect_RIGHT) { backlight_pins_off_right(); }

// Triggered when the counter reaches the TOP value
// this one triggers at F_CPU/65536 =~ 244 Hz
ISR(TIMERx_OVF_vect_RIGHT) {
#    ifdef BACKLIGHT_BREATHING
    if (is_breathing()) {
        breathing_task();
    }
#    endif
    // for very small values of OCRxx (or backlight level)
    // we can't guarantee this whole code won't execute
    // at the same time as the compare match interrupt
    // which means that we might turn on the leds while
    // trying to turn them off, leading to flickering
    // artifacts (especially while breathing, because breathing_task
    // takes many computation cycles).
    // so better not turn them on while the counter TOP is very low.
    if (OCRxx_RIGHT > 256) {
        backlight_pins_on_right();
    }
}

#endif

#define TIMER_TOP 0xFFFFU

// See http://jared.geek.nz/2013/feb/linear-led-pwm
static uint16_t cie_lightness(uint16_t v) {
    if (v <= 5243)     // if below 8% of max
        return v / 9;  // same as dividing by 900%
    else {
        uint32_t y = (((uint32_t)v + 10486) << 8) / (10486 + 0xFFFFUL);  // add 16% of max and compare
        // to get a useful result with integer division, we shift left in the expression above
        // and revert what we've done again after squaring.
        y = y * y * y >> 8;
        if (y > 0xFFFFUL)  // prevent overflow
            return 0xFFFFU;
        else
            return (uint16_t)y;
    }
}

// range for val is [0..TIMER_TOP]. PWM pin is high while the timer count is below val.
static inline void set_pwm(uint16_t val) { OCRxx = val; }

#ifdef SPLIT_KEYBOARD
static inline void set_pwm_right(uint16_t val) { OCRxx_RIGHT = val; }
#endif

#ifdef SPLIT_KEYBOARD
void backlight_set_left(uint8_t level) {
#else
void backlight_set(uint8_t level) {
#endif
    if (level > BACKLIGHT_LEVELS) level = BACKLIGHT_LEVELS;

    if (level == 0) {
#ifdef BACKLIGHT_PWM_TIMER
        if (OCRxx) {
            TIMSKx &= ~(_BV(OCIExA));
            TIMSKx &= ~(_BV(TOIEx));
        }
#else
        // Turn off PWM control on backlight pin
        disable_pwm();
#endif
        backlight_pins_off();
    } else {
#ifdef BACKLIGHT_PWM_TIMER
        if (!OCRxx) {
            TIMSKx |= _BV(OCIExA);
            TIMSKx |= _BV(TOIEx);
        }
#else
        // Turn on PWM control of backlight pin
        enable_pwm();
#endif
    }
    // Set the brightness
    set_pwm(cie_lightness(TIMER_TOP * (uint32_t)level / BACKLIGHT_LEVELS));
}

#ifdef SPLIT_KEYBOARD
void backlight_set_right(uint8_t level) {
    if (level > BACKLIGHT_LEVELS) level = BACKLIGHT_LEVELS;

    if (level == 0) {
#ifdef BACKLIGHT_PWM_TIMER_RIGHT
        if (OCRxx_RIGHT) {
            TIMSKx_RIGHT &= ~(_BV(OCIExA_RIGHT));
            TIMSKx_RIGHT &= ~(_BV(TOIEx_RIGHT));
        }
#else
        // Turn off PWM control on backlight pin
        disable_pwm_right();
#endif
        backlight_pins_off_right();
    } else {
#ifdef BACKLIGHT_PWM_TIMER_RIGHT
        if (!OCRxx_RIGHT) {
            TIMSKx_RIGHT |= _BV(OCIExA_RIGHT);
            TIMSKx_RIGHT |= _BV(TOIEx_RIGHT);
        }
#else
        // Turn on PWM control of backlight pin
        enable_pwm_right();
#endif
    }
    // Set the brightness
    set_pwm_right(cie_lightness(TIMER_TOP * (uint32_t)level / BACKLIGHT_LEVELS));
}
#endif

#ifdef SPLIT_KEYBOARD
void backlight_set(uint8_t level) {
    if (isLeftHand) {
        backlight_set_left(level);
    } else {
        backlight_set_right(level);
    }
}
#endif

void backlight_task(void) {}

#ifdef SPLIT_KEYBOARD
void backlight_task_right(void) {}
#endif

#ifdef BACKLIGHT_BREATHING

#    define BREATHING_NO_HALT 0
#    define BREATHING_HALT_OFF 1
#    define BREATHING_HALT_ON 2
#    define BREATHING_STEPS 128

static uint8_t  breathing_halt    = BREATHING_NO_HALT;
static uint16_t breathing_counter = 0;

#    ifdef BACKLIGHT_PWM_TIMER
static bool breathing = false;

bool is_breathing(void) { return breathing; }

#        define breathing_interrupt_enable() \
            do {                             \
                breathing = true;            \
            } while (0)
#        define breathing_interrupt_disable() \
            do {                              \
                breathing = false;            \
            } while (0)
#    else

bool is_breathing(void) { return !!(TIMSKx & _BV(TOIEx)); }

#        define breathing_interrupt_enable() \
            do {                             \
                TIMSKx |= _BV(TOIEx);        \
            } while (0)
#        define breathing_interrupt_disable() \
            do {                              \
                TIMSKx &= ~_BV(TOIEx);        \
            } while (0)
#    endif

#    define breathing_min()        \
        do {                       \
            breathing_counter = 0; \
        } while (0)
#    define breathing_max()                                       \
        do {                                                      \
            breathing_counter = get_breathing_period() * 244 / 2; \
        } while (0)

void breathing_enable(void) {
    breathing_counter = 0;
    breathing_halt    = BREATHING_NO_HALT;
    breathing_interrupt_enable();
}

void breathing_pulse(void) {
    if (get_backlight_level() == 0)
        breathing_min();
    else
        breathing_max();
    breathing_halt = BREATHING_HALT_ON;
    breathing_interrupt_enable();
}

void breathing_disable(void) {
    breathing_interrupt_disable();
    // Restore backlight level
    backlight_set(get_backlight_level());
}

void breathing_self_disable(void) {
    if (get_backlight_level() == 0)
        breathing_halt = BREATHING_HALT_OFF;
    else
        breathing_halt = BREATHING_HALT_ON;
}

/* To generate breathing curve in python:
 * from math import sin, pi; [int(sin(x/128.0*pi)**4*255) for x in range(128)]
 */
static const uint8_t breathing_table[BREATHING_STEPS] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 17, 20, 24, 28, 32, 36, 41, 46, 51, 57, 63, 70, 76, 83, 91, 98, 106, 113, 121, 129, 138, 146, 154, 162, 170, 178, 185, 193, 200, 207, 213, 220, 225, 231, 235, 240, 244, 247, 250, 252, 253, 254, 255, 254, 253, 252, 250, 247, 244, 240, 235, 231, 225, 220, 213, 207, 200, 193, 185, 178, 170, 162, 154, 146, 138, 129, 121, 113, 106, 98, 91, 83, 76, 70, 63, 57, 51, 46, 41, 36, 32, 28, 24, 20, 17, 15, 12, 10, 8, 6, 5, 4, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Use this before the cie_lightness function.
static inline uint16_t scale_backlight(uint16_t v) { return v / BACKLIGHT_LEVELS * get_backlight_level(); }

#    ifdef BACKLIGHT_PWM_TIMER
void breathing_task(void)
#    else
/* Assuming a 16MHz CPU clock and a timer that resets at 64k (ICR1), the following interrupt handler will run
 * about 244 times per second.
 */
ISR(TIMERx_OVF_vect)
#    endif
{
    uint8_t  breathing_period = get_breathing_period();
    uint16_t interval         = (uint16_t)breathing_period * 244 / BREATHING_STEPS;
    // resetting after one period to prevent ugly reset at overflow.
    breathing_counter = (breathing_counter + 1) % (breathing_period * 244);
    uint8_t index     = breathing_counter / interval % BREATHING_STEPS;

    if (((breathing_halt == BREATHING_HALT_ON) && (index == BREATHING_STEPS / 2)) || ((breathing_halt == BREATHING_HALT_OFF) && (index == BREATHING_STEPS - 1))) {
        breathing_interrupt_disable();
    }

    set_pwm(cie_lightness(scale_backlight((uint16_t)pgm_read_byte(&breathing_table[index]) * 0x0101U)));
}

#endif  // BACKLIGHT_BREATHING

#ifdef SPLIT_KEYBOARD
void backlight_init_ports_left(void) {
#else
void backlight_init_ports(void) {
#endif
    // Setup backlight pin as output and output to on state.
    backlight_pins_init();

    // I could write a wall of text here to explain... but TL;DW
    // Go read the ATmega32u4 datasheet.
    // And this: http://blog.saikoled.com/post/43165849837/secret-konami-cheat-code-to-high-resolution-pwm-on

#ifdef BACKLIGHT_PWM_TIMER
    // TimerX setup, Fast PWM mode count to TOP set in ICRx
    TCCRxA = _BV(WGM11);  // = 0b00000010;
    // clock select clk/1
    TCCRxB = _BV(WGM13) | _BV(WGM12) | _BV(CS10);  // = 0b00011001;
#else                                              // hardware PWM
    // Pin PB7 = OCR1C (Timer 1, Channel C)
    // Compare Output Mode = Clear on compare match, Channel C = COM1C1=1 COM1C0=0
    // (i.e. start high, go low when counter matches.)
    // WGM Mode 14 (Fast PWM) = WGM13=1 WGM12=1 WGM11=1 WGM10=0
    // Clock Select = clk/1 (no prescaling) = CS12=0 CS11=0 CS10=1

    /*
    14.8.3:
    "In fast PWM mode, the compare units allow generation of PWM waveforms on the OCnx pins. Setting the COMnx1:0 bits to two will produce a non-inverted PWM [..]."
    "In fast PWM mode the counter is incremented until the counter value matches either one of the fixed values 0x00FF, 0x01FF, or 0x03FF (WGMn3:0 = 5, 6, or 7), the value in ICRn (WGMn3:0 = 14), or the value in OCRnA (WGMn3:0 = 15)."
    */
#    if BACKLIGHT_ON_STATE == 1
    TCCRxA = _BV(COMxx1) | _BV(WGM11);
#    else
    TCCRxA = _BV(COMxx1) | _BV(COMxx0) | _BV(WGM11);
#    endif

    TCCRxB = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
#endif
    // Use full 16-bit resolution. Counter counts to ICR1 before reset to 0.
    ICRx = TIMER_TOP;

    backlight_init();
#ifdef BACKLIGHT_BREATHING
    if (is_backlight_breathing()) {
        breathing_enable();
    }
#endif
}


#ifdef SPLIT_KEYBOARD

void backlight_init_ports_right(void) {
    // Setup backlight pin as output and output to on state.
    backlight_pins_init_right();

    // I could write a wall of text here to explain... but TL;DW
    // Go read the ATmega32u4 datasheet.
    // And this: http://blog.saikoled.com/post/43165849837/secret-konami-cheat-code-to-high-resolution-pwm-on

#ifdef BACKLIGHT_PWM_TIMER_RIGHT
    // TimerX setup, Fast PWM mode count to TOP set in ICRx
    TCCRxA_RIGHT = _BV(WGM11);  // = 0b00000010;
    // clock select clk/1
    TCCRxB_RIGHT = _BV(WGM13) | _BV(WGM12) | _BV(CS10);  // = 0b00011001;
#else                                              // hardware PWM
    // Pin PB7 = OCR1C (Timer 1, Channel C)
    // Compare Output Mode = Clear on compare match, Channel C = COM1C1=1 COM1C0=0
    // (i.e. start high, go low when counter matches.)
    // WGM Mode 14 (Fast PWM) = WGM13=1 WGM12=1 WGM11=1 WGM10=0
    // Clock Select = clk/1 (no prescaling) = CS12=0 CS11=0 CS10=1

    /*
    14.8.3:
    "In fast PWM mode, the compare units allow generation of PWM waveforms on the OCnx pins. Setting the COMnx1:0 bits to two will produce a non-inverted PWM [..]."
    "In fast PWM mode the counter is incremented until the counter value matches either one of the fixed values 0x00FF, 0x01FF, or 0x03FF (WGMn3:0 = 5, 6, or 7), the value in ICRn (WGMn3:0 = 14), or the value in OCRnA (WGMn3:0 = 15)."
    */
#    if BACKLIGHT_ON_STATE == 1
    TCCRxA_RIGHT = _BV(COMxx1_RIGHT) | _BV(WGM11);
#    else
    TCCRxA_RIGHT = _BV(COMxx1_RIGHT) | _BV(COMxx0_RIGHT) | _BV(WGM11);
#    endif

    TCCRxB_RIGHT = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
#endif
    // Use full 16-bit resolution. Counter counts to ICR1 before reset to 0.
    ICRx_RIGHT = TIMER_TOP;

    backlight_init();
#ifdef BACKLIGHT_BREATHING
    if (is_backlight_breathing()) {
        breathing_enable();
    }
#endif
}
#endif

#ifdef SPLIT_KEYBOARD
void backlight_init_ports(void) {
    if (isLeftHand) {
        backlight_init_ports_left();
    } else {
        backlight_init_ports_right();
    }
}
#endif