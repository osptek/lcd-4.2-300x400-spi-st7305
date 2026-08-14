#pragma once

#include "esp_err.h"
#include "ft3269_touch.h"

esp_err_t board_touch_read_raw_points(ft3269_touch_point_t *points,
                                      uint8_t max_points,
                                      uint8_t *point_count);
