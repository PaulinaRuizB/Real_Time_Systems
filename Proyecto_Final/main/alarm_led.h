#ifndef ALARM_LED_H
#define ALARM_LED_H

#include <stdbool.h>

void alarm_led_init(void);
void alarm_led_set(bool enabled);
void alarm_led_toggle(void);

#endif