#ifndef _CONFIG_H_
#define _CONFIG_H_
#include <mcs51/lint.h>
#include <mcs51/8051.h>
#include "stc15w.h"


#define FOSC  24000000UL //System frequency.
//#define FOSC  32000000UL //System frequency.
#define BAUD  250000UL    //UART1 baud-rate

//ATTENTION there is an unrolled loop in leds.c depending on this number.
//         if you change this number, also fix the loop
#define NUM_LEDS 3

#define NUM_ADDRESSES (NUM_LEDS+2) //number of dmx adresses to use

// Pin connected to dedicated PWR LED
// Comment out if not present
//#define PWR_LED P0_3

#define USE_DIP_SHIFT
#define USE_STROBE

// Audio detect pin
#define MIC_INPUT P5_5

#endif
