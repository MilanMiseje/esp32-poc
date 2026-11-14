#include "led.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LED_GPIO GPIO_NUM_23
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RESOLUTION LEDC_TIMER_10_BIT
#define LEDC_DUTY_MAX ((1U << LEDC_DUTY_RESOLUTION) - 1U)
#define LEDC_FREQUENCY_HZ 5000

static bool led_is_on;
static uint8_t led_brightness_percent = 100;
static SemaphoreHandle_t led_mutex;

static esp_err_t led_apply_duty(uint32_t duty)
{
    esp_err_t err = ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    if (err != ESP_OK) {
        return err;
    }
    return ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

static uint32_t led_brightness_to_duty(uint8_t percent)
{
    const uint32_t value = percent;
    return (value * value * LEDC_DUTY_MAX + 5000U) / 10000U;
}

esp_err_t led_init(void)
{
    led_mutex = xSemaphoreCreateMutex();
    if (led_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RESOLUTION,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    esp_err_t err = ledc_timer_config(&timer_config);
    if (err != ESP_OK) {
        return err;
    }

    const ledc_channel_config_t channel_config = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .gpio_num = LED_GPIO,
        .duty = 0,
        .hpoint = 0,
    };

    err = ledc_channel_config(&channel_config);
    if (err == ESP_OK) {
        led_is_on = false;
        led_brightness_percent = 100;
    }
    return err;
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
    const uint32_t duty = next_state
                              ? led_brightness_to_duty(led_brightness_percent)
                              : 0;
    const esp_err_t err = led_apply_duty(duty);
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

esp_err_t led_set_brightness(uint8_t percent)
{
    if (percent > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    if (led_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(led_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    const uint32_t duty = led_is_on ? led_brightness_to_duty(percent) : 0;
    const esp_err_t err = led_apply_duty(duty);
    if (err == ESP_OK) {
        led_brightness_percent = percent;
    }
    xSemaphoreGive(led_mutex);
    return err;
}

esp_err_t led_get_brightness(uint8_t *percent)
{
    if (percent == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (led_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(led_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    *percent = led_brightness_percent;
    xSemaphoreGive(led_mutex);
    return ESP_OK;
}
