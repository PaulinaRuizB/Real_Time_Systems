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

//UART CONFIG 

#define ECHO_TEST_TXD (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      0
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define BUF_SIZE (1024)
#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)

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
    else if (sscanf(cmd, "ROJO_MAX_%d", &value) == 1) {
        rojo_max = (float)value;
        ESP_LOGI(TAG, "¡Éxito! Nuevo ROJO_MAX = %.1f", rojo_max);
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: ROJO_MAX configurado\n", strlen("OK: ROJO_MAX configurado\n"));
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
    else if (sscanf(cmd, "AZUL_MIN_%d", &value) == 1) {
        azul_min = (float)value;
        ESP_LOGI(TAG, "¡Éxito! Nuevo AZUL_MIN = %.1f", azul_min);
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: AZUL_MIN configurado\n", strlen("OK: AZUL_MIN configurado\n"));
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

void app_main(void)
{

    /* INIT */

    pwm_init();

    uart_init();

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 10, NULL);

    while (1) {

        /* CONTROL RGB MEZCLADO */

        uint16_t r = 0;
        uint16_t g = 0;
        uint16_t b = 0;

        /* Azul */

        if (T_celsius >= azul_min &&
            T_celsius <= azul_max) {

            b = pwm_intensity;
        }

        /* Verde */

        if (T_celsius >= verde_min &&
            T_celsius <= verde_max) {

            g = pwm_intensity;
        }

        /* Rojo */

        if (T_celsius >= rojo_min &&
            T_celsius <= rojo_max) {

            r = pwm_intensity;
        }

        /* Aplicar mezcla final */

        set_color(r, g, b);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}