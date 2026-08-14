#pragma once

#include "esp_err.h"

void board_touch_prepare_pins(void);
esp_err_t board_touch_init(void);
void board_touch_demo_start(void);
