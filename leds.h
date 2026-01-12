#ifndef LEDS_H
#define LEDS_H

#include <mcs51/lint.h>
#include <mcs51/8051.h>
#include "stc15w.h"
#include "config.h"

extern volatile unsigned char ledBrightness[NUM_LEDS];

/** this value is increased by the led timer every 2,42130688ms .
 *  It is used in the strobe logic.
 *  The strobe logic also resets this value to zero every time a strobe flash finishes.
 *  The maximum delay the timer can achieve is  637ms */
extern volatile unsigned char strobeCount;

/* Counting the number of MICROPHONE pulses detected per 255 clock ticks */
extern volatile unsigned char micCount;

void ledInit();

// Interrupts must be visible to main()
void timer0Interrupt()  __interrupt(TF0_VECTOR) __using(1);
#endif
