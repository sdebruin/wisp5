#include <msp430.h> 
#include <stdint.h>

#define RX_PIN		BIT1
#define TX_PIN		BIT0

#define LED1_PIN	BIT7
#define LED2_PIN	BIT2
#define LED3_PIN	BIT6
#define LED1_DIR	P1DIR
#define LED2_DIR	P2DIR
#define LED3_DIR	P1DIR
#define LED1_OUT	P1OUT
#define LED2_OUT	P2OUT
#define LED3_OUT	P1OUT

typedef enum {
	idle,
	preamble_data,
	preamble_RT,
	preamble_TR_high,
	preamble_TR_low
} mode_t;

mode_t RFID_mode;

uint16_t RT_start;
uint16_t timePeriod;
uint16_t pulseWidth;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    CSCTL0_H = 0xA5;
    CSCTL1 |= DCORSEL + DCOFSEL0 + DCOFSEL1;   // Set max. DCO setting
    CSCTL2 = SELA_3 + SELS_3 + SELM_3;        // set ACLK = MCLK = DCO
    CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;        // set all dividers to 0

    // Set up LEDs as outputs
    LED1_DIR |= LED1_PIN;
    LED2_DIR |= LED2_PIN;
    LED3_DIR |= LED3_PIN;

    // Set up RX input
    P2DIR &= ~RX_PIN;	// Set RX to input
    P2SEL0 |= RX_PIN;	// Set to TB0.CCR0A
    P2SEL1 |= RX_PIN;

    // Set up TX output
    P2DIR |= TX_PIN;
    P2OUT &= ~TX_PIN;

    // Set up debug output
    P1DIR |= BIT3;

    // Set up timer capture
    TB0CCTL0 = CM_1 + CCIS_0 + CAP;  	// Capture on rising edge for preamble
    TB0CCTL0 &= ~CCIFG;
    TB0CTL = TBSSEL_2 + MC_2 + TBCLR;
    TB0CCTL0 |= CCIE;

    // Initialize mode
    RFID_mode = idle;

    __enable_interrupt();

    while(1) {
    	volatile unsigned int i = 0;

    	LED1_OUT ^= LED1_PIN;

    	for(i = 10000; i > 0; i--);
    }
	
	return 0;
}

#pragma vector=TIMERB0_VECTOR
__interrupt void TIMER_B (void) {

	// Clear interrupt
	TB0CCTL0 &= ~CCIFG;

	// Indicate RX
	LED2_OUT ^= LED2_PIN;
	P1OUT |= BIT3;

	switch(RFID_mode){
	case idle:
		RFID_mode = preamble_data;
		RT_start = TB0CCR0;
		P1OUT &= ~BIT3;
		break;
	case preamble_data:
		RFID_mode = preamble_RT;
		timePeriod = TB0CCR0 - RT_start;
		pulseWidth = timePeriod >> 1;
		P1OUT &= ~BIT3;
		break;
	case preamble_RT:
		RFID_mode = preamble_TR_high;

		// Set compare interrupt to drive high when we should drive TX high
		TB0CCTL0 &= ~CAP;
		TB0CCR0 += timePeriod + timePeriod + pulseWidth;
		TB0CCTL0 = CCIE;

		P1OUT &= ~BIT3;
		P1OUT &= ~BIT3;

		break;
	case preamble_TR_high:

		P2OUT |= TX_PIN;
		P2OUT |= TX_PIN;

		RFID_mode = preamble_TR_low;

		// Set compare interrupt to drive low when we should drive TX low
		TB0CCR0 += pulseWidth;
		TB0CCTL0 = CCIE;

		P1OUT &= ~BIT3;
		P1OUT &= ~BIT3;

		break;
	case preamble_TR_low:

		P2OUT &= ~TX_PIN;
		P2OUT &= ~TX_PIN;

		RFID_mode = idle;

		// Return capture to normal operation
	    TB0CCTL0 = CM_1 + CCIS_0 + CAP;  	// Capture on rising edge for preamble
	    TB0CCTL0 &= ~CCIFG;
	    TB0CCTL0 |= CCIE;

		P1OUT &= ~BIT3;
		P1OUT &= ~BIT3;

		break;
	}
}
