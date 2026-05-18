#include <Libraries.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

//ADC CONFIG
#define ADC_CHANNEL        ADC_CHANNEL_2
#define ADC_ATTEN          ADC_ATTEN_DB_12

#define VCC        3.3
#define R_FIXED    4700.0
#define R0          4700.0
#define T0          298.15
#define BETA        3470.0

//PWM CONFIG
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES   LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY  5000

#define LEDC_CHANNEL_R  LEDC_CHANNEL_0
#define LEDC_CHANNEL_G  LEDC_CHANNEL_1
#define LEDC_CHANNEL_B  LEDC_CHANNEL_2

#define PWM_MAX ((1 << 10) - 1)

//PINS LED RGB

#define LED_R 4
#define LED_G 5
#define LED_B 6


void app_main(void) { 
    
    system_init(); 
    
    static rgb_config_t rgb_config = { 
        
        .rojo_min = 35.0, 
        .rojo_max = 80.0, 
        .verde_min = 25.0, 
        .verde_max = 35.0, 
        .azul_min = 10.0, 
        .azul_max = 25.0, 
        .pwm_intensity = PWM_MAX }; 
        
        xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL); 
        xTaskCreate(rgb_task, "rgb_task", 4096, &rgb_config, 5, NULL); 
        xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, NULL); 
        xTaskCreate(command_task, "command_task", 4096, &rgb_config, 5, NULL);
}