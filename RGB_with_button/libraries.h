#ifndef LIBRARIES_H
#define LIBRARIES_H

#include "driver/ledc.h"
#include <stdio.h>
#include <stdint.h>
#include "driver/gpio.h"

//estructura para las características base de los RGB

typedef struct{    
    uint32_t duty;    
    gpio_num_t gpio_num;
    ledc_channel_t channel;    
    
} led_t;

//estructura para la configuración de los LEDS
typedef struct{
    led_t led_red;
    led_t led_green;
    led_t led_blue;
    ledc_timer_t timer;
    ledc_timer_bit_t duty_resolution;
    uint32_t frequency;
    ledc_mode_t speed_mode;
} led_rgb_t;

//estructura configuración de los GPIO

typedef struct{
    gpio_num_t gpio_num;
    gpio_int_type_t intr_type;
    gpio_mode_t mode;
    gpio_pullup_t pull_up_en;
    gpio_pulldown_t pull_down_en;
} button_config_t;

void ledc_init(led_rgb_t *led_rgb);
void set_duty_rgb (led_rgb_t *led_rgb, uint32_t duty_red, uint32_t duty_green, uint32_t duty_blue);
void increment_duty_rgb(led_rgb_t *led_rgb, uint8_t color, uint8_t increment_percent);
void button_init(button_config_t *button_gpio, gpio_isr_t handler);

#endif // LIBRARIES_H
