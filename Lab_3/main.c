
#include <msp430.h>

void main(void)
{
  int i;
  // Setting up DCO to 25mhz
  UCSCTL1 = DCORSEL_5;      //setting clock to correct frequency range
  UCSCTL2 = 487;  //clock divisor
  UCSCTL4 = SELM__DCOCLK | SELS__DCOCLK; //enabling master,  submaster
  UCSCTL8 &= ~(SMCLKREQEN);
  P11SEL |= BIT1 | BIT0 | BIT2;    //selecting special function for these pins (output main clock)
  P11DIR |= BIT1 | BIT0 | BIT2;    // setting direction to output
  //Led/pin setup
  P1DIR |= BIT0;
  P10DIR |= BIT0;
  // Timer setup
  TA1CTL = TASSEL_1 + MC__UP + TACLR;  //timerA clock source select (aclk), count up/down (CCR0), timer clear
  TA1CCTL0 = CCIE;
  TA1CCR0 = 32768;
  // ADC setup
  ADC12MCTL0 |= ADC12INCH_10;
  //Enter LPM0, enable interrupts
  _EINT();
  LPM0;
  //LPM3; 
}

void timerA_ISR(void) __interrupt[TIMER1_A0_VECTOR] {
    P1OUT ^= BIT0; // led on
    P10OUT ^= BIT0; //send trigger on to oscilloscope
    TA1CCTL0 &= ~(CCIFG);
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
