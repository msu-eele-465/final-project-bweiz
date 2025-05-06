#include <msp430.h>
#include "../src/keypad.h"
#include "../src/i2c_master.h"
#include "../src/lcd.h"
#include <string.h>
#include <stdint.h>
#include <math.h>

// State definitions
#define STATE_MODE_SELECT 1
#define STATE_INPUT_PARAM 2
#define STATE_DISPLAY_RESULT 3

// Parameter IDs
#define PARAM_STOCK_PRICE 1
#define PARAM_STRIKE_PRICE 2
#define PARAM_TIME_EXP     3
#define PARAM_VOLATILITY   4
#define PARAM_RISK_FREE    5
#define PARAM_MKT_PRICE    6

// Encoder step sizes
#define NUM_STEPS 4
const float step_values[NUM_STEPS] = {10.0f, 1.0f, 0.1f, 0.01f};
const char *step_labels[NUM_STEPS] = {"x10"," x1","0.1","0.01"};
volatile int step_idx = 1;
volatile float encoder_step = 1.0f;
volatile float edit_value = 0.0f;

volatile int state_variable = STATE_MODE_SELECT;
volatile int current_param = 0;
char keypad_input[6] = {0};  // buffer for user input
volatile int input_index = 0;
volatile int send_i2c_update_flag = 0;

volatile float stock_price = 85.43f;
volatile float strike_price = 105.0f;
volatile float time_to_exp = 0.12f;
volatile float volatility = 0.45f;
volatile float risk_free_rate = 0.05f;
volatile float market_price = 0.65f;

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

