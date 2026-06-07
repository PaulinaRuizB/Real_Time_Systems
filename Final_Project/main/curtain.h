#ifndef CURTAIN_H
#define CURTAIN_H

#include <stdint.h>

void curtain_init(void);

void curtain_set_position(uint8_t percent);

uint8_t curtain_get_position(void);

#endif