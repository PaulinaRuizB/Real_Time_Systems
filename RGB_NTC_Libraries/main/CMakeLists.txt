idf_component_register(
    SRCS "Libraries.c" "oneshot_read_main.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_adc
)