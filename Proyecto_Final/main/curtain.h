#ifndef CURTAIN_H
#define CURTAIN_H

#include <stdint.h>
#include "esp_log.h"
#define MAX_CURTAIN_SCHEDULES 8

typedef struct
{
    uint8_t hour;
    uint8_t minute;
    uint8_t position;
    bool enabled;

} curtain_schedule_t;

void curtain_init(void);

void curtain_set_position(uint8_t percent);

uint8_t curtain_get_position(void);

#endif