#include "intrinsics.h"
#include <msp430fr2355.h>
#include <stdbool.h>
#include <stdint.h>

volatile char button_pressed = ' ';
volatile int curr_pattern = -1;
volatile int state = 0;
int cursor = 0;
int blink = 0;

volatile char last_pressed;
volatile int last_pattern;

void lcd_raw_send(int send_data, int num) {
    int send_data_temp = send_data;
    int nibble, i = 0;
    int busy = 1;

    while (i < num) {
        if (num == 2) {
            nibble = send_data_temp & 0xF0;
            send_data_temp <<= 4;
        } else {
            nibble = (send_data_temp & 0x0F) << 4;
            send_data_temp >>= 4;
        }

        // map bits 7-4 of nibble to data pins P2.0-2.2,2.4
        uint8_t out = 0;
        if (nibble & 0x10) out |= BIT0;
        if (nibble & 0x20) out |= BIT1;
        if (nibble & 0x40) out |= BIT2;
        if (nibble & 0x80) out |= BIT4;
        P2OUT = (P2OUT & ~(BIT0|BIT1|BIT2|BIT4)) | out;

        // pulse enable
        P4OUT |= BIT7; _delay_cycles(1000);
        P4OUT &= ~BIT7; _delay_cycles(1000);
        i++;
    }
    _delay_cycles(50000);
}

void lcd_string_write(char* string) {
    // RS=1,RW=0
    P4OUT |= BIT4;
    P4OUT &= ~BIT6;
    int x = 0;
    for (x = 0; string[x]; ++x) {
        lcd_raw_send((int)string[x], 2);
    }
}

void update_pattern(char* string) {
    // RS=0,RW=0
    P4OUT &= ~BIT4;
    P4OUT &= ~BIT6;
    lcd_raw_send(0x02, 2);   // return home
    lcd_string_write(string);
}

void update_key(char c) {
    char s[2] = {c, '\0'};
    P4OUT &= ~BIT4;
    P4OUT &= ~BIT6;
    lcd_raw_send(0xCF, 2);   
    lcd_string_write(s);
}

void setup_lcd() {
    // data pins as output
    P2DIR |= (BIT0|BIT1|BIT2|BIT4);
    P2OUT &= ~(BIT0|BIT1|BIT2|BIT4);
    // control pins as output 
    P4DIR |= (BIT4|BIT6|BIT7);
    P4OUT &= ~(BIT4|BIT6|BIT7);
}

// Clear display
void lcd_clear(void) {
    P4OUT &= ~BIT4; // RS=0
    P4OUT &= ~BIT6; // RW=0
    lcd_raw_send(0x01, 2);
    __delay_cycles(200000);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t addr = (row == 0 ? 0x80 : 0xC0) + (col & 0x0F);
    P4OUT &= ~BIT4;
    P4OUT &= ~BIT6;
    lcd_raw_send(addr, 2);
}

void lcd_putc(char c) {
    P4OUT |= BIT4;
    P4OUT &= ~BIT6;
    lcd_raw_send((int)c, 2);
}

void lcd_puts(const char* s) {
    while (*s) lcd_putc(*s++);
}
