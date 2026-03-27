#include <msp430.h>

extern int IncrementVcore(void);

#define  NUM_PMM_COREV_LVLS 2

void main(void) {
  int i;
  //set core voltage to max
  for(i = 0; i < NUM_PMM_COREV_LVLS; i++) {
    IncrementVcore();
  }

  //  Setting up DCO to 16mhz
  /// Clock config
  UCSCTL4 = SELA_0 | SELS__DCOCLK | SELM__DCOCLK; //Selects DCOCLK for MCLK and SMCLK and 0 for ACLK (XT1)
  UCSCTL2 = 487; //Divisor for DCO Creating 16 MHz for MCLK and SMCLK
 
  UCSCTL1 = DCORSEL_5; //Select tap 5 for DCO 17MHz.
  P11SEL |= BIT1 | BIT0 | BIT2;    //selecting special function for these pins (output main clock)
  P11DIR |= BIT1 | BIT0 | BIT2;    // setting direction to output 
  
  //setting up timer a
  TA1CTL =  TASSEL__ACLK | MC__UP |TACLR; // selects ACLK for TimerA 1
  TA1CCTL0 = CCIE;
  TA1CCR0 = 32768;

  /// I2C Config
   P3SEL |= BIT1 | BIT2;
   P3REN |= BIT1 | BIT2;
   P3DIR |= BIT1 | BIT2; // Output direction
   P3OUT |= BIT1 | BIT2; // Set as output
   P1DIR |= BIT7; // Set GPIO pin for CS
   P1OUT |= BIT7; // Set GPIO pin for CS
   UCB0CTL1 |= UCSWRST;
   UCB0CTL0 |= UCMST | UCMODE_3 | UCSYNC;
   UCB0CTL1 |= UCSSEL__SMCLK;
   UCB0BR0 = 80; /// int(N/16). N= 16MHz/400k baudrate 
   UCB0BR1 = 0;
   UCB0CTL1 &= ~(UCSWRST);
  
  // Enter LPM0, enable interrupts
  _EINT();
  LPM0;
}

char readByte(char slave_add, char register_add){
  char rx_byte;

  UCB0I2CSA = slave_add; // send slave address
  UCB0CTL1 |= UCTXSTT + UCTR; // generate START + I2C transmit (write)
  UCB0TXBUF = register_add; // write register address
//  while(!(UCB0IFG & UCTXIFG)); // wait until reg address got sent
  while( UCB0CTL1 & UCTXSTT); // wait till START condition is cleared
  UCB0CTL1 |= UCTXSTT; // generate RE-START
  UCB0CTL1 &=~ UCTR; // receive mode
  UCB0I2CSA = slave_add; // slave address again
  while( UCB0CTL1 & UCTXSTT); // wait till START condition is cleared
  rx_byte = UCB0RXBUF; // read byte
  UCB0CTL1 |= UCTXSTP; // generate stop condition
  while(UCB0CTL1 & UCTXSTP); // wait for stop condition to be sent
  return rx_byte;
}

void writeByte(char slave_add, char register_add, char write_val){
  char rx_byte;

  UCB0I2CSA = slave_add; // send slave address
  UCB0CTL1 |= UCTXSTT + UCTR; // generate START + I2C transmit (write)
  UCB0TXBUF = register_add; // write register address
  while(!(UCB0IFG & UCTXIFG)); // wait until reg address got sent
  while( UCB0CTL1 & UCTXSTT); // wait till START condition is cleared
  UCB0TXBUF = write_val;
  while(!(UCB0IFG & UCTXIFG)); // wait until reg address got sent
  UCB0CTL1 |= UCTXSTP;
}

char byte_read = 0;

void timerA_ISR(void) __interrupt[TIMER1_A0_VECTOR] {

  byte_read = readByte(24, 0x20);
  byte_read = readByte(24, 0x20);
  writeByte(24, 0x20, 0x27);
  byte_read = readByte(24, 0x20);
  byte_read = readByte(24, 0x20);
  TA1CCTL0 &= ~(CCIFG); // Clear timer interrupt flag
}