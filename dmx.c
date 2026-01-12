#include "config.h"
#include "dmx.h"

volatile unsigned char dmxData[NUM_ADDRESSES];
unsigned short dmxAddr = 0; 

#ifdef TX_DEBUG
// A tiny Transmit Buffer 
#define TXBUFFERSIZE 16
unsigned char txQueue[TXBUFFERSIZE];
unsigned char txHead=0;
volatile unsigned char txTail=0;
volatile unsigned char txBusy=0;
#endif

void dmxSetAddress(unsigned short newAddr) {
   // Updating SHARED 16 bit value 
   // needs to be protected from interrupt
   ES=0;
   dmxAddr = newAddr;
   ES=1;
}

void dmxUartInit()
{
    int i = 0;
    for(i = 0; i < NUM_ADDRESSES; ++i)
        dmxData[i] = 0;

    //DMX uses one start bit, 2 stopbits and no parity. That is mode 3 on the pcs15w404s

    /**
    set uart1 into 8-bit variable baud rate mode

    SM0=1 and SM1=1 => Mode 3 8 bitvariable baudrate async, one start bit, one stop bit, one programmable stop bit (TB8)
    SM2=0 => Disable automatic address recognition
    REN=1 => enable serial reception
    TB8=1 => set to 1 because dmx uses two stop bits
    RB8 is set by the hardware and contains the value of the first stop bit (always one for DMX)
    TI/RI are interrupt flags that will be set by hardware*/
    SCON = 0xD8;

    //calculate timer overlow values based to achieve BAUD rate based on cpu frequency
    //copied from the example code in offcial documentation

    AUXR = 0x05; // T2 in 1T mode
    //AUXR1 = 0x01; // Use P3.0,P3.1 for UART (default behavior, commented out)
    // LabRat: Updated to remove build warnings. Was ...
    //   T2L  =  (65536 - (FOSC/4/BAUD));    //Set the preload value
    T2L  =  (unsigned char) (65536 - (FOSC/4/BAUD))&0xFF;    //Set the preload value
    T2H  =  (65536 - (FOSC/4/BAUD))>>8;

    AUXR |= 0x10; // Start Timer 2

    PS = 0; //set uart interrupt to low priority

    ES  =  1; //enable UART1 interrupt
    EA  =  1; //enable all interrupts
}

void dmxUartInterrupt()  __interrupt(SI0_VECTOR) __using(2)
{
   static unsigned short bytesReceived = 0;
   static unsigned char  startCodeValid = 0;
   static unsigned char  rxData;

    /** NOTE
     *  we could enable frame error detection and check for frame error instead of RB8.
     *  But if we do that we keep missing the start code.
     *  I have no idea why that happens :-(
     *  But ignoring frame errors and just checking for RB8 works fine*/


    if(RI) //if we received a byte
    {
        RI = 0;

        //check 9th bit (first stop bit)
        if(RB8 == 1) //valid frame
        {
            rxData = SBUF;
            //check if start code is correct (i.e. is zero for normal dmx signal)
            if(bytesReceived == 0 && rxData == 0)
            {
                startCodeValid = 1;
            }
            //if start was valid and the current byte is in our address range
            else if (startCodeValid
                 && (bytesReceived >= dmxAddr)
                 && (bytesReceived < (dmxAddr + NUM_ADDRESSES)))
            {
                dmxData[bytesReceived - dmxAddr] = rxData;
            }
            bytesReceived++;
            if (bytesReceived > 512) {
               bytesReceived = 0;
               startCodeValid = 0;
            }
        }
        else //invalid frame.
        {
            #ifdef PWR_LED
               //check if frame is complete.
               //we dont check for (bytesReceived == 513) because dmx controllers are allowed
               //to end a frame early when there are no lights configured in that range.
               if(bytesReceived >= dmxAddr + NUM_ADDRESSES)
               {
                   //turn off power led.
                   //the main loop will turn it back on resulting in a flickering led
                   //if dmx is present
                   P0_3 = 1;
               }
            #endif

            bytesReceived = 0;
            startCodeValid = 0;
        }
    }

    if (TI) //if we have sent a byte
    {
        TI = 0; //clear transmit interrupt
#ifdef TX_DEBUG
        if (txHead != txTail){
           ACC = txQueue[txTail];
           SBUF= ACC;
           txTail = (txTail+1)& (TXBUFFERSIZE-1);
        } else {
           txBusy = 0;
        }
#endif
    }
}


#ifdef TX_DEBUG
unsigned char sendByte(unsigned char data) {
   unsigned char next = (txHead+1)&(TXBUFFERSIZE-1);
   if (next == txTail) return 0;
   txQueue[txHead] = data;
   txHead = next;
   if (!txBusy) {
      txBusy = 1;
      // Note: txBusy = 0 so no TI interrupt possible. 
      // Therefore no need to disable interrupts to update txTail
      ACC = txQueue[txTail];
      SBUF= ACC;
      txTail = (txTail+1)&(TXBUFFERSIZE-1);
   }
   return 1;
}
#endif
