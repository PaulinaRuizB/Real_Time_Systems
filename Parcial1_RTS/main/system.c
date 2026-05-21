#include <libraries.h>
// Colas 

QueueHandle_t temp_queue; 
QueueHandle_t cmd_queue;
QueueHandle_t config_queue;

//configuración del botón para cambios de unidades de tempretura 
void gpio_init_buttons()
{
    gpio_config_t io = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pin_bit_mask = (1ULL << BTN)
    };

    gpio_config(&io);
}

//Función para inicializar el sistema 
// INIT Sistema (pwm, uart, adc)

void system_init() {
    pwm_init();
    uart_init();
    adc_init();
    temp_queue = xQueueCreate(5, sizeof(float));
    cmd_queue = xQueueCreate(5, BUF_SIZE);
    config_queue = xQueueCreate(1, sizeof(system_config_t));
}