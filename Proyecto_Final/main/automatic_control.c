#include "automatic_control.h"
#include "fan_control.h"

#include <stddef.h>

void automatic_control_update_fan(system_state_t *state)
{
    if (state == NULL)
    {
        return;
    }

    float temperature = state->current_temperature_c;
    float desired = state->desired_temperature_c;
    float maximum = state->maximum_temperature_c;

    uint8_t fan_percent = 0;

    if (temperature <= desired)
    {
        fan_percent = 0;
    }
    else if (temperature >= maximum)
    {
        fan_percent = 100;
    }
    else
    {
        float range = maximum - desired;
        float difference = temperature - desired;

        fan_percent = (uint8_t)((difference * 100.0f) / range);
    }

    state->fan_percent = fan_percent;
    fan_control_set_percent(fan_percent);
}