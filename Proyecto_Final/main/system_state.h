#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    FAN_MODE_MANUAL = 0,
    FAN_MODE_AUTOMATIC
} fan_mode_t;

typedef struct
{
    float current_temperature_c;
    float current_light_lux;

    fan_mode_t fan_mode;
    uint8_t fan_percent;

    float desired_temperature_c;
    float maximum_temperature_c;

    bool alarm_active;

    uint8_t curtain_position_percent;

    uint8_t rgb_red;
    uint8_t rgb_green;
    uint8_t rgb_blue;
    uint8_t rgb_brightness;

} system_state_t;

#endif