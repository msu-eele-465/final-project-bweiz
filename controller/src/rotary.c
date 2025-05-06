#include <msp430.h>
#include "../src/keypad.h"
#include "../src/i2c_master.h"
#include "../src/lcd.h"
#include <string.h>
#include <stdint.h>

// --- Rotary encoder driver (on P5.0/A, P5.1/B)
static volatile int16_t count = 0;
static volatile uint8_t last  = 0;

void setup_encoder(void) {
    // P3.4/P3.5 are GPIO
    P3SEL0 &= ~(BIT4|BIT5);
    P3SEL1 &= ~(BIT4|BIT5);
    // Inputs with pull‑ups
    P3DIR   &= ~(BIT4|BIT5);
    P3REN   |=  (BIT4|BIT5);
    P3OUT   |=  (BIT4|BIT5);
    // Enable interrupts on both edges
    P3IES   &= ~(BIT4|BIT5);
    P3IE    |=  (BIT4|BIT5);
    P3IFG   &= ~(BIT4|BIT5);
    // Read initial state
    last = (P3IN >> 4) & 0x03;
}

int16_t encoder_get_delta(void) {
    int16_t d = count;
    count = 0;
    return d;
}

#pragma vector=PORT3_VECTOR
__interrupt void PORT3_ISR(void) {
    // Debug LED toggle
    P1OUT ^= BIT0;

    uint8_t triggered = P3IFG & (BIT4|BIT5);
    if (!triggered) return;
    P3IFG &= ~triggered;

    uint8_t s   = (P3IN >> 4) & 0x03;
    uint8_t idx = (last << 2) | s;
    switch (idx) {
      case 0b0001: case 0b0111:
      case 0b1110: case 0b1000:
        count++; break;
      case 0b0010: case 0b1011:
      case 0b1101: case 0b0100:
        count--; break;
      default: break;
    }
    last = s;

    if (triggered & BIT4) P3IES ^= BIT4;
    if (triggered & BIT5) P3IES ^= BIT5;
}
