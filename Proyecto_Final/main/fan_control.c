#include "fan_control.h"
#include "project_pins.h"

#include "driver/ledc.h"
#include "esp_err.h"

#define FAN_LEDC_TIMER      LEDC_TIMER_2
#define FAN_LEDC_CHANNEL    LEDC_CHANNEL_4
#define FAN_PWM_FREQ_HZ     25000
#define FAN_PWM_RESOLUTION  LEDC_TIMER_10_BIT
#define FAN_MAX_DUTY        1023

void fan_control_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = FAN_LEDC_TIMER,
        .duty_resolution = FAN_PWM_RESOLUTION,
        .freq_hz = FAN_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t channel_cfg = {
        .gpio_num = PIN_FAN_PWM,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = FAN_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = FAN_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));
}

void fan_control_set_percent(uint8_t percent)
{
    if (percent > 100)
    {
        percent = 100;
    }

    uint32_t duty = (FAN_MAX_DUTY * percent) / 100;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, FAN_LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, FAN_LEDC_CHANNEL);
}