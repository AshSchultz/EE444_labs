
#include <msp430.h>

extern int IncrementVcore(void);

#define  NUM_PMM_COREV_LVLS 4

void main(void)
{
  int i;
  // Setting up DCO to 25mhz
  UCSCTL1 = DCORSEL_6;      //setting clock to correct frequency range
  UCSCTL2 = 762;     //clock divisor
  UCSCTL4 = SELM__DCOCLK | SELS__DCOCLK | SELA__DCOCLK; //enabling master,  submaster, and aclk
  UCSCTL8 &= ~(SMCLKREQEN);
  P11SEL |= BIT1 | BIT0 | BIT2;    //selecting special function for these pins (output main clock)
  P11DIR |= BIT1 | BIT0 | BIT2;    // setting direction to output
  // Timer setup
  TA1CTL = TASSEL_1 + MC_3 + TACLR;  //timerA clock source select (aclk), count up/down (CCR0), timer clear
  TA1CCTL1 = OUTMOD_4;
  TA1CCTL2 = OUTMOD_4;
  TA1CCR0 = 125;
  TA1CCR1 = 93;
  TA1CCR2 = 31;
  P8SEL |= BIT6;
  P8DIR |= BIT6;
  P7SEL |= BIT3;
  P7DIR |= BIT3;
//set core voltage to max
  for(i = 0; i < NUM_PMM_COREV_LVLS; i++) {
    IncrementVcore();
  }

  //Enter LPM0, enable interrupts
  //_EINT();
  //for(;;){}
  LPM0;
  //LPM3; 
}


//int t0 = 0;
//int t1 = 0;
//int t2 = 0;
//int delta_t = 0;
//void button_ISR(void) __interrupt[PORT2_VECTOR] { // interrupt service routine handler port 2
//  if(t0 == 0) {
//    P1OUT ^= BIT0; // led on
//    P10OUT ^= BIT0; //send trigger on to oscilloscope
//    t1 = TA1R; //reading current location of time
//    t0++;
//  } else {
//    t2 = TA1R;
//    delta_t = t2 - t1;
//    P10OUT ^= BIT0; //send another trigger to oscilloscope
//    P1OUT ^= BIT0; // led off
//    t1 = 0;
//    t0 = 0;
//  }
//  P2IV = 0; //clear interrupt flag
//}
