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

#define LED_R 3
#define LED_G 6
#define LED_B 7

//UART CONFIG 

#define ECHO_TEST_TXD (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      0
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define BUF_SIZE (1024)

// VARIABLES 

static const char *TAG = "NTC_UART_SYSTEM";

adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t adc_cali_handle;

bool do_calibration = false;

//TEMPERATURE LIMITS

float rojo_min = 35.0;

float verde_min = 25.0;
float verde_max = 35.0;

float azul_max = 25.0;

// PWM 
uint16_t pwm_intensity = PWM_MAX;