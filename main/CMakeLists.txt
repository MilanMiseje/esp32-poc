idf_component_register(SRCS "main.c" "led.c" "button.c" "wifi.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_common esp_driver_gpio esp_event esp_netif
                                  esp_wifi freertos log nvs_flash)
