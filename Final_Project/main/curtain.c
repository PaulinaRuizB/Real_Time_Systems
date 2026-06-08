#include "curtain.h"
#include "servo.h"

static uint8_t current_position = 0;
static curtain_schedule_t schedules[MAX_CURTAIN_SCHEDULES];

//Programación manual de la cortina
void curtain_init(void)
{
    servo_init();
}

void curtain_set_position(uint8_t percent)
{
    if(percent > 100)
    {
        percent = 100;
    }

    current_position = percent;

    uint8_t angle =
        (percent * 180) / 100;

    servo_set_angle(angle);

    ESP_LOGI("CURTAIN", "Percent=%d",percent);
}

uint8_t curtain_get_position(void)
{
    return current_position;
}

//programación automática de la cortina 

void curtain_schedule_init(void)
{
    for(int i = 0; i < MAX_CURTAIN_SCHEDULES; i++)
    {
        schedules[i].hour = 0;
        schedules[i].minute = 0;
        schedules[i].position = 0;
        schedules[i].enabled = false;
    }
}

void curtain_schedule_set( uint8_t index, uint8_t hour, uint8_t minute, int8_t position)
{
    if(index >= MAX_CURTAIN_SCHEDULES)
    {
        return;
    }

    schedules[index].hour = hour;
    schedules[index].minute = minute;
    schedules[index].position = position;
    schedules[index].enabled = true;
}