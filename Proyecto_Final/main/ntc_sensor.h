#ifndef NTC_SENSOR_H
#define NTC_SENSOR_H

#include <stdint.h>

void ntc_sensor_init(void);
uint32_t ntc_sensor_read_raw(void);
float ntc_sensor_read_voltage(void);

float ntc_sensor_read_temperature_celsius(void);

#endif