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

// ================================
// UNIDADES DE TEMPERATURA
// ================================
typedef enum {
    TEMP_CELSIUS = 0,
    TEMP_FAHRENHEIT,
    TEMP_KELVIN
} temp_unit_t;


// ================================
// CONFIGURACIÓN GLOBAL DEL SISTEMA
// ================================
typedef struct {

    // Rangos RGB
    float rojo_min;
    float rojo_max;

    float verde_min;
    float verde_max;

    float azul_min;
    float azul_max;

    // Intensidad PWM
    uint16_t pwm_intensity;

    // Tiempo de muestreo temperatura
    uint32_t temp_period_ms;

    // Unidad de temperatura
    temp_unit_t temp_unit;

    // Umbral del potenciómetro
    uint8_t pot_threshold;

} system_config_t;

extern QueueHandle_t temp_queue;
extern QueueHandle_t cmd_queue;
extern QueueHandle_t config_queue;

// Funciones 
void system_init(void); 

void pwm_init(void); 

void uart_init(void); 

void adc_init(void); 

void set_color(uint16_t r, uint16_t g, uint16_t b); 

float ntc_calculate_temperature(void); 

uint8_t map_adc_to_pwm(int adc_value);

void process_uart_command(char *cmd, system_config_t *config);

void gpio_init_buttons();

// Tareas 
 void sensor_task(void *arg); 
 void rgb_task(void *arg); 
 void uart_task(void *arg); 
 void command_task(void *arg);

 void potentiometer_task(void *arg);

#endif 

