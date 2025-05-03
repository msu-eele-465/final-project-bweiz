#include "msp430fr2355.h"
#include <msp430.h>
#include "i2c_master.h"
#include <stdint.h>

volatile int i2c_busy = 0;

void i2c_master_setup(void) {
    //-- eUSCI_B0 --
    UCB0CTLW0 |= UCSWRST;

    UCB0CTLW0 |= UCSSEL__SMCLK;              // SMCLK
    UCB0BRW = 10;                       // Divider

    UCB0CTLW0 |= UCMODE_3;              // I2C Mode
    UCB0CTLW0 |= UCMST;                 // Master
    UCB0CTLW0 |= UCTR;                  // Tx
    UCB0CTLW1 |= UCASTP_2;
    //-- Configure GPIO --------
    P1SEL1 &= ~BIT3;           // eUSCI_B0
    P1SEL0 |= BIT3;

    P1SEL1 &= ~BIT2;
    P1SEL0 |= BIT2;

    
}

 void update_LCD(int modeID, int temperature, int window_size) {
    while(i2c_busy);
    i2c_busy = 1;

    UCB0IE &= ~UCTXIE0;

    UCB0CTLW0 |= UCTR;  // Transmit mode
    UCB0I2CSA = 0x02;   // Slave address

    UCB0TBCNT = 4;

    UCB0IFG &= ~UCSTPIFG;

    uint8_t lcdData[4];
    lcdData[0] = (uint8_t) modeID;
    lcdData[1] = (int)((temperature >> 8) & 0xFF);
    lcdData[2] = (int)(temperature & 0xFF);
    lcdData[3] = (uint8_t) window_size;

    

    UCB0CTLW0 |= UCTXSTT;
    int i;
    for (i = 0; i < 4; i++) {
        while(!(UCB0IFG & UCTXIFG));
        UCB0TXBUF = lcdData[i];
    }

     unsigned int timeout = 1000;
    while (!(UCB0IFG & UCSTPIFG) && timeout--) {
        ;
    }   
    UCB0IFG &= ~UCSTPIFG;

    i2c_busy = 0;

    UCB0IE |= UCTXIE0;
}

 void i2c_write_led(unsigned int pattNum) {
    while(i2c_busy);
    i2c_busy = 1;

    UCB0CTLW0 |= UCTR;  // Transmit mode
    UCB0I2CSA = 0x40;   // Slave address

    UCB0TBCNT = 1;

    UCB0IFG &= ~UCSTPIFG;

    send_buff = pattNum;
    UCB0TXBUF = send_buff;

    UCB0CTLW0 |= UCTXSTT;

    UCB0IFG &= ~UCSTPIFG;

    i2c_busy = 0;

} 
 
