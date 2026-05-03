
#include "libraries.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

void button_init(button_config_t *button_gpio, gpio_isr_t handler){
    gpio_install_isr_service(0);  // Instalar el servicio de ISR
    
    for (int i = 0; i < 3; i++) {
        gpio_config_t button = {
            .pin_bit_mask = 1ULL << button_gpio[i].gpio_num,
            .intr_type = GPIO_INTR_NEGEDGE,  // Interrupción en el flanco negativo (cuando se presiona el botón, va de HIGH a LOW)
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE
        };
        gpio_config(&button);  // Configura el GPIO
        gpio_isr_handler_add(button_gpio[i].gpio_num, handler, (void*)(intptr_t) i);  // i es el botón (0, 1, 2)
    }
}

void ledc_init(led_rgb_t *led_rgb){

    //configuración del timer general para los canales de LEDC
    ledc_timer_config_t timer_config = {
        .duty_resolution = led_rgb->duty_resolution,
        .freq_hz = led_rgb->frequency,
        .speed_mode = led_rgb->speed_mode,
        .timer_num = led_rgb->timer,
        .clk_cfg   = LEDC_AUTO_CLK
    };

    //configuración de cada canal de LEDC para cada color del LED RGB
    ledc_channel_config_t channels[3] = {
        {
            .channel = led_rgb->led_red.channel,
            .duty = led_rgb->led_red.duty,
            .gpio_num = led_rgb->led_red.gpio_num,
            .speed_mode = led_rgb->speed_mode,
            .timer_sel = led_rgb->timer
        },
        {
            .channel = led_rgb->led_green.channel,
            .duty = led_rgb->led_green.duty,
            .gpio_num = led_rgb->led_green.gpio_num,
            .speed_mode = led_rgb->speed_mode,
            .timer_sel = led_rgb->timer
        },
        {
            .channel = led_rgb->led_blue.channel,
            .duty = led_rgb->led_blue.duty,
            .gpio_num = led_rgb->led_blue.gpio_num,
            .speed_mode = led_rgb->speed_mode,
            .timer_sel = led_rgb->timer
        }
    };

    ledc_timer_config(&timer_config);

    for (int i = 0; i < 3; i++) {
        ledc_channel_config(&channels[i]);
    }

}

void set_duty_rgb (led_rgb_t *led_rgb, uint32_t duty_red, uint32_t duty_green, uint32_t duty_blue) {
    // Actualizar los valores de duty cycle en la estructura
    led_rgb->led_red.duty = duty_red;
    led_rgb->led_green.duty = duty_green;
    led_rgb->led_blue.duty = duty_blue;

    // Configurar el duty cycle para cada canal
    ledc_set_duty(led_rgb->speed_mode, led_rgb->led_red.channel, duty_red);
    ledc_update_duty(led_rgb->speed_mode, led_rgb->led_red.channel);

    ledc_set_duty(led_rgb->speed_mode, led_rgb->led_green.channel, duty_green);
    ledc_update_duty(led_rgb->speed_mode, led_rgb->led_green.channel);

    ledc_set_duty(led_rgb->speed_mode, led_rgb->led_blue.channel, duty_blue);
    ledc_update_duty(led_rgb->speed_mode, led_rgb->led_blue.channel);
}


void increment_duty_rgb(led_rgb_t *led_rgb, uint8_t color, uint8_t increment_percent) {
    uint32_t max_duty = (1 << led_rgb->duty_resolution) - 1;  // Máximo duty según resolución
    uint32_t new_duty;
    
    // Calcular incremento en función del porcentaje
    uint32_t increment = (max_duty * increment_percent) / 100;
    
    if (color == 0) {  // Red
        new_duty = led_rgb->led_red.duty + increment;
        if (new_duty > max_duty) new_duty = max_duty;  // Limitar al máximo
        led_rgb->led_red.duty = new_duty;
        ledc_set_duty(led_rgb->speed_mode, led_rgb->led_red.channel, new_duty);
        ledc_update_duty(led_rgb->speed_mode, led_rgb->led_red.channel);
    }
    else if (color == 1) {  // Green
        new_duty = led_rgb->led_green.duty + increment;
        if (new_duty > max_duty) new_duty = max_duty;
        led_rgb->led_green.duty = new_duty;
        ledc_set_duty(led_rgb->speed_mode, led_rgb->led_green.channel, new_duty);
        ledc_update_duty(led_rgb->speed_mode, led_rgb->led_green.channel);
    }
    else if (color == 2) {  // Blue
        new_duty = led_rgb->led_blue.duty + increment;
        if (new_duty > max_duty) new_duty = max_duty;
        led_rgb->led_blue.duty = new_duty;
        ledc_set_duty(led_rgb->speed_mode, led_rgb->led_blue.channel, new_duty);
        ledc_update_duty(led_rgb->speed_mode, led_rgb->led_blue.channel);
    }
}
