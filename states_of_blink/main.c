/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define LED_PIN 8
#define BTN_PIN 9


// Definición de la estructura para los tiempos
typedef struct {
    uint32_t on_ms;
    uint32_t off_ms;
} BlinkConfig;


// SIN INTERRUPCIONES 

// Prototipos de tareas
void task_button(void *pvParameters);
void task_led(void *pvParameters);

// Manejador de la cola
QueueHandle_t xQueueEstado; // Cola para enviar el estado del botón al LED

void app_main(void) {
    // Configuración de pines
    gpio_reset_pin(LED_PIN); //resetea 
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT); //configura como salida
    
    gpio_reset_pin(BTN_PIN);
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(BTN_PIN); // Activar resistencia pull-up interna

    // Crear cola para un solo entero (el número del estado)
    xQueueEstado = xQueueCreate(1, sizeof(int));

    if (xQueueEstado != NULL) {
        xTaskCreate(task_button, "Lectura_Boton", 2048, NULL, 1, NULL);
        xTaskCreate(task_led, "Control_LED", 2048, NULL, 2, NULL);
    }
}

void task_button(void *pvParameters) {
    int estado_actual = 1;
    int ultimo_nivel = 1;

    // Enviamos el estado inicial
    xQueueOverwrite(xQueueEstado, &estado_actual);

    while (1) {
        int nivel = gpio_get_level(BTN_PIN);
        
        // Detección de flanco de bajada (presionado) con debounce simple
        if (nivel == 0 && ultimo_nivel == 1) {
            estado_actual++;
            if (estado_actual > 4) estado_actual = 1;
            
            // Enviamos a la cola (sobrescribe si el LED no ha leído)
            xQueueOverwrite(xQueueEstado, &estado_actual);
            printf("Cambiando a Estado: %d\n", estado_actual);
            vTaskDelay(pdMS_TO_TICKS(200)); // Debounce
        }
        ultimo_nivel = nivel;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void task_led(void *pvParameters) {
    int estado_recibido = 1;
    BlinkConfig config = {2000, 2000}; // Valores por defecto (Estado 1)

    while (1) {
        // Revisamos la cola sin bloquear, para actualizar tiempos si hay cambio
        if (xQueueReceive(xQueueEstado, &estado_recibido, 0) == pdPASS) {
            switch (estado_recibido) {
                case 1: config.on_ms = 2000; config.off_ms = 2000; break;
                case 2: config.on_ms = 1000; config.off_ms = 2000; break;
                case 3: config.on_ms = 500;  config.off_ms = 500;  break;
                case 4: config.on_ms = 100;  config.off_ms = 100;  break;
            }
        }

        // Ejecución del parpadeo
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(config.on_ms));
        
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(config.off_ms));
    }
}



// CON INTERRUPCIONES 

QueueHandle_t xQueueEstado; 

void IRAM_ATTR gpio_isr_handler(void* arg) //IRAM_ATTR carga en la RAM 
//puntero void para luego hacer casting
{
    int estado = 1; // señal simple (no importa valor)
    xQueueSendFromISR(xQueueEstado, &estado, NULL); //FromISR tiene mayor prioridad 
}

void task_led(void *pvParameters)
{
    int estado_actual = 1;
    BlinkConfig config = {2000, 2000};
    int dummy;

    while (1)
    {
        // Espera evento del botón (no bloqueante)
        if (xQueueReceive(xQueueEstado, &dummy, 0) == pdPASS) //pdPASS indica que se recibió correctamente algo de la cola

        {
            estado_actual++;
            if (estado_actual > 4) estado_actual = 1;

            switch (estado_actual)
            {
                case 1: config = (BlinkConfig){2000, 2000}; break;
                case 2: config = (BlinkConfig){1000, 2000}; break;
                case 3: config = (BlinkConfig){500, 500}; break;
                case 4: config = (BlinkConfig){100, 100}; break;
            }

            printf("Nuevo estado: %d\n", estado_actual);
        }

        // Parpadeo NO bloqueante (fragmentado)
        for (int i = 0; i < config.on_ms; i += 50) //revisa constantemente la cola cada 50ms para 
        //detectar cambios de estado

        {
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(50));

            if (xQueueReceive(xQueueEstado, &dummy, 0)) //dummy es para verificar si hay un nuevo evento 
            //del botón, aunque no se use el valor
                break;
        }

        for (int i = 0; i < config.off_ms; i += 50)
        {
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(50));

            if (xQueueReceive(xQueueEstado, &dummy, 0))
                break;
        }
    }
}

void configure_gpio()
{
    // LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    // Botón
    gpio_reset_pin(BTN_PIN);
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(BTN_PIN);

    // Configurar interrupción por flanco de bajada
    gpio_set_intr_type(BTN_PIN, GPIO_INTR_NEGEDGE); //GPIO_INTR_NEGEDGE para detectar 
    //cuando se presiona el botón (nivel bajo por el pull-up)

    // Instalar servicio ISR
    gpio_install_isr_service(0);

    // Asociar ISR al pin
    gpio_isr_handler_add(BTN_PIN, gpio_isr_handler, NULL);
}

void app_main(void)
{
    configure_gpio();

    xQueueEstado = xQueueCreate(5, sizeof(int));

    xTaskCreate(task_led, "task_led", 4096, NULL, 2, NULL);
}
