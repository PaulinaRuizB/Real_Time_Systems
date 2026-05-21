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

// Botones de control 

#define BTN   GPIO_NUM_10


static const char *TAG = "NTC_SYSTEM"; 

// Init functions

//ADC INIT 
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle;
static bool do_calibration = false;

// Colas 

QueueHandle_t temp_queue; 
QueueHandle_t cmd_queue;
QueueHandle_t config_queue;

// Calibración ADC 
static bool example_adc_calibration_init( adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) { 
    adc_cali_handle_t handle = NULL; 
    bool calibrated = false; 
    
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED 
    
    adc_cali_curve_fitting_config_t cali_config = { 
        .unit_id = unit, 
        .chan = channel, 
        .atten = atten, 
        .bitwidth = ADC_BITWIDTH_DEFAULT, 
    }; 
        
        if (adc_cali_create_scheme_curve_fitting( &cali_config, &handle) == ESP_OK) { 
            calibrated = true; 
        } 
        
#endif 
        *out_handle = handle; 
        return calibrated; 
    }

// ADC Init

void adc_init()
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };

    adc_oneshot_new_unit(&init_config1, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_NTC, &config);
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_POT, &config);

    do_calibration = example_adc_calibration_init(
        ADC_UNIT_1,
        ADC_CHANNEL_NTC,
        ADC_ATTEN,
        &adc_cali_handle);
}

// FUNCIÓN CONFIG PWM ambos funcionamientos de LEDS
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

    ch.channel = LEDC_CHANNEL_THRESHOLD;
    ch.gpio_num = LED_THRESHOLD;
    ledc_channel_config(&ch);
}

//configuración LED 1 NTC
//CONFIG UART

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

    uart_param_config(ECHO_UART_PORT_NUM, &uart_config);
    uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    // Mensaje de bienvenida
    uart_write_bytes(ECHO_UART_PORT_NUM, "Sistema NTC UART listo. Envia comandos como ROJO_MIN_35 o PWM_50\n", strlen("Sistema NTC UART listo. Envia comandos como ROJO_MIN_35 o PWM_50\n"));
}

// INIT Sistema (pwm, uart, adc)

void system_init() {
    pwm_init();
    uart_init();
    adc_init();
    temp_queue = xQueueCreate(5, sizeof(float));
    cmd_queue = xQueueCreate(5, BUF_SIZE);
    config_queue = xQueueCreate(1, sizeof(system_config_t));
}

//Control RGB
void set_color(uint16_t r, uint16_t g, uint16_t b)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_R, r);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_R);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_G, g);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_G);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_B, b);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_B);
}

// Calcular la temperatura en base al ADC 

float ntc_calculate_temperature(){
    int raw;
        int voltage_mv;

        adc_oneshot_read(adc_handle, ADC_CHANNEL_NTC, &raw);

        if (do_calibration) {

            adc_cali_raw_to_voltage(adc_cali_handle,
                raw,
                &voltage_mv);
        }
        else {

            voltage_mv = (raw * 3300) / 4095;
        }

        float Vout = voltage_mv / 1000.0;

        if (Vout <= 0.01 || Vout >= VCC - 0.01) {

            uart_write_bytes(ECHO_UART_PORT_NUM , "ERROR: Voltaje fuera de rango\n", strlen("ERROR: Voltaje fuera de rango\n"));

            vTaskDelay(pdMS_TO_TICKS(100));

            return -100.0;
        }

        ESP_LOGI(TAG, "V: %.2f V", Vout);
        
        float R_ntc = R_FIXED * (Vout / (VCC - Vout));  

        float T_kelvin =
            1.0 /
            ((1.0 / T0) +
             (1.0 / BETA) * log(R_ntc / R0));

        float T_celsius = T_kelvin - 273.15;

        char temperature_msg[50];
        sprintf(temperature_msg, "Temperatura: %.2f °C\n", T_celsius);
        uart_write_bytes(ECHO_UART_PORT_NUM, temperature_msg, strlen(temperature_msg));

        return T_celsius;
}

// Procesar comandos de UART

