#include <msp430.h>

extern int IncrementVcore(void);

#define  NUM_PMM_COREV_LVLS 4

void main(void)
{
  int i;
  // Setting up DCO to 25mhz
  UCSCTL1 = DCORSEL_6; // Setting clock to correct frequency range
  UCSCTL2 = 762; // Clock divider
  // Setting master, submaster, and aclk to DCOCLK
  UCSCTL4 = SELM__DCOCLK | SELS__DCOCLK | SELA__DCOCLK; 
  // Disable SMCLK in Low Power Modes
  UCSCTL8 &= ~(SMCLKREQEN);
  // Enable output for MCLK, SMCLK, ACLK
  P11SEL |= BIT1 | BIT0 | BIT2;    // Selecting special function for these pins (output MCLK, SMCLK, ACLK)
  P11DIR |= BIT1 | BIT0 | BIT2; // setting direction to output
  // Timer setup
  TA1CTL = TASSEL_1 + MC_3 + TACLR; // timerA clock source select (aclk), count up/down (CCR0), timer clear
  TA1CCTL1 = OUTMOD_4; // When TA1R gets to TA1CCR1, toggle OUT1
  TA1CCTL2 = OUTMOD_4; // When TA1R gets to TA1CCR2, toggle OUT2
  TA1CCR0 = 125; // Set TA1CCR0 to 125, TA1R goes to that value, then descends
  TA1CCR1 = 93; // Set TA1CCR1 to 93, 25% duty cycle
  TA1CCR2 = 31; // Set TA1CCR2 to 31, 75% duty cycle
  // Setup PWM output pins
  P8SEL |= BIT6; // Setup P8.6 to special function (Output of OUT1)
  P8DIR |= BIT6; // Set that pin to output
  P7SEL |= BIT3; // Setup P7.3 to special function (Output of OUT2)
  P7DIR |= BIT3; // Set that pin to output
  //set core voltage to max
  for(i = 0; i < NUM_PMM_COREV_LVLS; i++) {
    IncrementVcore();
  }
  //Enter LPM0
  LPM0;
}
