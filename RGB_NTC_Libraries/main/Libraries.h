#ifndef LIBRARIES_H
#define LIBRARIES_H

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <math.h> 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
#include "freertos/queue.h" 
#include "driver/ledc.h" 
#include "driver/uart.h" 
#include "esp_log.h" 
#include "esp_adc/adc_oneshot.h" 
#include "esp_adc/adc_cali.h" 
#include "esp_adc/adc_cali_scheme.h"

// Estructura para configurar los limites de cada color 
typedef struct { 
    float rojo_min; 
    float rojo_max; 
    float verde_min; 
    float verde_max; 
    float azul_min; 
    float azul_max; 
    uint16_t pwm_intensity; 
} rgb_config_t;

// Funciones 
void system_init(void); 

void pwm_init(void); 

void uart_init(void); 

void adc_init(void); 

void set_color(uint16_t r, uint16_t g, uint16_t b); 

float ntc_calculate_temperature(void); 

void process_uart_command(char *cmd, rgb_config_t *config);

// Tareas 
 void sensor_task(void *arg); 
 void rgb_task(void *arg); 
 void uart_task(void *arg); 
 void command_task(void *arg);

#endif 

