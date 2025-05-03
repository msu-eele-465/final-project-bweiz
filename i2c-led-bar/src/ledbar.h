#ifndef LEDBAR_H
#define LEDBAR_H

#include <stdint.h>



void setup_ledbar(void);
void ledbar_i2c_slave_setup(void);
void update_ledbar_pins(unsigned int pins);
volatile int idle_count;
#endif