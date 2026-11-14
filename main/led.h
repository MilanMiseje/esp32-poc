#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t led_init(void);
esp_err_t led_toggle(void);
esp_err_t led_set_state(bool is_on);
