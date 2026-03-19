#include <msp430.h>

extern int IncrementVcore(void);

#define  NUM_PMM_COREV_LVLS 2
// Structure definition for the calibration data from flash
typedef struct {
  unsigned int thirtyfive_1_5;
  unsigned int eightyfive_1_5;
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
  ///clock config
  UCSCTL4 = SELA_0 | SELS__DCOCLK | SELM__DCOCLK; //Selects DCOCLK for MCLK and SMCLK and 0 for ACLK (XT1)
  UCSCTL2 = 518; //Divisor for DCO Creating 17 MHz for MCLK and SMCLK
 
  UCSCTL1 = DCORSEL_5; //Select tap 5 for DCO 17MHz. 
  
  // Set up UART
  

  P5SEL |= BIT6 | BIT7;

  UCA1CTL1 |= UCSWRST; // puts state machine in reset 
  UCA1CTL1 |= UCSSEL__SMCLK; // selects SMCLKC Source for BRCLK
  UCA1CTL0 |= UCPEN | UCPAR; // 8 bit even parity
  UCA1BR0 = 110; /// int(N/16). N= 17MHz/9600baudrate 
  UCA1BR1 = 0;
  UCA1MCTL |= UCOS16 | UCBRF_10 | UCBRS_6; //oversampling mode enabled   /// floor((N/16 - int(N/16)) *16) // UCBRS formula for low freq
  UCA1CTL1 &= ~UCSWRST;
  
  //setting up timer a
  TA1CTL =  TASSEL__ACLK | MC__UP |TACLR; // selects ACLK for TimerA 1
  TA1CCTL0 = CCIE;
  TA1CCR0 = 32768;

  //setting up reference module FOR adc
  REFCTL0 |= REFMSTR | REFVSEL_0 | REFOUT | REFON;
   
  //SET UP adc
  ADC12CTL0 &= ~ADC12ENC; // Unlocks ADC12 config. 
  ADC12CTL0 |= ADC12ON; // Turns on the ADC core.
  ADC12CTL0 |= ADC12SHT0_10 | ADC12MSC; // Chooses 10 = 1010b which corresponds to 512 cycles giving us sample time of (a bit less) than 128us.
      
  ADC12CTL1 |= ADC12SSEL_3; // Selects 3 = 11b which selects SMCLK as source for ADC.
  ADC12CTL1 |= ADC12SHP;    // Selects sampling timer
  ADC12CTL1 |= ADC12DIV_3;  // Selects 3 = 011b corresponding to dividing the SMCLK by 4 ( around 4MHz) before being sent to ADC.  
  ADC12CTL1 |= ADC12CONSEQ_1;  //sequence of channels mode for consectuive sampling.

  ADC12CTL2 |= ADC12RES_2; // (around) 3.25 us  conversion time on 4MHz SMCLK.

  ADC12IE |= ADC12IE7; //enables interrupts on ADC12MEM7
 
  ADC12MCTL0 |= ADC12INCH_10 | ADC12SREF_1; // sets input channel to temperature diode
  ADC12MCTL1 |= ADC12INCH_10 | ADC12SREF_1; // sets input channel to temperature diode
  ADC12MCTL2 |= ADC12INCH_10 | ADC12SREF_1; // sets input channel to temperature diode
  ADC12MCTL3 |= ADC12INCH_10 | ADC12SREF_1; // sets input channel to temperature diode
  ADC12MCTL4 |= ADC12INCH_10 | ADC12SREF_1; // sets input channel to temperature diode
  ADC12MCTL5 |= ADC12INCH_10 | ADC12SREF_1; // sets input channel to temperature diode
  ADC12MCTL6 |= ADC12INCH_10 | ADC12SREF_1; // sets input channel to temperature diode  
  ADC12MCTL7 |= ADC12INCH_10 | ADC12SREF_1 | ADC12EOS; // sets input channel to temperature diode and sets MEM7 as end of sequence 
  
  ADC12CTL0 |= ADC12ENC;// Enables conversion
 


   // Setup conversion factors for ADC values
  temp_m = ((float) tlv->eightyfive_1_5 - (float) tlv->thirtyfive_1_5) / TEMP_DIFF;
  temp_b = (float) tlv->eightyfive_1_5 - ((float) temp_m * TEMP_85);
 
 
  // Setting up interrupts for port 2
  P2DIR &= ~BIT6; // setting button to input on click
  P2IES |= BIT6;  // interrupt edge select
  P2IE |= BIT6;   // interrupt enable
  P2REN |= BIT6;  // enabling resistor
  P2OUT |= BIT6;  // setting to pullup resistor
  
    //Led/pin setup
  P1DIR |= BIT0;
  P10OUT |= BIT0;
  //P10DIR |= BIT0; // <<<<<<<<<<<<-----------
  P1OUT |= BIT0; // led on

  //Enter LPM0, enable interrupts
  _EINT();
  LPM0;
}


void button_ISR(void)__interrupt[PORT2_VECTOR]{ 

ADC12CTL0 |= ADC12SC; // Start conversion
P2IV = 0; //clear interrupt flag
}

unsigned int ten_counter = 0;
unsigned int min_counter = 0;
void timerA_ISR(void) __interrupt[TIMER1_A0_VECTOR] {
  if (counter < 9) {
    counter++;
  } else {
    ten_counter++;
    counter = 0;
  }

  if (ten_counter >= 6) {
    ten_counter = 0;
    min_counter++;
  }

  TA1CCTL0 &= ~(CCIFG); // Clear timer interrupt flag
}

unsigned int temp_sum = 0;
//            0         1         2         3         4         5
//            0123456789012345678901234567890123456789012345678901
char msg[] = "000. The temperature is 00 dC. Running time is 0:00\r\n"; 
unsigned int tx_count_one = 0;
unsigned int tx_count_ten = 0;
unsigned int tx_count_hun = 0;
int temp_ave = 0;
int temp = 0;
int i = 0;
void adc12_ISR(void) __interrupt[ADC12_VECTOR] {

  temp_sum += (ADC12MEM0 + ADC12MEM1 + ADC12MEM2 + ADC12MEM3 + ADC12MEM4 + ADC12MEM5 + ADC12MEM6 + ADC12MEM7); // Sample the value from the temperature sensor, clears interrupt flag

  temp_ave = temp_sum / 8;
  temp = ((temp_ave - temp_b) / temp_m);
  
  if (tx_count_one < 9) {
    tx_count_one++;
  } else {
    tx_count_ten++;
    tx_count_one = 0;
  }
  if (tx_count_ten >= 9) {
    tx_count_ten = 0;
    tx_count_hun++;
  }
  msg[0] = tx_count_hun + '0';
  msg[1] = tx_count_ten + '0';
  msg[2] = tx_count_one + '0';
  
  msg[24] = (temp / 10) + '0';
  msg[25] = (temp % 10) + '0';
  msg[47] = min_counter + '0';
  msg[49] = ten_counter + '0';
  msg[50] = counter + '0';
  i = 0;
  temp_sum = 0;
  UCA1IE |= UCTXIE; // enable transmit interrupts 
}

void uart_tx_ISR(void) __interrupt[USCI_A1_VECTOR] { 
  switch (UCA1IV) {
    case 0: break;
    case 4: 
        UCA1TXBUF = msg[i];
        i++;
        if( i >= sizeof(msg)) {
          UCA1IE &= ~UCTXIE;
        }
    default: break;
    }      
}
