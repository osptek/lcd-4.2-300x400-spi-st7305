#include "board_config.h"
#include "board_lcd.h"
#include "board_lcd_demo.h"
#include "board_touch.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bringup";

void app_main(void)
{
    ESP_LOGI(TAG, "4.2\" ST7305 400x300 bringup");

#if BOARD_ENABLE_TOUCH
    board_touch_prepare_pins();
#endif

    ESP_ERROR_CHECK(board_lcd_init());

#if BOARD_ENABLE_TOUCH
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_ERROR_CHECK(board_touch_init());
    board_touch_demo_start();
#else
    board_lcd_demo_start();
#endif
}
