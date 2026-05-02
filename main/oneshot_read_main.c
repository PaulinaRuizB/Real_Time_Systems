/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/ledc.h"

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
//ADC1 Channels
#define ADC_CHANNEL                 ADC_CHANNEL_2     
#define ADC_ATTEN                   ADC_ATTEN_DB_12

#define VCC 3.3 //VCC of the system
#define R_FIXED 4700.0
#define R0 4700.0
#define T0 298.15
#define BETA 3470.0

//PWM CONFIG 
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES   LEDC_TIMER_10_BIT  
#define LEDC_FREQUENCY  5000

#define LEDC_CHANNEL_R  LEDC_CHANNEL_0
#define LEDC_CHANNEL_G  LEDC_CHANNEL_1
#define LEDC_CHANNEL_B  LEDC_CHANNEL_2

#define PWM_MAX ((1 << 10) - 1)

//LED RGB 
#define LED_R 3 
#define LED_G 4 
#define LED_B 5 

const static char *TAG = "NTC_SYSTEM";

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);

//ADC 
adc_oneshot_unit_handle_t adc_handle; 
adc_cali_handle_t adc_cali_handle; 
bool do_calibration = false; 

//PWM

void pwm_init(){

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

    ch.channel = LEDC_CHANNEL_R; ch.gpio_num = LED_R; ledc_channel_config(&ch);
    ch.channel = LEDC_CHANNEL_G; ch.gpio_num = LED_G; ledc_channel_config(&ch);
    ch.channel = LEDC_CHANNEL_B; ch.gpio_num = LED_B; ledc_channel_config(&ch);
}

void set_color(uint16_t r, uint16_t g, uint16_t b){
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_R, r);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_R);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_G, g);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_G);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_B, b);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_B);
}

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (!calibrated) {
        ESP_LOGW(TAG, "ADC Calibration not available");
    }

    return calibrated;
}


void app_main(void)
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    //-------------ADC1 Calibration Init---------------//
    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL,ADC_ATTEN, &adc_cali_handle);
    
    pwm_init();

    while (1) {

        int raw, voltage_mv; 

        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, adc_raw[0][0]);
        if (do_calibration) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage_mv));
            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, voltage_mv);
        } else {
            voltage_mv = raw * 3300 / 4095;
        }

        float Vout = voltage_mv / 1000.0;

        if (Vout <= 0.01 || Vout >= VCC - 0.01) {
            ESP_LOGW(TAG, "Voltaje fuera de rango");
            continue;
        }

        float R_ntc = R_FIXED * (Vout / (VCC - Vout));

        float T_kelvin = 1.0 / ((1.0/T0) + (1.0/BETA) * log(R_ntc / R0));
        float T_celsius = T_kelvin - 273.15;

        ESP_LOGI(TAG, "Temp: %.2f °C", T_celsius);

        //=========== CONTROL LED ===========//

        if (T_celsius < 25) {
            set_color(0, 0, PWM_MAX);       // Azul
        }
        else if (T_celsius <= 35) {
            set_color(0, PWM_MAX, 0);       // Verde
        }
        else {
            set_color(PWM_MAX, 0, 0);       // Rojo
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}