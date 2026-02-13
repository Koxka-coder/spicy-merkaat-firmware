idf_component_register(SRCS "mqtt_client.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "mqtt" "esp_wifi" "esp_event")
