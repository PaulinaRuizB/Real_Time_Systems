#include <Libraries.h> 

static const char *TAG = "NTC_SYSTEM"; 

// Init functions

//ADC INIT 
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle;
static bool do_calibration = false;

// Colas 

QueueHandle_t temp_queue; 
QueueHandle_t cmd_queue;

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

    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config);

    do_calibration = example_adc_calibration_init(
        ADC_UNIT_1,
        ADC_CHANNEL,
        ADC_ATTEN,
        &adc_cali_handle);
}

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

     uart_driver_install(ECHO_UART_PORT_NUM , BUF_SIZE * 2, 0, 0, NULL, 0);
     uart_param_config(ECHO_UART_PORT_NUM , &uart_config);

    // Mensaje de bienvenida
    uart_write_bytes(ECHO_UART_PORT_NUM, "Sistema NTC UART listo. Envia comandos como ROJO_MIN_35 o PWM_50\n", strlen("Sistema NTC UART listo. Envia comandos como ROJO_MIN_35 o PWM_50\n"));
}

// INIT Sistema (pwm, uart, adc)

void system_init() { 
    pwm_init(); 
    uart_init(); 
    adc_init(); 
    temp_queue = xQueueCreate( 5, sizeof(float)); 
    cmd_queue = xQueueCreate( 5, BUF_SIZE); 
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

        adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw);

        if (do_calibration) {

            adc_cali_raw_to_voltage(adc_cali_handle,
                raw,
                &voltage_mv);
        }
        else {

            voltage_mv = (raw * 3300) / 4095;
        }

        float Vout = voltage_mv / 1000.0;

        char voltage_msg[50];
        sprintf(voltage_msg, "Voltaje: %.2f V\n", Vout);
        uart_write_bytes(ECHO_UART_PORT_NUM, voltage_msg, strlen(voltage_msg));

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

void process_uart_command(char *cmd, rgb_config_t *config)
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
    } 
    else {
        
        uart_write_bytes(ECHO_UART_PORT_NUM , "No reconozco el comando\n", strlen("No reconozco el comando\n"));
    }
}

// Tareas 

// Tarea para leer el sensor y enviar la temperatura a la cola
void sensor_task(void *arg) { 
    float temperature; 
    while (1) { 
        temperature = ntc_calculate_temperature(); 
        xQueueSend(temp_queue, &temperature, portMAX_DELAY); 
        vTaskDelay(pdMS_TO_TICKS(2000)); 
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
            
            xQueueSend(cmd_queue, data, portMAX_DELAY);
        }
    }
}

void command_task(void *arg) { 
    
    char cmd[BUF_SIZE]; 
    rgb_config_t *config = (rgb_config_t *) arg; 
    
    while (1) { 
        
        if (xQueueReceive(cmd_queue, cmd, portMAX_DELAY)) { 
            process_uart_command(cmd, config); 
        } 
    } 
} 

void rgb_task(void *arg) { 
    
    float temp; rgb_config_t *config = (rgb_config_t *) arg; 
    
    while (1) { 
        
        if (xQueueReceive(temp_queue, &temp, portMAX_DELAY)) {
             
            uint16_t r = 0; 
            uint16_t g = 0; 
            uint16_t b = 0; 

            if (temp >= config->azul_min && temp <= config->azul_max) { 
                
                b = config->pwm_intensity; 
            } 
                
            if (temp >= config->verde_min && temp <= config->verde_max) { 
                g = config->pwm_intensity; 
            } 
            
            if (temp >= config->rojo_min && temp <= config->rojo_max) { 
                
                r = config->pwm_intensity; 
            } 
                
                set_color(r, g, b); 
        } 
    } 
}