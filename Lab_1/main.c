#include <msp430.h>

extern int IncrementVcore(void);

#define  NUM_PMM_COREV_LVLS 4

void main(void)
{
  int i;
  // Setting up DCO to 25mhz
  UCSCTL1 = DCORSEL_6;
  UCSCTL2 = 762;
  UCSCTL4 = SELM__DCOCLK | SELS__DCOCLK | SELA__REFOCLK;
  P11SEL |= BIT1 | BIT0 | BIT2; //setting output of main clock at spec pin
  P11DIR |= BIT1 | BIT0 | BIT2;
  // Timer setup
  P1DIR |= BIT0;
  TA1CCTL0 = CCIE | CM_1 | CAP;
  TA1CTL = TASSEL_2 + MC_2 + TACLR + ID_3;

  // Set output of Port 8 pin 5
  P10DIR |= BIT0;

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

  for(i = 0; i < NUM_PMM_COREV_LVLS; i++) {
    IncrementVcore();
  }

  //Enter LPM0, enable interrupts
  _EINT();
  for(;;){}
  //LPM0;
}


//void CCR0_ISR(void) __interrupt[TIMER1_A0_VECTOR] {
//  if(t1 == 0) {
//    P8OUT ^= BIT5; //toggle
//    t1 = TA1CCR0;
//  } else {
//    t2 = TA1CCR0;
//    delta_t = abs(t2 - t1);
//    P8OUT ^= BIT5; //toggle
//  }
//  TA1CCTL0 &= ~CCIFG;
//}
int t0 = 0;
int t1 = 0;
int t2 = 0;
int delta_t = 0;
void button_ISR(void) __interrupt[PORT2_VECTOR] { //interrupt service routine handler port 2
  //TA1CCTL0 ^= CCIS_3; // Trigger CCR0_ISR
  if(t0 == 0) {
    P1OUT ^= BIT0;
    P10OUT ^= BIT0; //toggle
    t1 = TA1R;
    t0++;
  } else {
    t2 = TA1R;
    delta_t = t2 - t1;
    P10OUT ^= BIT0; //toggle
    P1OUT ^= BIT0;
    t1 = 0;
    t0 = 0;
  }
  P2IV = 0; //clear interrupt flag
}
