#include "servo.h"

#include "driver/ledc.h"
#include "esp_log.h"

#define SERVO_GPIO         4

#define SERVO_FREQ_HZ      50

#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2500

#define SERVO_TIMER        LEDC_TIMER_1
#define SERVO_CHANNEL      LEDC_CHANNEL_3

static const char *TAG = "SERVO";

static uint8_t current_angle = 0;
static bool servo_initialized = false;


/**
 * Convierte ángulo a ancho de pulso
 */
static uint32_t servo_angle_to_us(uint8_t angle)
{
    return SERVO_MIN_PULSE_US +
           ((SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * angle) / 180;
}


/**
 * Inicializa PWM del servo
 */
void servo_init(void)
{
    if (servo_initialized)
    {
        return;
    }

    ledc_timer_config_t timer_cfg =
    {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = SERVO_TIMER,
        .duty_resolution  = LEDC_TIMER_14_BIT,
        .freq_hz          = SERVO_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(
        ledc_timer_config(&timer_cfg)
    );

    ledc_channel_config_t channel_cfg =
    {
        .gpio_num   = SERVO_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = SERVO_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = SERVO_TIMER,
        .duty       = 0,
        .hpoint     = 0
    };

    ESP_ERROR_CHECK(
        ledc_channel_config(&channel_cfg)
    );

    servo_initialized = true;

    ESP_LOGI(TAG, "Servo initialized");
}


/**
 * Mueve el servo
 */
void servo_set_angle(uint8_t angle)
{
    if (angle > 180)
    {
        angle = 180;
    }

    current_angle = angle;

    uint32_t pulse_us =
        servo_angle_to_us(angle);

    uint32_t duty =
        (pulse_us * ((1 << 14) - 1))
        / 20000;

    ledc_set_duty(
        LEDC_LOW_SPEED_MODE,
        SERVO_CHANNEL,
        duty);

    ledc_update_duty(
        LEDC_LOW_SPEED_MODE,
        SERVO_CHANNEL);

    ESP_LOGI(
        TAG,
        "Angle=%d Pulse=%lu Duty=%lu",
        angle,
        pulse_us,
        duty);
}


uint8_t servo_get_angle(void)
{
    return current_angle;
}