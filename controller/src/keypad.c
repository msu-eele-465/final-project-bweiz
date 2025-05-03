#include "intrinsics.h"
#include <msp430.h>
#include <stdbool.h>
#include <msp430fr2355.h>
#include "keypad.h"

char code[] = "5381";



const unsigned rowPins[4] = {BIT4, BIT5, BIT6, BIT7};
const unsigned colPins[4] = {BIT0, BIT1, BIT2, BIT3};

const char keypad[4][4] = {                                 // Matrix rep. of keypad for pressedKey function
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};

void setup_keypad() {
    P1DIR |= (BIT4 | BIT5 | BIT6 | BIT7);        // rows = OUTPUT
    P6DIR &= ~(BIT0 | BIT1 | BIT2 | BIT3);              // cols = INPUT
    P6REN |= (BIT0 | BIT1 | BIT2 | BIT3);               // Pulldown resistors on cols
    P6OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3); 
    P1OUT &= ~(BIT4 | BIT5 | BIT6 | BIT7);              // rows low
}

char pressed_key() {
    int row, col;
    for (row = 0; row < 4; row++) {
        P1OUT &= ~(BIT4 | BIT5 | BIT6 | BIT7);              // Set rows low
        P1OUT |= rowPins[row];                              // current row high

        for(col = 0; col < 4; col++) {                      // Check each column for high
            __delay_cycles(1000);                       
            if((P6IN & colPins[col]) != 0) {                // If column high
                __delay_cycles(1000);                       // Debounce delay
                if((P6IN & colPins[col]) != 0) {            // Check again
                char keyP = keypad[row][col];
                
                while((P6IN & colPins[col]) != 0);          // Wait until key not pressed
                
                return keyP;                                // Update key
                }
            }
        }
    }
    return '\0';                                            // No key entered
}

void check_key() {
    int i, flag = 0;
    if (input_index == 3) {                                 // Only check after 4 digits entered

        for(i=0; i<3; i++) {
            if(keypad_input[i] != code[i]) {
                flag = 1;
            }
        }
        if(flag == 0){                                      // Code is correct
            state_variable = 1;
            input_index = 0;
            memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
        } else {
            state_variable = 0;
            input_index = 0;
            memset(keypad_input, 0, sizeof(keypad_input));  // Clear input
        }
            
            
    }
    
}