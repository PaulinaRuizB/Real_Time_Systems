#include <libraries.h>

// FUNCIÓN CONFIG PWM ambos funcionamientos de LEDS
void pwm_init()
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY
    };

    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .speed_mode = LEDC_MODE,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };

    ch.channel = LEDC_CHANNEL_R;
    ch.gpio_num = LED_R;
    ledc_channel_config(&ch);

    ch.channel = LEDC_CHANNEL_G;
    ch.gpio_num = LED_G;
    ledc_channel_config(&ch);

    ch.channel = LEDC_CHANNEL_B;
    ch.gpio_num = LED_B;
    ledc_channel_config(&ch);

    ch.channel = LEDC_CHANNEL_THRESHOLD;
    ch.gpio_num = LED_THRESHOLD;
    ledc_channel_config(&ch);
}

//Control RGB
void set_color(uint16_t r, uint16_t g, uint16_t b)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_R, r);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_R);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_G, g);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_G);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_B, b);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_B);
}