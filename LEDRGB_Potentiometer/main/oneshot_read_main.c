
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"


const static char *TAG = "RGB_CTRL";

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
//ADC1 Channels
#if CONFIG_IDF_TARGET_ESP32
#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_4
#define EXAMPLE_ADC1_CHAN1          ADC_CHANNEL_5
#else
#define ADC_CHANNEL         ADC_CHANNEL_2
#endif

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12

// Botones de control 

#define BTN_R   GPIO_NUM_4
#define BTN_G   GPIO_NUM_5
#define BTN_B   GPIO_NUM_6
#define BTN_OK  GPIO_NUM_7

// LED RGB

#define LED_R GPIO_NUM_8
#define LED_G GPIO_NUM_9
#define LED_B GPIO_NUM_10

//PWM Config 
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES   LEDC_TIMER_8_BIT  
#define LEDC_FREQUENCY  5000

#define LEDC_CHANNEL_R  LEDC_CHANNEL_0
#define LEDC_CHANNEL_G  LEDC_CHANNEL_1
#define LEDC_CHANNEL_B  LEDC_CHANNEL_2

// variables de inicialización 
uint8_t duty_r = 0;
uint8_t duty_g = 0;
uint8_t duty_b = 0;

// Estructuras para estados de color y control
typedef enum {
    SELECT_NONE,
    SELECT_R,
    SELECT_G,
    SELECT_B
} color_select_t;

color_select_t current_color = SELECT_NONE; // Variable para almacenar el color seleccionado

//Modo de operación del sistema 
typedef enum {
    MODE_CONFIG,
    MODE_SHOW
} system_mode_t;

system_mode_t mode = MODE_CONFIG;

// Función para configurar el PWM 
void pwm_init()
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch[3] = {
        {.channel = LEDC_CHANNEL_0, .gpio_num = LED_R},
        {.channel = LEDC_CHANNEL_1, .gpio_num = LED_G},
        {.channel = LEDC_CHANNEL_2, .gpio_num = LED_B},
    };

    for (int i = 0; i < 3; i++) {
        ch[i].speed_mode = LEDC_MODE;
        ch[i].timer_sel = LEDC_TIMER_0;
        ch[i].duty = 0;
        ch[i].hpoint = 0;
        ledc_channel_config(&ch[i]);
    }
}

// Función para actualizar el PWM con el potenciometro 
void update_pwm()
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_R, duty_r);
    ledc_update_duty(LLEDC_MODE, LEDC_CHANNEL_R);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_G duty_g);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_G);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_B, duty_b);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_B);
}

//Función para configurar los botones de control
void gpio_init_buttons()
{
    gpio_config_t io = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pin_bit_mask = (1ULL<<BTN_R) | (1ULL<<BTN_G) | (1ULL<<BTN_B) | (1ULL<<BTN_OK)
    };
    gpio_config(&io);
}

//Función para escalar los datos tomados del ADC a PWM para el control del RGB
uint8_t map_adc_to_pwm(int adc_raw)
{
    return (adc_raw * 255) / 4095; // 12-bit ADC → 8-bit PWM
}

// Función para actualizar el PWM según selección 
void update_pwm_preview()
{
    uint8_t r = 0, g = 0, b = 0;

    if (current_color == SELECT_R) r = duty_r;
    if (current_color == SELECT_G) g = duty_g;
    if (current_color == SELECT_B) b = duty_b;

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_R, r);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_R);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_G, g);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_G);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_B, b);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_B);
}

// Funcionamiento principal 
void app_main(void)
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));

    pwm_init();
    gpio_init_buttons();
    
    int adc_raw;
    //Variables de control para que los logs se actualicen solo cuando hay cambios 
    static int last_color = -1;
    static int last_pwm = -1;
    static system_mode_t last_mode = -1; 

    while (1) {

        // 1. Leer Valor de ADC 
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw));
        uint8_t pwm_val = map_adc_to_pwm(adc_raw);
    }

    // ===== 2. Lectura de botones (edge-like con delay simple) =====
    if (!gpio_get_level(BTN_R)) {
        current_color = SELECT_R;
        mode = MODE_CONFIG;
        vTaskDelay(pdMS_TO_TICKS(200)); // debounce básico
    }

    if (!gpio_get_level(BTN_G)) {
        current_color = SELECT_G;
        mode = MODE_CONFIG;
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    if (!gpio_get_level(BTN_B)) {
        current_color = SELECT_B;
        mode = MODE_CONFIG;
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    if (!gpio_get_level(BTN_OK)) {
        mode = MODE_SHOW;
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    // ===== 3. Guardar valores (modo configuración) =====
    if (mode == MODE_CONFIG) {
        if (current_color == SELECT_R) duty_r = pwm_val;
        if (current_color == SELECT_G) duty_g = pwm_val;
        if (current_color == SELECT_B) duty_b = pwm_val;

        update_pwm_preview();  // solo muestra el canal activo
    }

    // ===== 4. Mostrar mezcla =====
    if (mode == MODE_SHOW) {
        update_pwm();  // mezcla RGB completa
    }

    // Cambio de color
    if (current_color != last_color) {
        ESP_LOGI(TAG, "Color actual: %d", current_color);
        last_color = current_color;
    }

    // Cambio de modo
    if (mode != last_mode) {
        if (mode == MODE_CONFIG)
            ESP_LOGI(TAG, "Modo: CONFIG");
        else
            ESP_LOGI(TAG, "Modo: SHOW");

        last_mode = mode;
    }

    // Cambio de PWM (con filtro anti-ruido)
    if (abs(pwm_val - last_pwm) > 2) {
        ESP_LOGI(TAG, "PWM: %d", pwm_val);
        last_pwm = pwm_val;
    }

    // ===== 6. Delay del sistema =====
    vTaskDelay(pdMS_TO_TICKS(50));
}
