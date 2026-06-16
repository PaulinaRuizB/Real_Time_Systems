/**
 * Application entry point.
 */

#include "nvs_flash.h"
#include "wifi_app.h"
#include "driver/gpio.h"
#include "servo.h"
#include "curtain.h"
#include "rgb_led.h"
#include "alarm_led.h"
#include "ntc_sensor.h"
#include "light_sensor.h"
#include "fan_control.h"
#include "system_state.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "automatic_control.h"

#include "http_server.h"

#define BLINK_GPIO 2

static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

	system_state_t system_state = {
		.current_temperature_c = 0.0f,
		.current_light_lux = 0.0f,

		.fan_mode = FAN_MODE_MANUAL,
		.fan_percent = 0,

		.desired_temperature_c = 25.0f,
		.maximum_temperature_c = 35.0f,

		.alarm_active = false,

		.curtain_position_percent = 0,

		.rgb_red = 0,
		.rgb_green = 255,
		.rgb_blue = 0,
		.rgb_brightness = 100
	};

    init_obtain_time();
    configure_led();

    curtain_init();

    alarm_led_init();
    alarm_led_set(false);

    ntc_sensor_init();
    light_sensor_init();

    fan_control_init();
    fan_control_set_percent(0);

	http_server_set_system_state(&system_state);

    wifi_app_start();

	while (1)
	{
		system_state.current_temperature_c =
			ntc_sensor_read_temperature_celsius();

		system_state.current_light_lux =
			light_sensor_read_lux();
		
		if (system_state.fan_mode == FAN_MODE_AUTOMATIC)
		{
			automatic_control_update_fan(&system_state);
		}

		if (system_state.current_temperature_c >=
			system_state.maximum_temperature_c)
		{
			system_state.alarm_active = true;
		}
		else
		{
			system_state.alarm_active = false;
			alarm_led_set(false);
		}

		if (system_state.alarm_active)
		{
			alarm_led_toggle();
		}

		ESP_LOGI(
			"SYSTEM_STATE",
			"Temp=%.2f C | Luz=%.2f lux | Fan=%d%% | Mode=%d | Alarm=%d",
			system_state.current_temperature_c,
			system_state.current_light_lux,
			system_state.fan_percent,
			system_state.fan_mode,
			system_state.alarm_active
		);

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}