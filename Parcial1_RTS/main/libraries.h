#ifndef LIBRARIES_H
#define LIBRARIES_H

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
// ADC Channel 2 (GPIO2) para el sensor NTC de temperatura
#define ADC_CHANNEL_NTC    ADC_CHANNEL_2
// ADC Channel 1 (GPIO1) para el potenciómetro
#define ADC_CHANNEL_POT    ADC_CHANNEL_1
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

//canales para LED 1 NTC
#define LEDC_CHANNEL_R  LEDC_CHANNEL_0
#define LEDC_CHANNEL_G  LEDC_CHANNEL_1
#define LEDC_CHANNEL_B  LEDC_CHANNEL_2

//canales para LED 2 potenciometro 
#define LED_THRESHOLD 7 
#define LEDC_CHANNEL_THRESHOLD LEDC_CHANNEL_3
#define PWM_MAX ((1 << 10) - 1)

//PINS LED RGB

#define LED_R 4
#define LED_G 5
#define LED_B 6

//UART
#define ECHO_UART_PORT_NUM      0
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define BUF_SIZE (1024)
#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)

// Botones de control 

#define BTN   GPIO_NUM_10

//para la configuración del ADC
extern adc_oneshot_unit_handle_t adc_handle;
extern adc_cali_handle_t adc_cali_handle;
extern bool do_calibration;

// estructura para cambiar de unidad de temperatura
typedef enum {
    TEMP_CELSIUS = 0,
    TEMP_FAHRENHEIT,
    TEMP_KELVIN
} temp_unit_t;

//estructura para los limites de temperatura y la intensidad del PWM
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

//colas para cada tarea
extern QueueHandle_t temp_queue; //temperatura 
extern QueueHandle_t cmd_queue; //mensajes de UART 
extern QueueHandle_t config_queue; //configuración general 

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

