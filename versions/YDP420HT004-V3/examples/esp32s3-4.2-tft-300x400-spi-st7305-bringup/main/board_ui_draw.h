#pragma once

#include <stdint.h>

#include "st7305_lcd.h"

void board_ui_draw_glyph(uint16_t x, uint16_t y, char c, uint16_t scale, uint16_t color);
void board_ui_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t scale, uint16_t color);
uint16_t board_ui_text_width(const char *text, uint16_t scale);
