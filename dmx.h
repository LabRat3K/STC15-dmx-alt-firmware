#ifndef DMX_H
#define DMX_H
#include "config.h"

extern volatile unsigned char dmxData[NUM_ADDRESSES];
void dmxUartInit();

void dmxSetAddress(unsigned short newAddr);

// Interrupts MUST be visible to main()
void dmxUartInterrupt()  __interrupt(SI0_VECTOR) __using(1);

#ifdef TX_DEBUG
// non-blocking Transmit byte over serial - 16 byte queue
// returns number of bytes enqueued (1 or 0)
unsigned char sendByte(unsigned char data);
#endif

#endif
