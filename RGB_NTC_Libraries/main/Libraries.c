#include <Libraries.h> 

static const char *TAG = "NTC_SYSTEM"; 

QueueHandle_t temp_queue; 
QueueHandle_t cmd_queue; 

static adc_oneshot_unit_handle_t adc_handle; 
static adc_cali_handle_t adc_cali_handle; 
static bool do_calibration = false;

