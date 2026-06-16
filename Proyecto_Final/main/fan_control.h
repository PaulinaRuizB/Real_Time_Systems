#ifndef FAN_CONTROL_H
#define FAN_CONTROL_H

#include <stdint.h>

void fan_control_init(void);
void fan_control_set_percent(uint8_t percent);

#endif