void process_uart_command(char *cmd, system_config_t *config)
{
    
    cmd[strcspn(cmd, "\r\n")] = 0;

    int value;
    
    // LÍMITES DE TEMPERATURA
    if (sscanf(cmd, "ROJO_MIN_%d", &value) == 1) {
        config->rojo_min = (float)value;
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: ROJO_MIN configurado\n", strlen("OK: ROJO_MIN configurado\n"));
    } 
    else if (sscanf(cmd, "ROJO_MAX_%d", &value) == 1) {
        config->rojo_max = (float)value;
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: ROJO_MAX configurado\n", strlen("OK: ROJO_MAX configurado\n"));
    } 
    
    else if (sscanf(cmd, "VERDE_MIN_%d", &value) == 1) {
        config->verde_min = (float)value;
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: VERDE_MIN configurado\n", strlen("OK: VERDE_MIN configurado\n"));
    } 
    else if (sscanf(cmd, "VERDE_MAX_%d", &value) == 1) {
        config->verde_max = (float)value;
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: VERDE_MAX configurado\n", strlen("OK: VERDE_MAX configurado\n"));
    } 

    else if (sscanf(cmd, "AZUL_MIN_%d", &value) == 1) {
        config->azul_min = (float)value;
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: AZUL_MIN configurado\n", strlen("OK: AZUL_MIN configurado\n"));
    } 
    else if (sscanf(cmd, "AZUL_MAX_%d", &value) == 1) {
        config->azul_max = (float)value;
        uart_write_bytes(ECHO_UART_PORT_NUM , "OK: AZUL_MAX configurado\n", strlen("OK: AZUL_MAX configurado\n"));
    } 

    else if (sscanf(cmd, "PWM_%d", &value) == 1) {
        if (value >= 0 && value <= 100) {
            config->pwm_intensity = (value * PWM_MAX) / 100;
            char pwm_msg[50];
            sprintf(pwm_msg, "OK: PWM configurado al %d%%\n", value);
            uart_write_bytes(ECHO_UART_PORT_NUM , pwm_msg, strlen(pwm_msg));
        } else {
            uart_write_bytes(ECHO_UART_PORT_NUM , "ERROR: PWM debe estar entre 0 y 100\n", strlen("ERROR: PWM debe estar entre 0 y 100\n"));
        }
    } //tiempo de impresión
    else if (sscanf(cmd, "TIEMPO_%d", &value) == 1) {
        if (value > 0) {
            config->temp_period_ms = value * 1000;
            char msg[64]; sprintf(msg, "OK: Tiempo %d s\n", value);
            uart_write_bytes(ECHO_UART_PORT_NUM, msg, strlen(msg));
        } else {
            uart_write_bytes(ECHO_UART_PORT_NUM, "ERROR: TIEMPO debe ser mayor que 0\n", strlen("ERROR: TIEMPO debe ser mayor que 0\n"));
        }
    }
    //Umbral para que se encienda el led con el potenciometro
    else if (sscanf(cmd, "UMBRAL_%d", &value) == 1) {
        if (value >= 0 && value <= 255) {
            config->pot_threshold = value;
            char msg[64];
            sprintf(msg, "OK: Umbral %d\n", value);
            uart_write_bytes(ECHO_UART_PORT_NUM, msg, strlen(msg));
        } else {
            uart_write_bytes(ECHO_UART_PORT_NUM, "ERROR: UMBRAL debe estar entre 0 y 255\n", strlen("ERROR: UMBRAL debe estar entre 0 y 255\n"));
        }
    }
    else {
        
        uart_write_bytes(ECHO_UART_PORT_NUM , "No reconozco el comando\n", strlen("No reconozco el comando\n"));
    }
}

// Tareas 

// Tarea para leer el sensor y enviar la temperatura a la cola
void sensor_task(void *arg) {
    float temperature;
    system_config_t config;

    while (1) {
        xQueuePeek(config_queue, &config, portMAX_DELAY);
        temperature = ntc_calculate_temperature();

        // para mostrar la temperatura según unidad por cada pulsación del botón
        float temp_show;
        char unit[5];

        switch (config.temp_unit)
        {
            case TEMP_CELSIUS:
                temp_show = temperature;
                strcpy(unit, "C");
                break;

            case TEMP_KELVIN:
                temp_show = temperature + 273.15;
                strcpy(unit, "K");
                break;

            case TEMP_FAHRENHEIT:
                temp_show = (temperature * 9.0 / 5.0) + 32.0;
                strcpy(unit, "F");
                break;

            default:
                temp_show = temperature;
                strcpy(unit, "C");
                break;
        }

        char msg[64]; 
        sprintf(msg, "Temperatura: %.2f %s\n", temp_show, unit);
        uart_write_bytes (ECHO_UART_PORT_NUM, msg, strlen(msg));
        xQueueSend(temp_queue, &temperature, portMAX_DELAY); 
        vTaskDelay(pdMS_TO_TICKS(config.temp_period_ms)); 
    } 
}

// TAREA PARA EL UART
void uart_task(void *arg)
{
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);

        if (len > 0) {
            data[len] = '\0'; // Terminamos la cadena de texto
            
            // Hacemos un "ECHO": devolvemos el texto a YAT para confirmar recepción
            uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) data, len);
            
            xQueueSend(cmd_queue, (void *)data, portMAX_DELAY);
        }
    }
}

