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

//UART CONFIG 

#define ECHO_TEST_TXD (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      0
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define BUF_SIZE (1024)
#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)


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

void update_pwm_preview();
void update_pwm();
uint8_t map_adc_to_pwm(int adc_raw);
void gpio_init_buttons();

// Tareas 
 void sensor_task(void *arg); 
 void rgb_task(void *arg); 
 void uart_task(void *arg); 
 void command_task(void *arg);

 void potentiometer_task(void *arg);

#endif 

