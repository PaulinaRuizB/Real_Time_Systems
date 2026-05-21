// Calibración ADC 
#include <libraries.h>

//ADC INIT 
adc_oneshot_unit_handle_t adc_handle = NULL;
adc_cali_handle_t adc_cali_handle = NULL;
bool do_calibration = false;
static const char *ADC_TAG = "ADC_DRIVER";

//Calibración de los canales del ADC
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

        ESP_LOGI(ADC_TAG, "V: %.2f V", Vout);
        
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

//calibración del ADC para el potenciometro
uint8_t map_adc_to_pwm(int adc_raw)
{
    return (adc_raw * 255) / 4095;
}