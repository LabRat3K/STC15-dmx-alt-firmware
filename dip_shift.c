#include "dip.h"

/* dip mapping:
   DIP1-8: SHIFT REGISTER
   SHIFT_LATCH P3_7
   SHIFT_CLOCK P3_6
   SHIFT_DATA  P3_3
   9  = P1_5
   10 = P1_4
*/

#define SHIFT_LATCH P3_7
#define SHIFT_CLOCK P3_6
#define SHIFT_DATA P3_3
#define PIN_9 P1_5
#define PIN_FUNC P1_4

unsigned short gDipCache = 0xFFFF; // Enumerate to invalid value
// A tiny state machine to latch and read the DIP switch status from
// the atteched shift register IC. 
// Note all ODD number states are a DO NOTHING to allow the signals to settle
unsigned char gDipReadState = 0;
  

#define WAIT_CYCLES 8 
// Quick and Dirty lookup table to deal with pin mapping on the DIP switches
const unsigned char manglemap[16]={0x00,0x80,0x10,0x90,0x20,0xa0,0x30,0xb0,0x40,0xc0,0x50,0xd0,0x60,0xe0,0x70,0xf0};

unsigned short result;
unsigned char bitCount; // Count down number of bits shifted
unsigned char waitCount=WAIT_CYCLES;
unsigned short readDmxAddr()
{
    //initialize unused bits as 1 (will later be inverted to 0)

    unsigned char uNibble = (result & 0xF0)>>4;

    if ((gDipReadState<9) && !(gDipReadState & 0x01)) {
       switch(gDipReadState) {
          case 0: SHIFT_LATCH = 0;
                  bitCount = 8;
                  result = 0xfe;
                  break;
          case 2: SHIFT_LATCH = 1;
                  break;
          case 4: result = (result<<1) | SHIFT_DATA;
                  SHIFT_CLOCK = 1;
                  break;
          case 6: SHIFT_CLOCK = 0;
                  bitCount--;
                  // Keep looping until all 8 bits read
                  if (bitCount) gDipReadState=3;
                  break;
          case 8: //due to the pullups we read a 1 when the dip is in off-position.
                  //thus invert every pin
                  // NOTE: The "brilliant" designers of this PCB didn't route the switchs in order. 
                  // So some bit manipulation afer the fact is required (don't blame me)
                  uNibble = (result&0xF0)>>4; 
                  gDipCache = ~((result & 0xFE0F) | manglemap[uNibble] | (PIN_9 << 8));
                  break;
       }
    } else {
       // Wait .. 
       waitCount--;
       if (waitCount) return gDipCache;
       waitCount = WAIT_CYCLES; // ready for next pause
       // Fall through to next state
    }
    gDipReadState++;
    gDipReadState &= 0x1F; // Loop delay - reduce polling interval

    return gDipCache;
}

unsigned char readFunctionDip()
{
    return !PIN_FUNC;
}

void dipInit()
{
    //we use the quasi-bi-directional mode.
    //This is the default thus we dont have to configure the mode.

    SHIFT_LATCH = 0;
    SHIFT_CLOCK = 0;
    //turn on the pullups
    SHIFT_DATA = 1;

    // DipSwith Input configurations: High Impedance input 
    //P1M1 |= 0x20;
    P1M0 &= ~0x30;

    PIN_FUNC = 1;
    PIN_9 = 1;

    // FORCE a POLL of the registers: 
    // High order bits will be cleared after clocking the bits in
    while (readDmxAddr() == 0xFFFF);
}
