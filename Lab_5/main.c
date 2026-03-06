#include <msp430.h>

extern int IncrementVcore(void);

#define  NUM_PMM_COREV_LVLS 4
// Structure definition for the calibration data from flash
typedef struct {
  int thirtyfive_1_5;
  int eightyfive_1_5;
} tlv_structure_t;

unsigned int counter = 0;

// Point the structure to the place in memory where the calibration values are
volatile tlv_structure_t *tlv = (tlv_structure_t *) 0x1A1A;

// Values for converting ADC values to calibrated temperature values
int temp_m;
int temp_b;


// x2 - x1, difference in calibration tempratures, or run in slope
#define TEMP_DIFF 55
// Temperature of calibration value
#define TEMP_85 85


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
  
  
  //setting up timer a
  TA1CTL = TASSEL_1 + MC__UP + TACLR;  //timerA clock source select (aclk), count up(CCR0), timer clear
  TA1CCTL0 = CCIE;
  TA1CCR0 = 32768;

  //setting up reference module FOR adc
  REFCTL0 |= REFMSTR | REFVSEL_0 | REFOUT | REFON;
   
  //SET UP adc

  ADC12CTL0 |= ADC12SHT0_12 | ADC12ON | ADC12MSC; // Sample and hold for 1024 clks, turn on ADC
  ADC12MCTL0 |= ADC12INCH_10 | ADC12SREF1;
  ADC12MCTL1 |= ADC12INCH_10 | ADC12SREF1;
  ADC12MCTL2 |= ADC12INCH_10 | ADC12SREF1;
  ADC12MCTL3 |= ADC12INCH_10 | ADC12SREF1;
  ADC12MCTL4 |= ADC12INCH_10 | ADC12SREF1;
  ADC12MCTL5 |= ADC12INCH_10 | ADC12SREF1;
  ADC12MCTL6 |= ADC12INCH_10 | ADC12SREF1;
  ADC12MCTL7 |= ADC12INCH_10 | ADC12SREF1 | ADC12EOS; // Choose input for ADC as temp sensor, set reference voltage
  ADC12CTL1 |= ADC12SSEL_3 | ADC12DIV1 | ADC12SHP | ADC12CONSEQ_1; // use SMCLK, divide by 2, use sample and hold timer,sequence of channels (adc takes 100us,so 17Mhz/2=8.5 Mhz ... 1/8.5mhz = 118ns x 1024= 120 us > 100us)
  
  ADC12IE |= ADC12IE7; // Enable interrupts for the temperature sensor adc pin
  ADC12CTL0 |= ADC12ENC; // Enable conversion for ADC 8

   // Setup conversion factors for ADC values
  temp_m = ((float) tlv->eightyfive_1_5 - (float)tlv->thirtyfive_1_5) / (float) TEMP_DIFF;
  temp_b = (float) tlv->eightyfive_1_5 - ((float) temp_m * (float) TEMP_85);
 
  // Setting up interrupts for port 2
  P2DIR &= ~BIT6; // setting button to input on click
  P2IES |= BIT6;  // interrupt edge select
  P2IE |= BIT6;   // interrupt enable
  P2REN |= BIT6;  // enabling resistor
  P2OUT |= BIT6;  // setting to pullup resistor
  
    //Led/pin setup
  P1DIR |= BIT0;
  P10DIR |= BIT0;
  P1OUT |= BIT0; // led on


  // Set up UART
  P5SEL |= BIT6 | BIT7;
  UCA1CTL1 |= UCSWRST;
  UCA1CTL0 |= UCPEN | UCPAR; //aprity enable, even parity, 7bit
  UCA1CTL1 |= UCSSEL_2; // UART using SMCLK
  UCA1BR0 = 110;
  UCA1BR1 = 0;
  UCA1MCTL |= UCBRF_10 | UCBRS_6 | UCOS16;
  UCA1CTL1 &= ~UCSWRST;
  UCA1IE |= UCTXIE; // enable transmit interrupts 
  UCA1IFG &= ~UCTXIFG;
  //Enter LPM0, enable interrupts
  _EINT();
  LPM0;
}


void button_ISR(void)__interrupt[PORT2_VECTOR]{ 

ADC12CTL0 |= ADC12SC; // Start conversion
P2IV = 0; //clear interrupt flag
}


void timerA_ISR(void) __interrupt[TIMER1_A0_VECTOR] {
  counter = counter + 1;
  TA1CCTL0 &= ~(CCIFG); // Clear timer interrupt flag
}

unsigned int temp_sum = 0;
int i = 0;
void adc12_ISR(void) __interrupt[ADC12_VECTOR] {

  temp_sum+= ADC12MEM0; // Sample the value from the temperature sensor, clears interrupt flag
  temp_sum+= ADC12MEM1;
  temp_sum+= ADC12MEM2;
  temp_sum+= ADC12MEM3;
  temp_sum+= ADC12MEM4;
  temp_sum+= ADC12MEM5;
  temp_sum+= ADC12MEM6;
  temp_sum+= ADC12MEM7;
  UCA1IFG = UCTXIFG;
}
//            0         1         2         3         4         5
//            0123456789012345678901234567890123456789012345678901
char msg[] = "000. The temperature is 00 0C. Running time is 0:00\r\n"; 
unsigned int tx_count = 0;
unsigned int hun_pl;
unsigned int ten_pl;

void uart_tx_ISR(void) __interrupt[USCI_A1_VECTOR] { 
  switch(UCA1IV) {
    case 0: break;
    case 4: 
      temp_sum / 8;
      temp_sum = (temp_sum - temp_b) / temp_m;
      tx_count++;
      hun_pl = tx_count / 100;
      ten_pl = (tx_count - (hun_pl * 100))/10;
      msg[0] += hun_pl;
      msg[1] += ten_pl;
      msg[2] += (tx_count - (ten_pl* 10))- (hun_pl*100);
      
      msg[24] += temp_sum /10;
      msg[25] += temp_sum - ((temp_sum /10) * 10);
      msg[27] = 167;
      msg[47] += counter / 60;
      msg[49] += (counter / 10) % 6;
      msg[50] += (counter % 6);
      for(i = 0; i < sizeof(msg); i++) {
        while(!(UCA1IFG & UCTXIFG));
        UCA1TXBUF = msg[i];
      }
    default: break;
    }      
}
