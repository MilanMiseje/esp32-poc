#include "button.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_GPIO GPIO_NUM_27
#define BUTTON_DEBOUNCE_MS 30

static int stable_button_level;
static int last_button_level;
static TickType_t last_change;

esp_err_t button_init(void)
{
    const gpio_config_t button_config = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    const esp_err_t err = gpio_config(&button_config);
    if (err != ESP_OK) {
        return err;
    }

    stable_button_level = gpio_get_level(BUTTON_GPIO);
    last_button_level = stable_button_level;
    last_change = xTaskGetTickCount();
    return ESP_OK;
}

bool button_was_pressed(void)
{
    const int button_level = gpio_get_level(BUTTON_GPIO);
    const TickType_t now = xTaskGetTickCount();

    if (button_level != last_button_level) {
        last_button_level = button_level;
        last_change = now;
        return false;
    }

    if (button_level != stable_button_level &&
        (now - last_change) >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)) {
        stable_button_level = button_level;
        return stable_button_level == 0;
    }

    return false;
}
