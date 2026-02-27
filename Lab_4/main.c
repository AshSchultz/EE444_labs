#include <msp430.h>

extern int IncrementVcore(void);

#define  NUM_PMM_COREV_LVLS 4


void main(void) {
  int i;
  //set core voltage to max
  for(i = 0; i < NUM_PMM_COREV_LVLS; i++) {
    IncrementVcore();
  }
  // Setting up DCO to 17mhz
  UCSCTL1 = DCORSEL_5;      // Setting clock to correct frequency range
  UCSCTL2 = 518;  // Clock divisor
  UCSCTL4 = SELM__DCOCLK | SELS__DCOCLK; //enabling master,  submaster
  UCSCTL8 &= ~(SMCLKREQEN);
  P11SEL |= BIT1 | BIT0 | BIT2;    //selecting special function for these pins (output main clock)
  P11DIR |= BIT1 | BIT0 | BIT2;    // setting direction to output
  // Set up UART
  P5SEL |= BIT6 | BIT7;
  UCA1CTL1 |= UCSWRST;
  UCA1CTL0 |= UCPEN | UCPAR | UC7BIT;
  UCA1CTL1 |= UCSSEL_2; // UART using SMCLK
  UCA1BR0 = 27;
  UCA1BR1 = 0;
  UCA1MCTL |= UCBRF_10 | UCBRS_5 | UCOS16;
  UCA1CTL1 &= ~UCSWRST;
  UCA1IE |= UCRXIE;

  //Enter LPM0, enable interrupts
  _EINT();
  LPM0;
}

char rx_byte;

#define  CHAR_DIFF 0x20
void uart_rx_ISR(void) __interrupt[USCI_A1_VECTOR] {
  switch(UCA1IV) {
    case 0: break;
    case 2:
        rx_byte = UCA1RXBUF;
        if ( (rx_byte <= 'z') && (rx_byte >= 'a') ) {
           rx_byte = rx_byte - CHAR_DIFF;
        }
      while(!(UCA1IFG&UCTXIFG));
        UCA1TXBUF = rx_byte;
    default: break;
    }      
}
