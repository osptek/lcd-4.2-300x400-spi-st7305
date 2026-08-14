#include "board_lcd.h"

#include "board_config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "st7305_lcd.h"

static const char *TAG = "board_lcd";

#if BOARD_LCD_PATTERN_CYCLE || BOARD_ENABLE_TOUCH

static void draw_full_checkerboard(uint8_t cell)
{
    const uint16_t w = BOARD_LCD_H_RES;
    const uint16_t h = BOARD_LCD_V_RES;

    st7305_lcd_clear();
    if (cell == 0) {
        return;
    }

    const uint16_t cols = w / cell;
    const uint16_t rows = h / cell;

    for (uint16_t row = 0; row < rows; ++row) {
        for (uint16_t col = 0; col < cols; ++col) {
            if ((row + col) % 2) {
                continue;
            }
            st7305_lcd_fill_rect((uint16_t)(col * cell),
                                 (uint16_t)(row * cell),
                                 cell,
                                 cell,
                                 ST7305_COLOR_BLACK);
        }
    }
}

typedef enum {
    PAT_FULL_BLACK = 0,
    PAT_FULL_WHITE,
    PAT_CHECKER_4,
    PAT_CHECKER_8,
    PAT_CHECKER_16,
    PAT_CHECKER_32,
    PAT_CHECKER_64,
    PAT_BORDER_FRAME,
    PAT_COUNT,
} lcd_pattern_t;

#if BOARD_LCD_PATTERN_CYCLE
static const char *pattern_name(lcd_pattern_t p)
{
    switch (p) {
    case PAT_FULL_BLACK:
        return "FULL_BLACK";
    case PAT_FULL_WHITE:
        return "FULL_WHITE";
    case PAT_CHECKER_4:
        return "CHECKER_4";
    case PAT_CHECKER_8:
        return "CHECKER_8";
    case PAT_CHECKER_16:
        return "CHECKER_16";
    case PAT_CHECKER_32:
        return "CHECKER_32";
    case PAT_CHECKER_64:
        return "CHECKER_64";
    case PAT_BORDER_FRAME:
        return "BORDER_FRAME";
    default:
        return "?";
    }
}
#endif

static void render_pattern(lcd_pattern_t p)
{
    const uint16_t w = BOARD_LCD_H_RES;
    const uint16_t h = BOARD_LCD_V_RES;

    switch (p) {
    case PAT_FULL_BLACK:
        st7305_lcd_fill(0xFF);
        break;
    case PAT_FULL_WHITE:
        st7305_lcd_fill(0x00);
        break;
    case PAT_CHECKER_4:
        draw_full_checkerboard(4);
        break;
    case PAT_CHECKER_8:
        draw_full_checkerboard(8);
        break;
    case PAT_CHECKER_16:
        draw_full_checkerboard(16);
        break;
    case PAT_CHECKER_32:
        draw_full_checkerboard(32);
        break;
    case PAT_CHECKER_64:
        draw_full_checkerboard(64);
        break;
    case PAT_BORDER_FRAME:
        st7305_lcd_clear();
        st7305_lcd_fill_rect(0, 0, w, 6, ST7305_COLOR_BLACK);
        st7305_lcd_fill_rect(0, h - 6, w, 6, ST7305_COLOR_BLACK);
        st7305_lcd_fill_rect(0, 0, 6, h, ST7305_COLOR_BLACK);
        st7305_lcd_fill_rect(w - 6, 0, 6, h, ST7305_COLOR_BLACK);
        draw_full_checkerboard(12);
        break;
    default:
        st7305_lcd_fill(0x00);
        break;
    }
}

#endif /* BOARD_LCD_PATTERN_CYCLE || BOARD_ENABLE_TOUCH */

#if BOARD_LCD_PATTERN_CYCLE
static void pattern_cycle_task(void *arg)
{
    lcd_pattern_t idx = PAT_FULL_BLACK;
    (void)arg;

    vTaskDelay(pdMS_TO_TICKS(300));

    while (true) {
        render_pattern(idx);
        esp_err_t err = st7305_lcd_flush();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "flush %s: %s", pattern_name(idx), esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "pattern: %s", pattern_name(idx));
        }

        idx = (lcd_pattern_t)((idx + 1) % PAT_COUNT);
        vTaskDelay(pdMS_TO_TICKS(BOARD_LCD_PATTERN_HOLD_MS));
    }
}
#endif

esp_err_t board_lcd_init(void)
{
    ESP_RETURN_ON_ERROR(st7305_lcd_init(BOARD_PIN_LCD_RS,
                                        BOARD_PIN_LCD_RST,
                                        BOARD_PIN_LCD_CS,
                                        BOARD_PIN_LCD_CLK,
                                        BOARD_PIN_LCD_MOSI,
                                        BOARD_LCD_SPI_HOST,
                                        BOARD_LCD_SPI_HZ),
                        TAG,
                        "lcd init failed");

#if BOARD_LCD_PATTERN_CYCLE
    ESP_LOGI(TAG, "LCD pattern cycle: black/white/checker..., hold %d ms",
             BOARD_LCD_PATTERN_HOLD_MS);
    xTaskCreate(pattern_cycle_task, "lcd_pat", 4096, NULL, 5, NULL);
#else
#if BOARD_ENABLE_TOUCH
    render_pattern(PAT_BORDER_FRAME);
    ESP_RETURN_ON_ERROR(st7305_lcd_flush(), TAG, "flush failed");
#endif
    ESP_LOGI(TAG, "LCD ready %dx%d", BOARD_LCD_H_RES, BOARD_LCD_V_RES);
#endif

    return ESP_OK;
}
