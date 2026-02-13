idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "nvs_flash" "nvs_config" "wifi_manager"
                             "adc_driver" "ens210_driver" "zone_fsm"
                             "mqtt_client" "json" "esp_timer" "esp_wifi")
