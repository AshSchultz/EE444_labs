#include <msp430.h>
void main(void) {
  // Setting up DCO to 12mhz
  UCSCTL1 = DCORSEL_5;
  UCSCTL2 = 365;
  UCSCTL4 = SELM__DCOCLK;
  P11SEL = BIT1; //setting output of main clock at spec pin
  P11DIR = BIT1;
  // Setting up interrupts for port 2
  P2DIR &= ~BIT6;
  P2IES |= BIT6;
  P2IE |= BIT6;
  P2REN |= BIT6; //setup pullup res
  P2OUT |= BIT6;
  //Just in case...
  P2IFG = 0;
  P1SEL &= ~BIT0;
  P1DIR |= BIT0;

  _EINT(); //enables interrupts
  
  LPM0; //sleepy
}

void button_ISR(void) __interrupt[PORT2_VECTOR] { //interrupt service routine handler port 2
  P1OUT ^= BIT0; //toggle led pin
  P2IV = 0; //clearign interrupt flag
}