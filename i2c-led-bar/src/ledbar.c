#include <msp430fr2311.h>
#include <stdint.h>

volatile uint8_t pattern = -1; // Current pattern
volatile uint8_t step[4] = {0, 0, 0, 0}; // Current step in each pattern
volatile float base_tp = 0.5;    // Default 1.0s
//volatile uint16_t base_tp = 4096;



// Setup the timer for the ledbar
void setup_ledbar() {
    P1DIR |= (BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7); // Setup all the pins
    P2DIR |= (BIT0 | BIT6);
    P1OUT &= ~(BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7); // Setup all the pins
    P2OUT &= ~(BIT0 | BIT6);

}

void update_ledbar_pins(int pins) {
    int current_pins = pins;

    if ((current_pins & 0b00000001) == 1) {
        P1OUT |= BIT0;
    } else {
        P1OUT &= ~BIT0;
    }
    current_pins = current_pins >> 1;

    if ((current_pins & 0b00000001) == 1) {
        P1OUT |= BIT1;
    } else {
        P1OUT &= ~BIT1;
    }
    current_pins = current_pins >> 1;

    if ((current_pins & 0b00000001) == 1) {
        P1OUT |= BIT4;
    } else {
        P1OUT &= ~BIT4;
    }
    current_pins = current_pins >> 1;

    if ((current_pins & 0b00000001) == 1) {
        P1OUT |= BIT5;
    } else {
        P1OUT &= ~BIT5;
    }
    current_pins = current_pins >> 1;

    if ((current_pins & 0b00000001) == 1) {
        P1OUT |= BIT6;
    } else {
        P1OUT &= ~BIT6;
    }
    current_pins = current_pins >> 1;

    if ((current_pins & 0b00000001) == 1) {
        P1OUT |= BIT7;
    } else {
        P1OUT &= ~BIT7;
    }
    current_pins = current_pins >> 1;

    if ((current_pins & 0b00000001) == 1) {
        P2OUT |= BIT0;
    } else {
        P2OUT &= ~BIT0;
    }
    current_pins = current_pins >> 1;

    if ((current_pins & 0b00000001) == 1) {
        P2OUT |= BIT6;
    } else {
        P2OUT &= ~BIT6;
    }

}