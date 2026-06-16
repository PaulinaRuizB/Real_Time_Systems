#include "ntc_sensor.h"
#include "project_pins.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include <math.h>

#define NTC_R_FIXED_OHMS      10000.0f
#define NTC_R_NOMINAL_OHMS    10000.0f
#define NTC_TEMP_NOMINAL_K    298.15f
#define NTC_BETA              3950.0f
#define NTC_SUPPLY_VOLTAGE    3.3f

static const char *TAG = "NTC_SENSOR";

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle;
static bool adc_calibrated = false;

void ntc_sensor_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &channel_config));

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = ADC_CHANNEL_0,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    if (adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle) == ESP_OK)
    {
        adc_calibrated = true;
        ESP_LOGI(TAG, "ADC calibration enabled");
    }
    else
    {
        adc_calibrated = false;
        ESP_LOGW(TAG, "ADC calibration not available");
    }
}

uint32_t ntc_sensor_read_raw(void)
{
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &raw));
    return (uint32_t) raw;
}

float ntc_sensor_read_voltage(void)
{
    const int samples = 20;
    int raw = 0;
    int raw_sum = 0;
    int voltage_mv = 0;

    for (int i = 0; i < samples; i++)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &raw));
        raw_sum += raw;
    }

    int raw_average = raw_sum / samples;

    if (adc_calibrated)
    {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw_average, &voltage_mv));
        return voltage_mv / 1000.0f;
    }

    return (raw_average * 3.3f) / 4095.0f;
}

float ntc_sensor_read_temperature_celsius(void)
{
    float voltage = ntc_sensor_read_voltage();

    if (voltage <= 0.0f || voltage >= NTC_SUPPLY_VOLTAGE)
    {
        return -273.15f;
    }

    float ntc_resistance =
        (NTC_R_FIXED_OHMS * voltage) /
        (NTC_SUPPLY_VOLTAGE - voltage);

    float temperature_kelvin =
        1.0f /
        (
            (1.0f / NTC_TEMP_NOMINAL_K) +
            (1.0f / NTC_BETA) *
            logf(ntc_resistance / NTC_R_NOMINAL_OHMS)
        );

    return temperature_kelvin - 273.15f;
}