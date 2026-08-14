#include "board_lcd_demo.h"

#include "board_config.h"

#if !BOARD_ENABLE_TOUCH

#include "board_ui_draw.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "st7305_lcd.h"

static const char *TAG = "lcd_demo";

static const uint16_t s_border = 3;
static const uint16_t s_header_h = 32;
static const uint16_t s_blink_x = 360;
static const uint16_t s_blink_y = 276;
static const uint16_t s_blink_size = 10;

static void draw_border(void)
{
    const uint16_t w = BOARD_LCD_H_RES;
    const uint16_t h = BOARD_LCD_V_RES;
    const uint16_t b = s_border;

    st7305_lcd_fill_rect(0, 0, w, b, ST7305_COLOR_BLACK);
    st7305_lcd_fill_rect(0, h - b, w, b, ST7305_COLOR_BLACK);
    st7305_lcd_fill_rect(0, 0, b, h, ST7305_COLOR_BLACK);
    st7305_lcd_fill_rect(w - b, 0, b, h, ST7305_COLOR_BLACK);
}

static void draw_header(void)
{
    const uint16_t w = BOARD_LCD_H_RES;
    const char *title = "ST7305 BRINGUP";
    const uint16_t scale = 2;
    const uint16_t text_w = board_ui_text_width(title, scale);
    const uint16_t x = (w > text_w) ? (uint16_t)((w - text_w) / 2) : 0;
    const uint16_t y = (s_header_h - (7 * scale)) / 2;

    st7305_lcd_fill_rect(0, 0, w, s_header_h, ST7305_COLOR_BLACK);
    board_ui_draw_text(x, y, title, scale, ST7305_COLOR_WHITE);
}

static void draw_corner_bracket(uint16_t cx, uint16_t cy, uint16_t len, bool mirror_x, bool mirror_y)
{
    const uint16_t thick = 2;
    uint16_t hx;
    uint16_t hy;
    uint16_t vx;
    uint16_t vy;

    if (!mirror_x && !mirror_y) {
        hx = cx;
        hy = cy;
        vx = cx;
        vy = cy;
    } else if (mirror_x && !mirror_y) {
        hx = (uint16_t)(cx - len);
        hy = cy;
        vx = (uint16_t)(cx - thick);
        vy = cy;
    } else if (!mirror_x && mirror_y) {
        hx = cx;
        hy = (uint16_t)(cy - thick);
        vx = cx;
        vy = (uint16_t)(cy - len);
    } else {
        hx = (uint16_t)(cx - len);
        hy = (uint16_t)(cy - thick);
        vx = (uint16_t)(cx - thick);
        vy = (uint16_t)(cy - len);
    }

    st7305_lcd_fill_rect(hx, hy, len, thick, ST7305_COLOR_BLACK);
    st7305_lcd_fill_rect(vx, vy, thick, len, ST7305_COLOR_BLACK);
}

static void draw_alignment_markers(void)
{
    const uint16_t inset = 24;
    const uint16_t len = 28;
    const uint16_t w = BOARD_LCD_H_RES;
    const uint16_t h = BOARD_LCD_V_RES;
    const uint16_t top = s_header_h + 12;

    draw_corner_bracket(inset, top, len, false, false);
    draw_corner_bracket(w - inset, top, len, true, false);
    draw_corner_bracket(inset, h - inset, len, false, true);
    draw_corner_bracket(w - inset, h - inset, len, true, true);
}

static void draw_center_crosshair(void)
{
    const uint16_t cx = BOARD_LCD_H_RES / 2;
    const uint16_t cy = (BOARD_LCD_V_RES + s_header_h) / 2;
    const uint16_t arm = 18;
    const uint16_t thick = 2;

    st7305_lcd_fill_rect((uint16_t)(cx - arm), cy, arm * 2, thick, ST7305_COLOR_BLACK);
    st7305_lcd_fill_rect(cx, (uint16_t)(cy - arm), thick, arm * 2, ST7305_COLOR_BLACK);
}

static void draw_checker_patch(void)
{
    const uint16_t cell = 8;
    const uint16_t cols = 6;
    const uint16_t rows = 4;
    const uint16_t patch_w = cols * cell;
    const uint16_t patch_h = rows * cell;
    const uint16_t x = (BOARD_LCD_H_RES - patch_w) / 2;
    const uint16_t y = (BOARD_LCD_V_RES - patch_h) / 2 + 8;

    st7305_lcd_fill_rect(x - 2, y - 2, patch_w + 4, patch_h + 4, ST7305_COLOR_BLACK);
    st7305_lcd_fill_rect(x, y, patch_w, patch_h, ST7305_COLOR_WHITE);

    for (uint16_t row = 0; row < rows; ++row) {
        for (uint16_t col = 0; col < cols; ++col) {
            if ((row + col) % 2) {
                continue;
            }
            st7305_lcd_fill_rect((uint16_t)(x + col * cell),
                                 (uint16_t)(y + row * cell),
                                 cell,
                                 cell,
                                 ST7305_COLOR_BLACK);
        }
    }
}

static void draw_info_lines(void)
{
    const uint16_t scale = 2;
    const char *mode = "LCD ONLY";
    const char *res = "400x300";
    const char *ready = "READY";

    board_ui_draw_text(16, 48, mode, scale, ST7305_COLOR_BLACK);

    const uint16_t res_w = board_ui_text_width(res, 3);
    const uint16_t res_x = (BOARD_LCD_H_RES > res_w) ? (uint16_t)((BOARD_LCD_H_RES - res_w) / 2) : 0;
    board_ui_draw_text(res_x, 210, res, 3, ST7305_COLOR_BLACK);

    board_ui_draw_text(16, 276, ready, scale, ST7305_COLOR_BLACK);
}

static void draw_static_ui(void)
{
    st7305_lcd_clear();
    draw_border();
    draw_header();
    draw_alignment_markers();
    draw_checker_patch();
    draw_center_crosshair();
    draw_info_lines();
    st7305_lcd_fill_rect(s_blink_x, s_blink_y, s_blink_size, s_blink_size, ST7305_COLOR_BLACK);
}

static void lcd_demo_task(void *arg)
{
    (void)arg;
    bool blink_on = true;

    vTaskDelay(pdMS_TO_TICKS(200));

    while (true) {
        st7305_lcd_fill_rect(s_blink_x, s_blink_y, s_blink_size, s_blink_size,
                             blink_on ? ST7305_COLOR_BLACK : ST7305_COLOR_WHITE);
        st7305_lcd_flush();
        blink_on = !blink_on;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void board_lcd_demo_start(void)
{
    draw_static_ui();
    st7305_lcd_flush();
    xTaskCreate(lcd_demo_task, "lcd_demo", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "LCD bringup UI started");
}

#else

void board_lcd_demo_start(void) {}

#endif
