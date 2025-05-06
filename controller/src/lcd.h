#ifndef LCD_H
#define LCD_H

#include <msp430fr2355.h>
#include <stdbool.h>
#include "intrinsics.h"
#include <stdint.h>


extern volatile char button_pressed;
extern volatile int  curr_pattern;
extern volatile int  state;
extern int           cursor;
extern int           blink;
extern volatile char last_pressed;
extern volatile int  last_pattern;


void lcd_raw_send(int send_data, int num);
void lcd_string_write(char *string);
void update_pattern(char *string);
void update_key(char c);
void setup_lcd(void);
void lcd(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_putc(char c);
void lcd_puts(const char *str);

#endif // LCD_H