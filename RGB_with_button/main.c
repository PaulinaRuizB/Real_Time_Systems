
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "libraries.h"

#define LED_R 4
#define LED_G 5
#define LED_B 6


#define BUTTON_R  2
#define BUTTON_G  3
#define BUTTON_B  1

// variable configurations 
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES   LEDC_TIMER_10_BIT  
#define LEDC_FREQUENCY  5000

#define LEDC_CHANNEL_R  LEDC_CHANNEL_0
#define LEDC_CHANNEL_G  LEDC_CHANNEL_1
#define LEDC_CHANNEL_B  LEDC_CHANNEL_2

// needed variables 
static led_rgb_t my_led_rgb;
static QueueHandle_t button_queue = NULL;

// interruption with IRAM 
static void IRAM_ATTR button_isr_handler(void* arg)
{
    uint32_t button_num = (uint32_t)(intptr_t)arg;

    BaseType_t hp = pdFALSE;

    xQueueSendFromISR(button_queue, &button_num, &hp);

    if (hp) {
        portYIELD_FROM_ISR();
    }
}

// for the button 
static void button_task(void *arg)
{
    uint32_t button_num;
    TickType_t last_press[3] = {0, 0, 0};

    while (1) {
        if (xQueueReceive(button_queue, &button_num, portMAX_DELAY)) {

            TickType_t now = xTaskGetTickCount();

            if (now - last_press[button_num] > pdMS_TO_TICKS(150)) {

                gpio_num_t pins[3] = {BUTTON_R, BUTTON_G, BUTTON_B};

                if (gpio_get_level(pins[button_num]) == 0) {

                    const char* color_name =
                        (button_num == 0) ? "Red" :
                        (button_num == 1) ? "Green" : "Blue";

                    ESP_LOGI("BUTTON", "Button %lu (%s)", button_num, color_name);

                    increment_duty_rgb(&my_led_rgb, button_num, 10);

                    last_press[button_num] = now;
                }
            }
        }
    }
}

void app_main(void)
{
    // Configuración LED
    my_led_rgb = (led_rgb_t) {
        .led_red   = {.duty = 0, .gpio_num = LED_R, .channel = LEDC_CHANNEL_R},
        .led_green = {.duty = 0, .gpio_num = LED_G, .channel = LEDC_CHANNEL_G},
        .led_blue  = {.duty = 0, .gpio_num = LED_B, .channel = LEDC_CHANNEL_B},
        .timer = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .frequency = LEDC_FREQUENCY,
        .speed_mode = LEDC_MODE
    };

    ledc_init(&my_led_rgb);

    // inicial ilumination
    increment_duty_rgb(&my_led_rgb, 0, 20);

    // Cola
    button_queue = xQueueCreate(10, sizeof(uint32_t));
    if (!button_queue) {
        ESP_LOGE("MAIN", "Error creando cola");
        vTaskDelay(portMAX_DELAY);
    }

    // Task
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    // Configuración botones
    button_config_t button_pins[3] = {
        {.gpio_num = BUTTON_R},
        {.gpio_num = BUTTON_G},
        {.gpio_num = BUTTON_B}
    };

    button_init(button_pins, button_isr_handler);

    ESP_LOGI("MAIN", "Sistema listo");
}
