#include <msp430.h>

extern int IncrementVcore(void);

#define  NUM_PMM_COREV_LVLS 4

void main(void)
{
  int i;
  // Setting up DCO to 25mhz
  UCSCTL1 = DCORSEL_6;      //setting clock to correct frequency range
  UCSCTL2 = 762;     //clock divisor
  UCSCTL4 = SELM__DCOCLK | SELS__DCOCLK | SELA__XT1CLK; //enabling master,  submaster, and aclk
  P11SEL |= BIT1 | BIT0 | BIT2;    //selecting special function for these pins (output main clock)
  P11DIR |= BIT1 | BIT0 | BIT2;    // setting direction to output
  // Timer setup
  P1DIR |= BIT0; /////I MIGHT NOT NEED THIS

  TA1CTL = TASSEL_1 + MC_2 + TACLR;  //timerA clock source select (aclk), count up continuous (65k), timer clear, 

  // Set output of Port 10 pin 0
  P10DIR |= BIT0;

  // Setting up interrupts for port 2
  P2DIR &= ~BIT6; // setting button to input on click
  P2IES |= BIT6;  // interrupt edge select
  P2IE |= BIT6;   // interrupt enable
  P2REN |= BIT6;  // enabling resistor
  P2OUT |= BIT6;  // setting to pullup resistor
  
  P2IFG = 0;      // reset interrupt flag
  P1SEL &= ~BIT0; // set to GPIO (disable special function)
  P1DIR |= BIT0;  // set to output


  //set core voltage to max
  for(i = 0; i < NUM_PMM_COREV_LVLS; i++) {
    IncrementVcore();
  }

  //Enter LPM0, enable interrupts
  _EINT();
  for(;;){}
  LPM0; 
}


int t0 = 0;
int t1 = 0;
int t2 = 0;
int delta_t = 0;
void button_ISR(void) __interrupt[PORT2_VECTOR] { // interrupt service routine handler port 2
  if(t0 == 0) {
    P1OUT ^= BIT0; // led on
    P10OUT ^= BIT0; //send trigger on to oscilloscope
    t1 = TA1R; //reading current location of time
    t0++;
  } else {
    t2 = TA1R;
    delta_t = t2 - t1;
    P10OUT ^= BIT0; //send another trigger to oscilloscope
    P1OUT ^= BIT0; // led off
    t1 = 0;
    t0 = 0;
  }
  P2IV = 0; //clear interrupt flag
}
