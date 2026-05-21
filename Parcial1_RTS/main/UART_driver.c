#include <libraries.h>

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
    //mensaje de PWM
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

