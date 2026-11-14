#include "led.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LED_GPIO GPIO_NUM_23

static bool led_is_on;
static SemaphoreHandle_t led_mutex;

esp_err_t led_init(void)
{
    const gpio_config_t led_config = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&led_config);
    if (err != ESP_OK) {
        return err;
    }

    err = gpio_set_level(LED_GPIO, 0);
    if (err != ESP_OK) {
        return err;
    }

    led_is_on = false;
    led_mutex = xSemaphoreCreateMutex();
    return led_mutex == NULL ? ESP_ERR_NO_MEM : ESP_OK;
}

static esp_err_t led_update(bool toggle, bool requested_state)
{
    if (led_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(led_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    const bool next_state = toggle ? !led_is_on : requested_state;
    const esp_err_t err = gpio_set_level(LED_GPIO, next_state);
    if (err == ESP_OK) {
        led_is_on = next_state;
    }
    xSemaphoreGive(led_mutex);
    return err;
}

esp_err_t led_toggle(void)
{
    return led_update(true, false);
}

esp_err_t led_set_state(bool is_on)
{
    return led_update(false, is_on);
}
