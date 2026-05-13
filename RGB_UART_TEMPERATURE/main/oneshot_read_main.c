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

// PWM CONFIG 
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

// FUNCIÓN CONFIG PWM
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
}

// FUNCIÓN PARA CONFIGURAR EL COLOR

void set_color(uint16_t r, uint16_t g, uint16_t b)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_R, r);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_R);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_G, g);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_G);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_B, b);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_B);
}

// CALBIRACIÓN DEL ADC (NO SÉ SI ES NECESARIO XD)

static bool example_adc_calibration_init( adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;

    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    if (adc_cali_create_scheme_curve_fitting(&cali_config, &handle) == ESP_OK) {
        calibrated = true;
    }

#endif

    *out_handle = handle;

    return calibrated;
}

void uart_init()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

     uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);
     uart_param_config(UART_NUM_0, &uart_config);

    // Mensaje de bienvenida
    uart_write_bytes(ECHO_UART_PORT_NUM , "Sistema NTC UART listo. Envia comandos como ROJO_MIN_35 o PWM_50\n", strlen("Sistema NTC UART listo. Envia comandos como ROJO_MIN_35 o PWM_50\n"));
}

//PROCESAR MENSAJES DE UART
void process_uart_command(char *cmd)
{
    
    cmd[strcspn(cmd, "\r\n")] = 0;

    int value;
    
    // LÍMITES DE TEMPERATURA
    if (sscanf(cmd, "ROJO_MIN_%d", &value) == 1) {
        rojo_min = (float)value;
        ESP_LOGI(TAG, "¡Éxito! Nuevo ROJO_MIN = %.1f", rojo_min);
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: ROJO_MIN configurado\n", strlen("OK: ROJO_MIN configurado\n"));
    } 
    else if (sscanf(cmd, "VERDE_MIN_%d", &value) == 1) {
        verde_min = (float)value;
        ESP_LOGI(TAG, "¡Éxito! Nuevo VERDE_MIN = %.1f", verde_min);
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: VERDE_MIN configurado\n", strlen("OK: VERDE_MIN configurado\n"));
    } 
    else if (sscanf(cmd, "VERDE_MAX_%d", &value) == 1) {
        verde_max = (float)value;
        ESP_LOGI(TAG, "¡Éxito! Nuevo VERDE_MAX = %.1f", verde_max);
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: VERDE_MAX configurado\n", strlen("OK: VERDE_MAX configurado\n"));
    } 
    else if (sscanf(cmd, "AZUL_MAX_%d", &value) == 1) {
        azul_max = (float)value;
        ESP_LOGI(TAG, "¡Éxito! Nuevo AZUL_MAX = %.1f", azul_max);
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: AZUL_MAX configurado\n", strlen("OK: AZUL_MAX configurado\n"));
    } 
    else if (sscanf(cmd, "PWM_%d", &value) == 1) {
        if (value >= 0 && value <= 100) {
            pwm_intensity = (value * PWM_MAX) / 100;
            ESP_LOGI(TAG, "¡Éxito! Intensidad LED al %d%%", value);
            char pwm_msg[50];
            sprintf(pwm_msg, "OK: PWM configurado al %d%%\n", value);
            uart_write_bytes(ECHO_UART_PORT_NUM , pwm_msg, strlen(pwm_msg));
        } else {
            ESP_LOGW(TAG, "Error: El PWM debe estar entre 0 y 100");
            uart_write_bytes(ECHO_UART_PORT_NUM , "ERROR: PWM debe estar entre 0 y 100\n", strlen("ERROR: PWM debe estar entre 0 y 100\n"));
        }
    } 
    else {
        
        ESP_LOGW(TAG, "No reconozco el comando: [%s]", cmd);
    }
}

// TAREA PARA EL UART
void uart_task(void *arg)
{
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        // Leemos como en el ejemplo de Echo
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);

        if (len > 0) {
            data[len] = '\0'; // Terminamos la cadena de texto
            
            // Hacemos un "ECHO": devolvemos el texto a YAT para confirmar recepción
            uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) data, len);
            
            // Procesamos el comando
            process_uart_command((char *)data);
        }
    }
}