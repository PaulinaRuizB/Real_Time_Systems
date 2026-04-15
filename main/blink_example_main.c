#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "blink_with_button"; //declaracion de etiqueta para logs (como print)

/* LED configurado desde menuconfig */
#define LED_GPIO      CONFIG_BLINK_GPIO 

#define BUTTON_GPIO   9

static bool s_led_state = false; //variable global para almacenar el estado del LED (encendido o apagado)

//función para configurar el GPIO del LED como salida y apagarlo inicialmente
static void configure_led(void) 
{
    ESP_LOGI(TAG, "Configuring GPIO LED..."); //imprime un mensaje de información en el log 
    //indicando que se está configurando el GPIO del LED
    gpio_reset_pin(LED_GPIO); //restablece el pin del LED a su estado predeterminado (desconfigurado)
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT); //configura el pin del LED como salida
    gpio_set_level(LED_GPIO, 0); //apaga el LED estableciendo su nivel lógico a 0 (LOW)
    //Esto asegura que el LED arranque apagado.
}

//función para configurar el GPIO del botón como entrada con resistencia pull-up interna
static void configure_button(void)
{
    ESP_LOGI(TAG, "Configuring Button GPIO..."); //imprime un mensaje de información en el 
    //log indicando que se está configurando el GPIO del botón
    gpio_config_t io_conf = { //estructura de configuración para el GPIO del botón
        .pin_bit_mask = (1ULL << BUTTON_GPIO), //configuración de máscara para seleccionar el 
        //pin del botón (en este caso, el pin 9)
        .mode = GPIO_MODE_INPUT, //configura el pin del botón como entrada
        .pull_up_en = GPIO_PULLUP_ENABLE, //habilita la resistencia pull-up interna para el pin del botón, 
        //lo que significa que el pin estará en estado HIGH (1) cuando el botón no esté presionado
        // y LOW (0) cuando el botón esté presionado
        .pull_down_en = GPIO_PULLDOWN_DISABLE, //deshabilita la resistencia pull-down interna para
        // el pin del botón
        .intr_type = GPIO_INTR_DISABLE //deshabilita las interrupciones para el pin del botón,
        //ya que en este ejemplo se leerá el estado del botón de forma periódica en el bucle principal
    };
    gpio_config(&io_conf); //Escribe en los registros del GPIO del ESP32.
}

void app_main(void) //Es una tarea corriendo bajo un scheduler RTOS.
{
    configure_led(); //configura el GPIO del LED como salida y lo apaga inicialmente
    configure_button(); //configura el GPIO del botón como entrada con resistencia pull-up interna

    bool blink_enabled = false; //variables de control
    bool last_button_state = 1; //1 por el pull-up interna, el botón no presionado es HIGH (1)
    // y presionado es LOW (0)

    while (1)
    {
        bool current_button_state = gpio_get_level(BUTTON_GPIO); //lectura del estado actual del botón (HIGH o LOW)

        // Detectar flanco de bajada (HIGH → LOW)
        if (last_button_state == 1 && current_button_state == 0)
        {
            blink_enabled = !blink_enabled;
            ESP_LOGI(TAG, "Blink mode: %s", blink_enabled ? "ENABLED" : "DISABLED");
            vTaskDelay(pdMS_TO_TICKS(200));  // tarea debounce para evitar múltiples cambios 
            //de estado por un solo pulso del botón
        }

        last_button_state = current_button_state; //cambia el estado anterior del botón al 
        //estado actual para la próxima iteración

        if (blink_enabled)
        {
            s_led_state = !s_led_state; //cambia el estado del LED (si estaba apagado, lo enciende; 
            //si estaba encendido, lo apaga)
            gpio_set_level(LED_GPIO, s_led_state);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_BLINK_PERIOD));
        }
        else
        {
            gpio_set_level(LED_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}