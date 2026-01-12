#include "config.h"
#include "leds.h"

//software pwm for leds

//brightness array for leds, this is written to from somewhere else
volatile unsigned char ledBrightness[NUM_LEDS];

//27136 interrupts/s @ 2007916.66 increments/s
//#define TIMER_START 65462

//37888 interrupts/s = ~148hz led update rate
//#define TIMER_START (65536-53) 

//54272 interrupts/s = ~212hz led update rate
//#define TIMER_START (65536-37) 

//80384 interrupts/s = ~314hz led update rate
//#define TIMER_START (65536-25) 

//105728 interrupts/s ( every 9,45823us) = ~413hz led update rate 
#define TIMER_START (65536-19) 

volatile unsigned char strobeCount;
volatile unsigned char micCount; // count number of audio detected samples

void ledInit()
{
    
#ifdef ORIGINAL_DEVICE 
    //  8 channel controller
    P3 &= (~0x30);
    P2 &= (~0x7E);

    P3M0 = 0x30; //set P3.4 and P3.5 to strong push pull output
    P2M0 = 0x7e; //set P2.1 - P2.6 to strong push pull output
#else
    // 3 channel  STC15 device
    P1 &= (~0x0B);

    P1M0 |= 0x0B; // set P1.0,P1.1, and P1.3 to strong push pull output
#endif

    AUXR &= ~0x80;   // Set timer0 clock source to sysclk/12 (12T mode)
    TMOD &= 0xF0;    // Clear 4bit field for timer0

    //set reload counter
    TH0 = TIMER_START >> 8;
    TL0 = (unsigned char)TIMER_START;

    // set timer 0 interrupt priority to high
    // this is done to ensure stable pwm.
    // if we set this to low the uart will interrupt
    // the pwm and cause visible flicker when the leds are
    // at low brightness 
    PT0 = 1; 

    ET0 = 1; // Enable Timer0 interrupts
    EA  = 1; // Global interrupt enable

    TR0 = 1; // Start Timer 0
}

void timer0Interrupt()  __interrupt(TF0_VECTOR) __using(1)
{
   static volatile unsigned char timer0Cnt = 0;

   // Read::Modify::Write should be safe in interrupt context
   unsigned char temp_byte;

    // This code is not generic and kept unrolled for performance. 

    //NOTE currently 255 is not fully on, it will still turn off for one cycle.
    // FIXME try to fix it without decreasing the performance
    // LabRat: 
    //       Could try temp_byte |= ((time0Cnt<= ledBrightness[x]) && (ledBrightness[x])<<y);
    //       This would give 0 to 255 (always off and always on for extreme limits)

#ifdef ORIGINAL_DEVICE 
    //  8 channel controller
    temp_byte = P3&(~0x30);
    temp_byte |= ((timer0Cnt < ledBrightness[0])<<4); 
    temp_byte |= ((timer0Cnt < ledBrightness[1])<<5); 
    P3 = temp_byte;

    temp_byte = P2&(~0x7E);
    temp_byte |= ((timer0Cnt < ledBrightness[2])<<1); 
    temp_byte |= ((timer0Cnt < ledBrightness[3])<<2); 
    temp_byte |= ((timer0Cnt < ledBrightness[4])<<3); 
    temp_byte |= ((timer0Cnt < ledBrightness[5])<<4); 
    temp_byte |= ((timer0Cnt < ledBrightness[6])<<5); 
    temp_byte |= ((timer0Cnt < ledBrightness[7])<<6); 
    P2 = temp_byte;
#else
    // 3 channel  STC15 device
    temp_byte = P1&(~0x0B);
    // Warning: This method will over-write the weak-pullups if a 0 is read
    temp_byte |= (((timer0Cnt <= ledBrightness[0])&&ledBrightness[0])<<3);//RED
    temp_byte |= (((timer0Cnt <= ledBrightness[1])&&ledBrightness[1])<<1);//VERT
    temp_byte |= (((timer0Cnt <= ledBrightness[2])&&ledBrightness[2]));   //BLUE
    P1 = temp_byte| 0x30; // Add the weak-pullups
#endif

    //timer but some of the WS-DMX boards use really simple
    //STC controllers that only have two timers.
    //Therefore we generate the strobe tick in here.
    //one increment of strobeCount is ~2.48ms
    if(timer0Cnt == 255)
    {
        strobeCount++;
        micCount = 0;
    }
    micCount += (P5_5 == 0);
    timer0Cnt++;

}