void setup_ledbar_update_timer() {
    TB1CTL = TBSSEL__ACLK | MC__UP | ID__4;                             // Use ACLK, up mode, divider 4
    TB1CCR0 = (int)((32000 * base_tp) / 4.0);                           // Set update interval based on base_tp
    TB1CCTL0 = CCIE;                                                    // Enable interrupt for TB1 CCR0
} 

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
         switch (state_variable) {
            case STATE_MODE_SELECT:
            // --------------------------------------------
            // ------------ MODE SELECT -------------------
            // --------------------------------------------
                if (key >= '1' && key <= '6') {
                    current_param = key - '0';
                    // Initialize edit_value
                    switch (current_param) {
                        case PARAM_STOCK_PRICE:  edit_value = stock_price;    break;
                        case PARAM_STRIKE_PRICE: edit_value = strike_price;   break;
                        case PARAM_TIME_EXP:     edit_value = time_to_exp;    break;
                        case PARAM_VOLATILITY:   edit_value = volatility;     break;
                        case PARAM_RISK_FREE:    edit_value = risk_free_rate; break;
                        case PARAM_MKT_PRICE:    edit_value = market_price;   break;
                    }
                    display_prompt_param(current_param);
                    show_edit_value();
                    state_variable = STATE_INPUT_PARAM;
                } else if (key == '#') {
                    state_variable = STATE_DISPLAY_RESULT;
                }
            break;
                // Show step label  
            case STATE_INPUT_PARAM:
                    if (key == '*') {
                        // Cycle step size
                        step_idx = (step_idx + 1) % NUM_STEPS;
                        encoder_step = step_values[step_idx];
                        lcd_set_cursor(1,13);
                        lcd_puts(step_labels[step_idx]);
                        lcd_set_cursor(1,0);
                        return;
                    }

                if (key == 'C') {
                    // Store confirmed value
                    switch (current_param) {
                        case PARAM_STOCK_PRICE:  stock_price    = edit_value; break;
                        case PARAM_STRIKE_PRICE: strike_price   = edit_value; break;
                        case PARAM_TIME_EXP:     time_to_exp    = edit_value; break;
                        case PARAM_VOLATILITY:   volatility     = edit_value; break;
                        case PARAM_RISK_FREE:    risk_free_rate = edit_value; break;
                        case PARAM_MKT_PRICE:    market_price = edit_value;   break;
                        default: break;
                    }
                    show_main_menu();
                    state_variable = STATE_MODE_SELECT;
                    
                }
                 else if(key == '#') {
                    state_variable = STATE_DISPLAY_RESULT;
                }
                break;
            

         case STATE_DISPLAY_RESULT: 
            __disable_interrupt();
             float result = black_scholes_call(
                stock_price, strike_price,
                time_to_exp, risk_free_rate,
                volatility
            ); 
            // compute percent difference vs market price
            float pct_diff = 0.0f;
            if (result != 0.0f) {
                pct_diff = (market_price - result) / result * 100.0f;
            }
            
            __enable_interrupt();

            lcd_clear();
            
            // format result xx.xx
            int pr = (int)(result * 100 + 0.5f);
            int w = pr / 100;
            int f = pr % 100;
            char res_str[8]; int idr = 0;
            if (w >= 100) {
                res_str[idr++] = '0' + (w / 100);
                res_str[idr++] = '0' + ((w / 10) % 10);
                res_str[idr++] = '0' + (w % 10);
            } else if (w >= 10) {
                res_str[idr++] = '0' + (w / 10);
                res_str[idr++] = '0' + (w % 10);
            } else {
                res_str[idr++] = '0';
                res_str[idr++] = '0' + w;
            }
            res_str[idr++] = '.';
            res_str[idr++] = '0' + (f / 10);
            res_str[idr++] = '0' + (f % 10);
            res_str[idr] = '\0';
            // format market price xx.xx
            int mp = (int)(market_price * 100 + 0.5f);
            int mw = mp / 100;
            int mf = mp % 100;
            char mkt_str[8]; int idm = 0;
            if (mw >= 100) {
                mkt_str[idm++] = '0' + (mw / 100);
                mkt_str[idm++] = '0' + ((mw / 10) % 10);
                mkt_str[idm++] = '0' + (mw % 10);
            } else if (mw >= 10) {
                mkt_str[idm++] = '0' + (mw / 10);
                mkt_str[idm++] = '0' + (mw % 10);
            } else {
                mkt_str[idm++] = '0';
                mkt_str[idm++] = '0' + mw;
            }
            mkt_str[idm++] = '.';
            mkt_str[idm++] = '0' + (mf / 10);
            mkt_str[idm++] = '0' + (mf % 10);
            mkt_str[idm] = '\0';
            // format percent diff with one decimal and % sign
            char pct_str[8]; int idp = 0;
            // sign
            pct_str[idp++] = (pct_diff >= 0.0f) ? '+' : '-';
            int pd = (int)(fabsf(pct_diff) * 10 + 0.5f); // one decimal
            int pd_w = pd / 10;
            int pd_f = pd % 10;
            if (pd_w >= 100) {
                pct_str[idp++] = '0' + (pd_w / 100);
                pct_str[idp++] = '0' + ((pd_w / 10) % 10);
                pct_str[idp++] = '0' + (pd_w % 10);
            } else if (pd_w >= 10) {
                pct_str[idp++] = '0' + (pd_w / 10);
                pct_str[idp++] = '0' + (pd_w % 10);
            } else {
                pct_str[idp++] = '0';
                pct_str[idp++] = '0' + pd_w;
            }
            pct_str[idp++] = '.';
            pct_str[idp++] = '0' + pd_f;
            pct_str[idp++] = '%';
            pct_str[idp] = '\0';

            // display first line: Call price
            lcd_set_cursor(0, 0);
            lcd_puts("Call:");
            lcd_puts(res_str);
            
            // display second line: Market price and % diff
            lcd_set_cursor(1, 0);
            lcd_puts("Mkt:");
            lcd_puts(mkt_str);
            lcd_puts(" ");
            lcd_puts(pct_str);
            
            while (!pressed_key()) {
                set_ledbar_percent(pct_diff);
            }
            show_main_menu();
            state_variable = STATE_MODE_SELECT;
        break;

    }
}

