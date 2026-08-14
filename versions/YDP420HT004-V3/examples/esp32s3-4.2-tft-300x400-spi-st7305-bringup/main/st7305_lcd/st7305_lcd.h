#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

#define ST7305_COLOR_WHITE        (0)
#define ST7305_COLOR_BLACK        (1)

esp_err_t st7305_lcd_init(gpio_num_t dc,
                          gpio_num_t rst,
                          gpio_num_t cs,
                          gpio_num_t sclk,
                          gpio_num_t mosi,
                          spi_host_device_t host,
                          int spi_clock_hz);

void st7305_lcd_fill(uint8_t pattern);
void st7305_lcd_clear(void);
void st7305_lcd_write_point(uint16_t x, uint16_t y, uint16_t color);
void st7305_lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void st7305_lcd_invert_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
esp_err_t st7305_lcd_flush(void);
    