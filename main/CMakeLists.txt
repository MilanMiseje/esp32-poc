idf_component_register(SRCS "main.c" "led.c" "button.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_common esp_driver_gpio freertos)
