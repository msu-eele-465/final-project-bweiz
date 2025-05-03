#include <msp430.h>
#include "../src/keypad.h"
#include "../src/i2c_master.h"
#include "../src/lcd.h"
#include <string.h>
#include <stdint.h>

// State definitions
#define STATE_MODE_SELECT 1
#define STATE_INPUT_PARAM 2

// Parameter IDs
#define PARAM_STOCK_PRICE 1
#define PARAM_STRIKE_PRICE 2
#define PARAM_TIME_EXP     3
#define PARAM_VOLATILITY   4
#define PARAM_RISK_FREE    5

volatile int state_variable = STATE_MODE_SELECT;
volatile int current_param = 0;
char keypad_input[6] = {0};  // buffer for user input
volatile int input_index = 0;
volatile int send_i2c_update_flag = 0;

volatile float stock_price = 100.0;
volatile float strike_price = 100.0;
volatile float time_to_exp = 0.5;
volatile float volatility = 0.2;
volatile float risk_free_rate = 0.05;
volatile float market_price = 0.0;

float norm_cdf(float x);
float black_scholes_call(float S, float K, float T, float r, float sigma);
void display_prompt_param(int param);
void display_result(float result);
void process_keypad(void);



volatile int pattern = -1; // Current pattern
volatile int step[4] = {0, 0, 0, 0}; // Current step in each pattern
volatile float base_tp = 0.5;    // Default 1.0s


const uint8_t pattern_0 = 0b10101010;
const int pattern_1[4] = {0b10101010, 0b10101010, 0b01010101, 0b01010101};  // Pattern 1
const int pattern_3[6] = {0b00011000, 0b00100100,   // Pattern 3
                          0b01000010, 0b10000001,
                          0b01000010, 0b00100100};

void setup_heartbeat() {
    // --    LED   --
    
    P6DIR |= BIT6;                                                      // P6.6 as OUTPUT
    P6OUT |= BIT6;                                                      // Start LED off

    // -- Timer B0 --
    TB0R = 0;
    TB0CCTL0 = CCIE;                                                    // Enable Interrupt
    TB0CCR0 = 32820;                                                    // 1 sec timer
    TB0EX0 = TBIDEX__8;                                                 // D8
    TB0CTL = TBSSEL__SMCLK | MC__UP | ID__4;                            // Small clock, Up counter,  D4
    TB0CCTL0 &= ~CCIFG;
}

/* void setup_ledbar_update_timer() {
    TB1CTL = TBSSEL__ACLK | MC__UP | ID__4;                             // Use ACLK, up mode, divider 4
    TB1CCR0 = (int)((32000 * base_tp) / 4.0);                           // Set update interval based on base_tp
    TB1CCTL0 = CCIE;                                                    // Enable interrupt for TB1 CCR0
} */

/* void rgb_timer_setup() {
    P3DIR |= (BIT2 | BIT7);                                 // Set as OUTPUTS
    P2DIR |= BIT4;
    P3OUT |= (BIT2 | BIT7);                                 // Start HIGH
    P2OUT |= BIT4;

    TB2R = 0;
    TB2CTL |= (TBSSEL__SMCLK | MC__UP);                     // Small clock, Up counter
    TB2CCR0 = 512;                                          // 1 sec timer
    TB2CCTL0 |= CCIE;                                       // Enable Interrupt
    TB2CCTL0 &= ~CCIFG;
} */

uint8_t compute_ledbar() {
    uint8_t led_pins = 0;
    if (pattern < 0) {
        led_pins = 0x00;
    }
    switch (pattern) {
        case 0:
            led_pins = pattern_0;
            break;
        case 1:
            led_pins = pattern_1[step[pattern]];
            step[pattern] = (step[pattern] + 1) % 4;
            break;
        case 2:
            led_pins = step[pattern];
            step[pattern] = (step[pattern] + 1) % 255;
            break;
        case 3:
            led_pins = pattern_3[step[pattern]]; // Pattern 3
            step[pattern] = (step[pattern] + 1) % 6; // advance to the next step
            break;
        default:
            break; 
    }
    return led_pins;
}

void change_led_pattern(int new_pattern) {
    if (new_pattern == pattern) {
        step[pattern] = 0;  // Just reset the step count if the same pattern is selected
    }

    pattern = new_pattern;
}

void update_slave_ledbar() {
    volatile int ledbar_pattern = compute_ledbar();
    while(i2c_busy);
    i2c_write_led(ledbar_pattern);
}

