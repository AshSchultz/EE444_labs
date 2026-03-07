#include <msp430.h>

extern void IncrementVcore(void);

#define  NUM_PMM_COREV_LVLS 2
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
  ///clock config
  UCSCTL4 = SELA_0 | SELS__DCOCLK | SELM__DCOCLK; //Selects DCOCLK for MCLK and SMCLK and 0 for ACLK (XT1)
  UCSCTL2 = 518; //Divisor for DCO Creating 17 MHz for MCLK and SMCLK
 
  UCSCTL1 = DCORSEL_5; //Select tap 5 for DCO 17MHz. 
  
  
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
  UCA1CTL1 |= UCSWRST; // puts state machine in reset 
  UCA1CTL1 |= UCSSEL__SMCLK; // selects SMCLKC Source for BRCLK
  UCA1CTL0 |= UCPEN | UCPAR | UC7BIT ; //
  UCA1BR0 = 110; /// int(N/16). N= 17MHz/9600baudrate 
  UCA1BR1 = 0;
  UCA1MCTL |= UCOS16 | UCBRF_10 | UCBRS_6; //oversampling mode enabled   /// floor((N/16 - int(N/16)) *16) // UCBRS formula for low freq
  UCA1CTL1 &= ~UCSWRST;

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
//            0         1         2         3         4         5
//            0123456789012345678901234567890123456789012345678901
char msg[] = "000. The temperature is 00 0C. Running time is 0:00\r\n"; 
unsigned int tx_count = 0;
unsigned int hun_pl;
unsigned int ten_pl;
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
  UCA1IE |= UCTXIE; // enable transmit interrupts 
}

void uart_tx_ISR(void) __interrupt[USCI_A1_VECTOR] { 
  switch(UCA1IV) {
    case 0: break;
    case 4: 
        UCA1TXBUF = msg[i];
        i++;
        if( i > sizeof(msg)) {
          UCA1IE &= ~UCTXIE;
        }
    default: break;
    }      
}
