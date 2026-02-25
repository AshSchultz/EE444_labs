
#include <msp430.h>

extern int IncrementVcore(void);

// Structure definition for the calibration data from flash
typedef struct {
  int thirtyfive_1_5;
  int eightyfive_1_5;
} tlv_structure_t;

// Point the structure to the place in memory where the calibration values are
volatile tlv_structure_t *tlv = (tlv_structure_t *) 0x1A1A;

// Values for converting ADC values to calibrated temperature values
float temp_m;
float temp_b;

// Size of circular buffer
#define CIRCULAR_BUFFER_SIZE 255
// x2 - x1, difference in calibration tempratures, or run in slope
#define TEMP_DIFF 55
// Temperature of calibration value
#define TEMP_85 85
#define  NUM_PMM_COREV_LVLS 4

void main(void) {
  int i;
  // Setting up DCO to 16mhz
  UCSCTL1 = DCORSEL_5;      // Setting clock to correct frequency range
  UCSCTL2 = 487;  // Clock divisor
  UCSCTL4 = SELM__DCOCLK | SELS__DCOCLK; //enabling master,  submaster
  UCSCTL8 &= ~(SMCLKREQEN);
  P11SEL |= BIT1 | BIT0 | BIT2;    //selecting special function for these pins (output main clock)
  P11DIR |= BIT1 | BIT0 | BIT2;    // setting direction to output
  // Set up Ref module
  REFCTL0 |= REFMSTR | REFVSEL_0 | REFOUT | REFON;
  //Led/pin setup
  P1DIR |= BIT0;
  P10DIR |= BIT0;
  // Timer setup
  TA1CTL = TASSEL_1 + MC__UP + TACLR;  //timerA clock source select (aclk), count up/down (CCR0), timer clear
  TA1CCTL0 = CCIE;
  TA1CCR0 = 32768;
  // ADC setup
  ADC12CTL0 |= ADC12SHT0_12 | ADC12ON; // Sample and hold for 1024 clks, turn on ADC
  ADC12MCTL0 |= ADC12INCH_10 | ADC12SREF1; // Choose input for ADC as temp sensor, set reference voltage
  ADC12CTL1 |= ADC12SSEL_3 | ADC12DIV1 | ADC12SHP; // use SMCLK, divide by 2, use sample and hold timer
  ADC12IE |= ADC12IE0; // Enable interrupts for the temperature sensor adc pin
  ADC12CTL0 |= ADC12ENC; // Enable conversion for ADC
  // Setting up interrupts for port 2
  P2DIR &= ~BIT6; // setting button to input on click
  P2IES |= BIT6;  // interrupt edge select
  P2IE |= BIT6;   // interrupt enable
  P2REN |= BIT6;  // enabling resistor
  P2OUT |= BIT6;  // setting to pullup resistor
  
  // Setup conversion factors for ADC values
  temp_m = ((float) tlv->eighty_1_5 - (float)tlv->thirty_1_5) / (float) TEMP_DIFF;
  temp_b = (float) tlv->eighty_1_5 - ((float) temp_m * (float) TEMP_85);

  //set core voltage to max
  for(i = 0; i < NUM_PMM_COREV_LVLS; i++) {
    IncrementVcore();
  }

  //Enter LPM0, enable interrupts
  _EINT();
  LPM0;
}

void timerA_ISR(void) __interrupt[TIMER1_A0_VECTOR] {
  P1OUT ^= BIT0; // led on
  P10OUT |= BIT0; //send trigger on to oscilloscope
  ADC12CTL0 |= ADC12SC; // Start conversion
  TA1CCTL0 &= ~(CCIFG); // Clear timer interrupt flag
}

int adc_val;
float circ_buff[256] = {0};
int i = 0;
void adc12_ISR(void) __interrupt[ADC12_VECTOR] {
  P10OUT &= ~(BIT0); // Send trigger to oscilloscope
  adc_val = ADC12MEM0; // Sample the value from the temperature sensor, clears interrupt flag
  circ_buff[i] = ((float) adc_val - temp_b) / temp_m; // convert adc value and store in circ_buff
  i++; // Move circular buffer pointer
  if ( i > CIRCULAR_BUFFER_SIZE ) { // Reset to start of buffer if full
    i = 0;
  }
}

// button interrupt for debugging
void button_ISR(void) __interrupt[PORT2_VECTOR] { // interrupt service routine handler port 2
  P2IV = 0; //clear interrupt flag
}