void process_keypad() {
    
    char key = pressed_key();
    if (key == '\0') return;
         if (state_variable == STATE_MODE_SELECT) {
            // --------------------------------------------
            // ------------ MODE SELECT -------------------
            // --------------------------------------------
            if (key != '\0') {                                                  
                if (key >= '1' && key <= '5') {
                    current_param = key - '0';
                    state_variable = STATE_INPUT_PARAM;
                    input_index = 0;
                    memset(keypad_input, 0, sizeof(keypad_input));
                    display_prompt_param(current_param);
                    }
                } else if (state_variable == 3) {
                    if(key >= '0' && key <= '9') {
                        uint8_t num = key - '0';
                        change_led_pattern(num);
                        state_variable = 1;
                        input_index = 0;
                        memset(keypad_input, 0, sizeof(keypad_input));
                    }   
                }            
        } else if (state_variable == STATE_INPUT_PARAM) {
            if (key == 'C') {  // Confirm
                float value = atof(keypad_input);
                switch (current_param) {
                    case PARAM_STOCK_PRICE:  stock_price    = value; break;
                    case PARAM_STRIKE_PRICE: strike_price   = value; break;
                    case PARAM_TIME_EXP:     time_to_exp    = value; break;
                    case PARAM_VOLATILITY:   volatility     = value; break;
                    case PARAM_RISK_FREE:    risk_free_rate = value; break;
                    default: break;
                }
                lcd_clear();
                lcd_puts("Saved!");
                __delay_cycles(1000000);
                // Return to main menu
                lcd_clear();
                lcd_puts("1:S 2:K 3:T");
                lcd_set_cursor(1,0);
                lcd_puts("4:V 5:r #:Go");
                state_variable = STATE_MODE_SELECT;
            }
        }
}

void display_prompt_param(int param) {
    lcd_clear();
    switch (param) {
        case PARAM_STOCK_PRICE:  lcd_puts("Set Stock Price:"); break;
        case PARAM_STRIKE_PRICE: lcd_puts("Set Strike Price:"); break;
        case PARAM_TIME_EXP:     lcd_puts("Set Time Exp (yr):"); break;
        case PARAM_VOLATILITY:   lcd_puts("Set Volatility:"); break;
        case PARAM_RISK_FREE:    lcd_puts("Set Risk-Free r:"); break;
        default:                 lcd_puts("Set Param:");     break;
    }
    lcd_set_cursor(1,0);
}

/* void process_flags(void) {
    if(send_i2c_update_flag && state_variable == 1) {
        update_slave_ledbar();
        send_i2c_update_flag = 0;
    }*/


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;
    i2c_master_setup();
    setup_keypad();
    setup_heartbeat();
    //setup_ledbar_update_timer();
    //rgb_timer_setup();

    //setup_ADC();
    setup_lcd();

        // Initial menu display
    lcd_clear();
    lcd_puts("1:S 2:K 3:T");
    lcd_set_cursor(1,0);
    lcd_puts("4:V 5:r #:Go");

    send_buff = 0;
    ready_to_send = 0;

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    UCB0CTLW0 &= ~UCSWRST;                // Take out of reset
    UCB0IE |= UCTXIE0;
    UCB0IE |= UCRXIE0;

    __enable_interrupt();

    while(1)
    {
        process_keypad();
/*         if (state_variable != 0 && state_variable != 2) {
            while(i2c_busy);
            process_flags();
        } */

    }
}

// ----------------------------------------------------------------------------------------------------------------------------------------
// ------------- INTERRUPTS ---------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------------------

#pragma vector=TIMER0_B0_VECTOR
__interrupt void Timer_B0_ISR(void) {
    TB0CCTL0 &= ~CCIFG;
    P6OUT ^= BIT6;
}

/* #pragma vector = TIMER1_B0_VECTOR
__interrupt void Timer_B1_ISR(void) {
    TB1CCTL0 &= ~CCIFG;
    send_i2c_update_flag = 1;
    TB1CCR0 = (int)((32768 * base_tp) / 4.0);
} */

/*  #pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_ISR(void){
    P1OUT |= BIT0;
    int current = UCB0IV;
    switch(current) {
        case 0x18:  // TXIFG
            UCB0TXBUF = send_buff;
            i2c_busy = 0;
            break;
        default:
            break;
    }
} */