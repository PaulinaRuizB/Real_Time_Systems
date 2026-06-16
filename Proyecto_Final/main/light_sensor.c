#include "light_sensor.h"
#include "project_pins.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BH1750_ADDR              0x23
#define BH1750_CMD_POWER_ON      0x01
#define BH1750_CMD_RESET         0x07
#define BH1750_CMD_CONT_HIGH_RES 0x10

static const char *TAG = "LIGHT_SENSOR";

static i2c_master_bus_handle_t i2c_bus_handle;
static i2c_master_dev_handle_t bh1750_handle;

static void light_sensor_write_cmd(uint8_t command)
{
    ESP_ERROR_CHECK(
        i2c_master_transmit(
            bh1750_handle,
            &command,
            1,
            1000
        )
    );
}

void light_sensor_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BH1750_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &bh1750_handle));

    light_sensor_write_cmd(BH1750_CMD_POWER_ON);
    vTaskDelay(pdMS_TO_TICKS(10));

    light_sensor_write_cmd(BH1750_CMD_RESET);
    vTaskDelay(pdMS_TO_TICKS(10));

    light_sensor_write_cmd(BH1750_CMD_CONT_HIGH_RES);
    vTaskDelay(pdMS_TO_TICKS(180));

    ESP_LOGI(TAG, "GY-30 / BH1750 initialized");
}

float light_sensor_read_lux(void)
{
    uint8_t data[2] = {0};

    ESP_ERROR_CHECK(
        i2c_master_receive(
            bh1750_handle,
            data,
            2,
            1000
        )
    );

    uint16_t raw = ((uint16_t)data[0] << 8) | data[1];

    return raw / 1.2f;
}