#include "alarm_led.h"
#include "project_pins.h"

#include "driver/gpio.h"

static bool alarm_led_state = false;

void alarm_led_init(void)
{
    gpio_reset_pin(PIN_ALARM_LED);
    gpio_set_direction(PIN_ALARM_LED, GPIO_MODE_OUTPUT);

    // LED ánodo común:
    // 1 = apagado
    // 0 = encendido
    gpio_set_level(PIN_ALARM_LED, 1);

    alarm_led_state = false;
}

void alarm_led_set(bool enabled)
{
    alarm_led_state = enabled;

    if (enabled)
    {
        gpio_set_level(PIN_ALARM_LED, 0);
    }
    else
    {
        gpio_set_level(PIN_ALARM_LED, 1);
    }
}

void alarm_led_toggle(void)
{
    alarm_led_state = !alarm_led_state;
    alarm_led_set(alarm_led_state);
}