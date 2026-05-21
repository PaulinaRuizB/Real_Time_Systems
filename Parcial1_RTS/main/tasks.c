#include <libraries.h>

static const char *TASK_TAG = "TASKS";

// TAREA PARA EL UART
void uart_task(void *arg)
{
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);

        if (len > 0) {
            data[len] = '\0'; 
            //Para saber si recibió el mensaje 
            uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) data, len);
            
            xQueueSend(cmd_queue, (void *)data, portMAX_DELAY);
        }
    }
}

//tarea para procesar los comandos recibidos por UART

void command_task(void *arg) {

    char cmd[BUF_SIZE];
    system_config_t config;

    while (1) {

        if (xQueueReceive(cmd_queue, cmd, portMAX_DELAY)) {

            xQueuePeek(config_queue, &config, portMAX_DELAY); //para obtener la configuración actual
            process_uart_command(cmd, &config);
            xQueueOverwrite(config_queue, &config);
        }
    }
}

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

//tarea para controlar el RGB con limites de la temperatura 
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

//tarea para el funcionamiento de la temperatura y del potenciometro
void potentiometer_task(void *arg)
{
    gpio_init_buttons();

    int adc_raw;
    int last_pot = -1;
    system_config_t config;

    while (1)
    {

        xQueuePeek(
            config_queue,
            &config,
            portMAX_DELAY);

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

                    uart_write_bytes( ECHO_UART_PORT_NUM, "Unidad: Celsius\n", strlen("Unidad: Celsius\n"));
                break;

                case TEMP_FAHRENHEIT:

                    uart_write_bytes( ECHO_UART_PORT_NUM, "Unidad: Fahrenheit\n", strlen("Unidad: Fahrenheit\n"));
                break;

                case TEMP_KELVIN:
                    uart_write_bytes( ECHO_UART_PORT_NUM, "Unidad: Kelvin\n", strlen("Unidad: Kelvin\n"));
                    break;
            }

            // debounce
            vTaskDelay(pdMS_TO_TICKS(250));
        }

        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_POT, &adc_raw));

        uint8_t pot_value = map_adc_to_pwm(adc_raw);

        if(pot_value >= config.pot_threshold) //condición si traspasa el umbral
        {
            ledc_set_duty( LEDC_MODE, LEDC_CHANNEL_THRESHOLD, PWM_MAX);

            ledc_update_duty(LEDC_MODE,LEDC_CHANNEL_THRESHOLD);
        }
        else
        {
            ledc_set_duty(LEDC_MODE,LEDC_CHANNEL_THRESHOLD, 0);

            ledc_update_duty(LEDC_MODE,LEDC_CHANNEL_THRESHOLD);
        }

        if(abs(pot_value - last_pot) > 3)
        {
            ESP_LOGI(TASK_TAG, "Potenciometro: %d | Umbral: %d", pot_value, config.pot_threshold);

            last_pot = pot_value;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

