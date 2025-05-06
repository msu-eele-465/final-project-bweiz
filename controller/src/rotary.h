#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

void setup_encoder(void);
int16_t encoder_get_delta(void);

#endif // ENCODER_H
