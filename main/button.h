#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t button_init(void);
bool button_was_pressed(void);
