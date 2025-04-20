idf_component_register(SRCS "app_main.c" "dv8_mqtt.c" "led_expressions.c" 
                    PRIV_REQUIRES mqtt nvs_flash esp_netif json 
                    INCLUDE_DIRS "include")
