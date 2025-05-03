#include <msp430fr2311.h>
#include "ledbar.h"
#include <stdint.h>

void ledbar_i2c_slave_setup() {




    UCB0CTLW0 |= UCSWRST;       // Software Reset
    UCB0CTLW0 = UCSWRST | UCMODE_3 | UCSYNC;  // Input clock
    //UCB0CTLW0 = UCMODE_3;
    //UCB0CTLW0 |= UCSYNC;
    //UCB0CTLW0 &= ~UCMST;
    UCB0I2COA0 = 0x40 | UCOAEN;
    
            // Configure pins for I2C
    P1SEL1 &= ~(BIT2 | BIT3);
    P1SEL0 |= (BIT2 | BIT3);

    UCB0CTLW0 &= ~UCSWRST;



    //UCB0CTLW1 = 0;
    //UCB0CTLW0 &= ~UCTR;




    UCB0IE |= (UCRXIE0 | UCSTPIE);
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_ISR(void) {
    int led_data = 0;
    int current = UCB0IV;
    switch(current) {
        case 0x08:
            break;
        case 0x16:
            led_data = UCB0RXBUF;
            update_ledbar_pins(led_data);
            P2OUT |= BIT7;
            idle_count=0;
            break;
    }

    
}