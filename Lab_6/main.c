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

  /// I2C Config
   P3SEL |= BIT3 | BIT2;
   UCB0CTL0 |= UCMST | UCMODE_3;
   UCB0CTL1 |= UCSSEL__SMCLK;
   UCA0BR0 = 40; /// int(N/16). N= 16MHz/400k baudrate 
   UCA1BR1 = 0;
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
  while(!(UCB0IFG & UCTXIFG)); // wait until reg address got sent
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