#include "board_touch.h"

#include "board_config.h"

#if BOARD_ENABLE_TOUCH

#include <stdio.h>

#include "board_touch_priv.h"
#include "board_ui_draw.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "st7305_lcd.h"

static const char *TAG = "touch_demo";

static void map_touch_point(const ft3269_touch_point_t *in, uint16_t *x, uint16_t *y)
{
    *x = in->y;
    *y = in->x;
    if (*y < BOARD_LCD_V_RES) {
        *y = (BOARD_LCD_V_RES - 1) - *y;
    }
    if (*x >= BOARD_LCD_H_RES) {
        *x = BOARD_LCD_H_RES - 1;
    }
    if (*y >= BOARD_LCD_V_RES) {
        *y = BOARD_LCD_V_RES - 1;
    }
}

static void draw_touch_cross(uint16_t cx, uint16_t cy)
{
    const uint16_t thick = BOARD_TOUCH_CROSS_THICK;
    uint16_t horiz_y = (cy > (thick / 2)) ? (cy - (thick / 2)) : 0;
    uint16_t horiz_h = thick;
    if (horiz_y + horiz_h > BOARD_LCD_V_RES) {
        horiz_h = BOARD_LCD_V_RES - horiz_y;
    }

    uint16_t vert_x = (cx > (thick / 2)) ? (cx - (thick / 2)) : 0;
    uint16_t vert_w = thick;
    if (vert_x + vert_w > BOARD_LCD_H_RES) {
        vert_w = BOARD_LCD_H_RES - vert_x;
    }

    st7305_lcd_fill_rect(0, horiz_y, BOARD_LCD_H_RES, horiz_h, ST7305_COLOR_BLACK);
    st7305_lcd_fill_rect(vert_x, 0, vert_w, BOARD_LCD_V_RES, ST7305_COLOR_BLACK);
}

static void draw_coord_text(uint16_t x, uint16_t y)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%u,%u", x, y);

    const uint16_t scale = 3;
    const uint16_t text_w = board_ui_text_width(buf, scale);
    const uint16_t text_h = 7 * scale + 4;

    st7305_lcd_fill_rect(0, 0, text_w + 8, text_h, ST7305_COLOR_WHITE);
    board_ui_draw_text(4, 2, buf, scale, ST7305_COLOR_BLACK);
}

static void touch_poll_task(void *arg)
{
    (void)arg;
    ft3269_touch_point_t points[5];

    while (true) {
        uint8_t count = 0;
        esp_err_t err = board_touch_read_raw_points(points, 5, &count);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "read_points: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if (count > 0) {
            st7305_lcd_clear();
            for (uint8_t i = 0; i < count; ++i) {
                uint16_t x = 0;
                uint16_t y = 0;
                map_touch_point(&points[i], &x, &y);
                ESP_LOGI(TAG, "touch[%u] x=%u y=%u", i, x, y);
                draw_touch_cross(x, y);
                draw_coord_text(x, y);
            }
            st7305_lcd_flush();
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void board_touch_demo_start(void)
{
    st7305_lcd_clear();
    st7305_lcd_flush();
    xTaskCreate(touch_poll_task, "touch_poll", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Touch crosshair demo started");
}

#else

void board_touch_demo_start(void) {}

#endif
