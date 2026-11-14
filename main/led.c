#include "led.h"

#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_23

static bool led_is_on;

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
    if (err == ESP_OK) {
        led_is_on = false;
    }
    return err;
}

esp_err_t led_toggle(void)
{
    const bool next_state = !led_is_on;
    const esp_err_t err = gpio_set_level(LED_GPIO, next_state);
    if (err == ESP_OK) {
        led_is_on = next_state;
    }
    return err;
}