void show_main_menu() {
    lcd_clear();
    lcd_puts("1:S 2:K 3:T");
    lcd_set_cursor(1,0);
    lcd_puts("4:V 5:r 6:MP #:Go");
}
void display_prompt_param(int param) {
    lcd_clear();
    switch (param) {
        case PARAM_STOCK_PRICE:  lcd_puts("Set Stock Price:"); break;
        case PARAM_STRIKE_PRICE: lcd_puts("Set Strike Price:"); break;
        case PARAM_TIME_EXP:     lcd_puts("Set Time Exp (yr):"); break;
        case PARAM_VOLATILITY:   lcd_puts("Set Volatility:"); break;
        case PARAM_RISK_FREE:    lcd_puts("Set Risk-Free r:"); break;
        case PARAM_MKT_PRICE:    lcd_puts("Set MKT price"); break;
        default:                 lcd_puts("Set Param:");     break;
    }
    lcd_set_cursor(1,0);
}

float range_for(int param) {
    switch (param) {
      case 1:
      case 2:                   return 1000.0f;
      case 3:                   return   2.0f;
      case 4:                   return   1.0f;
      case 5:                   return   0.10f;
      case 6:                   return   100.0f;
      default:                  return   1.0f;
    }
}  

void show_edit_value(void) {
    int16_t delta = encoder_get_delta();
    if (delta) {
        __delay_cycles(20000);
        edit_value += delta * encoder_step;
        float r = range_for(current_param);
        if (edit_value < 0) edit_value = 0;
        if (edit_value > r) edit_value = r;

                // Compute integer and fractional parts
                int v = (int)(edit_value * 100 + 0.5f);
                int whole = v / 100;
                int frac  = v % 100;
                char s[7];
                int idx = 0;
                if (whole >= 100) {
                    s[idx++] = '0' + (whole / 100);
                    s[idx++] = '0' + ((whole / 10) % 10);
                    s[idx++] = '0' + (whole % 10);
                } else if (whole >= 10) {
                    s[idx++] = '0' + (whole / 10);
                    s[idx++] = '0' + (whole % 10);
                } else {
                    s[idx++] = '0';
                    s[idx++] = '0' + whole;
                }
                s[idx++] = '.';
                s[idx++] = '0' + (frac / 10);
                s[idx++] = '0' + (frac % 10);
                s[idx]   = '\0';
            lcd_set_cursor(1, 0);
            lcd_puts(s);
            lcd_puts("  ");  // overwrite extra digits
            
    }
}

void set_ledbar_percent(float pct) {
    if (pct < 0.0f) pct = -pct;
    if (pct > 100.0f) pct = 100.0f;
     
    int bars = pct;         
    if (pct > 0.0f && bars == 0) // make sure ANY non‑zero percent lights at least 1
        bars = 1;
    if (bars > 8)                // cap at 8 bars
        bars = 8;

    int mask = 0;
    if (bars > 0) {
        mask = (int)(((1u << bars) -1 ) << (8 - bars));
    }
    while (i2c_busy);
    i2c_write_led(mask);
}


float norm_cdf(float x) {           // Cumulative normal function approximation
    return 0.5f * (1.0f + erff(x / sqrtf(2.0f)));
}

float black_scholes_call(float S, float K, float T, float r, float sigma) {

    float sqrtT = sqrtf(T);

    float d1 = (logf(S / K) + (r + 0.5f * sigma * sigma) * T) / (sigma * sqrtT);
    float d2 = d1 - sigma * sqrtT;
    return S * norm_cdf(d1) - K * expf(-r * T) * norm_cdf(d2);
}

int main(void)
{
    
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer


    P1DIR |= BIT0;
    P1OUT &= ~BIT0;
    i2c_master_setup();
    setup_keypad();
    setup_heartbeat();
    setup_encoder();

    setup_lcd();


        // Initial menu display
    show_main_menu();

    send_buff = 0;
    ready_to_send = 0;

    
                                            // to activate previously configured port settings
    UCB0CTLW0 &= ~UCSWRST;                // Take out of reset
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode    

    UCB0IE |= UCTXIE0;
    UCB0IE |= UCRXIE0;

    __enable_interrupt();

    while(1)
    {
        if (state_variable == STATE_INPUT_PARAM) {
            show_edit_value();  // live update while turning
        }
        process_keypad();

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

#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_ISR(void){
    int current = UCB0IV;
    switch(current) {
        case 0x18:  // TXIFG
            UCB0TXBUF = send_buff;
            i2c_busy = 0;
            break;
        default:
            break;
    }
} 


