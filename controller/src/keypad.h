#ifndef KEYPAD_H
#define KEYPAD_H

void setup_keypad(void);
char pressed_key(void);
void check_key(void);

extern volatile int input_index;
extern volatile int state_variable;
extern char keypad_input[4];


#endif