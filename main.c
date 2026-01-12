#include "config.h"
#include "dmx.h"
#include "dip.h"
#include "leds.h"

//400 interrupts/s 
// on time of a strobe flash in ~2.5ms steps
#define STROBE_ON_TIME_MS 3

volatile unsigned char  functionCache = 1; // Assert in DMX mode to begin
volatile unsigned short switchCache=0x8000;  // Out of range, to force 1st read

unsigned short masterBrightness = 0;

#define NUM_LIGHT_SEQ 8
unsigned char currentPattern = 0;
const unsigned char SEQUENCE[NUM_LIGHT_SEQ][3] = {
   {0x00,0x00,0x00},
   {0xFF,0x00,0x00},
   {0x00,0xFF,0x00},
   {0x00,0x00,0xFF},
   {0x80,0x80,0x00},
   {0x00,0x80,0x80},
   {0x80,0x00,0x80},
   {0xFF,0xFF,0xFF}
  };

#ifdef PWR_LED
   //used to clicker the power led
   unsigned char pwrLedCnt = 0;

   inline void flickerPwrLed()
   {
       //if power led has been turned off by uart, leave it off for 255 loop iterations
       if(PWR_LED)
       {
           pwrLedCnt++;
           if(pwrLedCnt == 255)
           {
               //turn on power led 
               //uart turns it off when it has received a correct frame.
               //this results in flickering if dmx is present and steady on if no dmx present.
               PWR_LED = 0;
               pwrLedCnt = 0;
           }
       }
   }
#endif

void readDipSwitch()
{
    unsigned short tempValues = readDmxAddr();
    unsigned char functionBit = readFunctionDip();
    unsigned char staticUpdate = 0;

    if (functionBit != functionCache) {
       functionCache = functionBit;
       if (!functionCache) {
          // Toggle TO static.. update leds
          staticUpdate = 1;
       }
    }

    if (switchCache != tempValues)  {
       switchCache = tempValues;

       if (tempValues == 0) {
              tempValues = 1;
       } else {
          if (tempValues > 512 - NUM_ADDRESSES) {
             tempValues = 512 - NUM_ADDRESSES;
          }
       }
       dmxSetAddress(tempValues);
       if (!functionCache) {
          // change to dip switch while static.
          //  so update the leds
          staticUpdate = 1;
       }
    } 

    if (staticUpdate) {
       // Static Display : lamp values pulled from DIP settings
       ledBrightness[0] = (switchCache&0x07)<<5; // RED   (3 bits)
       ledBrightness[1] = (switchCache&0x38)<<2; // GREEN (3 bits)
       ledBrightness[2] = (switchCache&0xC0); // BLUE  (2 bits)
       masterBrightness = 255;
    }

}


inline unsigned char calcStrobeTimeMs(unsigned char strobeDmxVal)
{
    // maps between 200 and 10 timer ticks
    // i.e. 500ms and 25ms delay
    // i.e. 2hz and 40hz

    /** 
     * Unoptimized floating point version of this function:
     * 
     * long map(long x, long in_min, long in_max, long out_min, long out_max) 
     * {
     *     return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
     * }
     * return map(strobeDmxVal, 0, 255, 255, 25);
     * 
     * This code has been optimized by using fixed comma math with factor 255.
     */

    
    return (51000 - strobeDmxVal * 190)/255;

}

void main()
{
#ifdef USE_STROBE
    unsigned char strobeDmx = 0;
    unsigned char strobeOffTime = 0;
    
    unsigned char oldStrobe = 0;
    unsigned char strobeOn = 0; //current state of strobe (led on or off)
#endif


    dipInit();

    readDipSwitch();

    ledInit(); //modifies AUXR

    dmxUartInit(); //initially sets AUXR

#ifdef PWR_LED
    PWR_LED = 0; //turn on power
#endif

    while(1)
    {
#ifdef PWR_LED
        flickerPwrLed();
#endif
        readDipSwitch();

        if (functionCache) { // If mode = DMX
#ifdef USE_STROBE
           strobeDmx = dmxData[1];
           if(!oldStrobe && strobeDmx) {
               //strobe has been turned on, reset strobe start time to now
               strobeCount = 0;
               strobeOn = 1;
           }
           oldStrobe = strobeDmx;

           if(strobeDmx) {
               if(strobeOn) {
                   //check if strobe flash needs to be turned off
                   if(strobeCount >= STROBE_ON_TIME_MS) {
                       //led was on long enough, turn off
                       masterBrightness = 0;
                       strobeOn = 0;
                       strobeCount = 0;
                   } else {
                       //led should stay on (or turn on)
                       masterBrightness = dmxData[0];
                   }
               } else {
                   strobeOffTime = calcStrobeTimeMs(strobeDmx);
                   //check if it is time to turn on strobe
                   if(strobeCount > strobeOffTime) {
                       //time to turn the strobe back on
                       masterBrightness = dmxData[0];
                       strobeOn = 1;
                       strobeCount = 0;
                   } else {
                       //leave it off a little longer (or turn it off)
                       masterBrightness = 0;
                   }
               }
           } else 
#endif
           {
               masterBrightness = dmxData[0];
           }

           // The master scaling is done in fixed point math with scale 255
           // 255 was chosen because it allows to ommit scaling of masterBrightness
           // and is close to the theoretical maximum scale of 257 
           // (255*257=biggest possible unsigned short).

           //loop unrolled for performance reasons
           ledBrightness[0] = (dmxData[2] * masterBrightness) / 255; // RED
           ledBrightness[1] = (dmxData[3] * masterBrightness) / 255; // GREEN
           ledBrightness[2] = (dmxData[4] * masterBrightness) / 255; // BLUE

#if NUM_LEDS > 3
           ledBrightness[3] = (dmxData[5] * masterBrightness) / 255;
           ledBrightness[4] = (dmxData[6] * masterBrightness) / 255;
           ledBrightness[5] = (dmxData[7] * masterBrightness) / 255;
           ledBrightness[6] = (dmxData[8] * masterBrightness) / 255;
           ledBrightness[7] = (dmxData[9] * masterBrightness) / 255;
#endif
       } else {
#ifdef MIC_INPUT
         // THIS is the MICROPHONE INPUT code
         if (switchCache == 0) {
            if (micCount) { // Arbitrary sensitivity could be >10?
               currentPattern = (currentPattern+1)&(NUM_LIGHT_SEQ-1);;
               ledBrightness[0] = SEQUENCE[currentPattern][0];
               ledBrightness[1] = SEQUENCE[currentPattern][1];
               ledBrightness[2] = SEQUENCE[currentPattern][2];
            }
         }
#endif
       }

       WDT_CONTR = 0x3C; // Feed the watchdog
    }
}
