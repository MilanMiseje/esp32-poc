#include "button.h"
#include "led.h"

#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define POLL_INTERVAL_MS 10

void app_main(void)
{
    ESP_ERROR_CHECK(led_init());
    ESP_ERROR_CHECK(button_init());

    while (1) {
        if (button_was_pressed()) {
            ESP_ERROR_CHECK(led_toggle());
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}