void command_task(void *arg) {

    char cmd[BUF_SIZE];
    system_config_t config;

    while (1) {

        if (xQueueReceive(cmd_queue, cmd, portMAX_DELAY)) {

            xQueuePeek(config_queue, &config, portMAX_DELAY);
            process_uart_command(cmd, &config);
            xQueueOverwrite(config_queue, &config);
        }
    }
}

void rgb_task(void *arg) {

    float temp;
    system_config_t current_config;

    while (1) {

        if (xQueueReceive(temp_queue, &temp, portMAX_DELAY)) {

            xQueuePeek(config_queue, &current_config, portMAX_DELAY);

            uint16_t r = 0;
            uint16_t g = 0;
            uint16_t b = 0;

            if (temp >= current_config.azul_min && temp <= current_config.azul_max) {
                b = current_config.pwm_intensity;
            }

            if (temp >= current_config.verde_min && temp <= current_config.verde_max) {
                g = current_config.pwm_intensity;
            }

            if (temp >= current_config.rojo_min && temp <= current_config.rojo_max) {
                r = current_config.pwm_intensity;
            }

            set_color(r, g, b);
        } 
    } 
}

// configuración LED potenciometro

// ======================================================
// CONFIGURACION BOTON UNIDAD
// ======================================================

void gpio_init_buttons()
{
    gpio_config_t io = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pin_bit_mask = (1ULL << BTN)
    };

    gpio_config(&io);
}


// ======================================================
// MAP ADC -> 8 BITS
// ======================================================

uint8_t map_adc_to_pwm(int adc_raw)
{
    return (adc_raw * 255) / 4095;
}


// ======================================================
// POTENTIOMETER TASK
// ======================================================

void potentiometer_task(void *arg)
{
    gpio_init_buttons();

    int adc_raw;

    int last_pot = -1;

    system_config_t config;

    while (1)
    {
        // ==========================================
        // OBTENER CONFIGURACION ACTUAL
        // ==========================================

        xQueuePeek(
            config_queue,
            &config,
            portMAX_DELAY);

        // ==========================================
        // CAMBIO UNIDAD TEMPERATURA
        // ==========================================

        if (!gpio_get_level(BTN))
        {
            // Cambiar unidad

            config.temp_unit++;

            if(config.temp_unit > TEMP_KELVIN)
            {
                config.temp_unit =
                    TEMP_CELSIUS;
            }

            // Guardar configuracion

            xQueueOverwrite(
                config_queue,
                &config);

            // Mensaje UART

            switch(config.temp_unit)
            {
                case TEMP_CELSIUS:

                    uart_write_bytes(
                    ECHO_UART_PORT_NUM,
                    "Unidad: Celsius\n",
                    strlen("Unidad: Celsius\n"));

                break;

                case TEMP_FAHRENHEIT:

                    uart_write_bytes(
                    ECHO_UART_PORT_NUM,
                    "Unidad: Fahrenheit\n",
                    strlen("Unidad: Fahrenheit\n"));

                break;

                case TEMP_KELVIN:

                    uart_write_bytes(
                    ECHO_UART_PORT_NUM,
                    "Unidad: Kelvin\n",
                    strlen("Unidad: Kelvin\n"));

                    break;
            }

            // debounce

            vTaskDelay(pdMS_TO_TICKS(250));
        }

        // ==========================================
        // LEER POTENCIOMETRO
        // ==========================================

        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc_handle,
                ADC_CHANNEL_POT,
                &adc_raw));

        // convertir a 0-255

        uint8_t pot_value =
            map_adc_to_pwm(adc_raw);

        // ==========================================
        // CONTROL LED UMBRAL
        // ==========================================

        if(pot_value >= config.pot_threshold)
        {
            ledc_set_duty(
                LEDC_MODE,
                LEDC_CHANNEL_THRESHOLD,
                PWM_MAX);

            ledc_update_duty(
                LEDC_MODE,
                LEDC_CHANNEL_THRESHOLD);
        }
        else
        {
            ledc_set_duty(
                LEDC_MODE,
                LEDC_CHANNEL_THRESHOLD,
                0);

            ledc_update_duty(
                LEDC_MODE,
                LEDC_CHANNEL_THRESHOLD);
        }

        // ==========================================
        // LOG SOLO SI CAMBIA
        // ==========================================

        if(abs(pot_value - last_pot) > 3)
        {
            ESP_LOGI(
                TAG,
                "Potenciometro: %d | Umbral: %d",
                pot_value,
                config.pot_threshold);

            last_pot = pot_value;
        }

        // ==========================================
        // DELAY
        // ==========================================

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

