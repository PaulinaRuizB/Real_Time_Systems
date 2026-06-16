idf_component_register(SRCS "servo.c" "main.c" "rgb_led.c" "wifi_app.c" "http_server.c" "curtain.c" "alarm_led.c" "ntc_sensor.c" "light_sensor.c" "fan_control.c" "automatic_control.c"
                    PRIV_REQUIRES esp_wifi nvs_flash driver esp_http_server esp_timer app_update cjson mqtt esp_driver_gpio esp_driver_ledc esp_adc esp_driver_i2c

                    INCLUDE_DIRS "."
                    EMBED_FILES webpage/app.css webpage/app.js webpage/favicon.ico webpage/index.html webpage/jquery-3.3.1.min.js
                    )