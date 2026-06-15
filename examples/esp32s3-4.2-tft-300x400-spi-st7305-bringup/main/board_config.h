#pragma once

#include "driver/gpio.h"

/* 1=启用 FT3269 触摸与画点演示，0=仅 LCD */
#define BOARD_ENABLE_TOUCH        (0)

#define BOARD_LCD_H_RES           (400)
#define BOARD_LCD_V_RES           (300)

/* LCD 4 线 SPI：CS=9 RST=10 TE=11 CLK=12 MOSI=13 RS(DC)=14 */
#define BOARD_PIN_LCD_CS          (GPIO_NUM_9)
#define BOARD_PIN_LCD_RST         (GPIO_NUM_10)
#define BOARD_PIN_LCD_TE          (GPIO_NUM_11)
#define BOARD_PIN_LCD_CLK         (GPIO_NUM_12)
#define BOARD_PIN_LCD_MOSI        (GPIO_NUM_13)
#define BOARD_PIN_LCD_RS          (GPIO_NUM_14)

/* 触摸 I2C FT3269 @ 0x38 */
#define BOARD_PIN_TOUCH_SDA       (GPIO_NUM_39)
#define BOARD_PIN_TOUCH_SCL       (GPIO_NUM_40)
#define BOARD_PIN_TOUCH_RST       (GPIO_NUM_41)
#define BOARD_PIN_TOUCH_INT       (GPIO_NUM_42)

#define BOARD_TOUCH_RST_ACTIVE    (0)
#define BOARD_TOUCH_I2C_HZ        (100000)
#define BOARD_TOUCH_I2C_TIMEOUT_MS (200)

#define BOARD_LCD_SPI_HOST        SPI2_HOST
#define BOARD_LCD_SPI_HZ          (40000000)
#define BOARD_LCD_CS_MANUAL       (0)

/* 单次 SPI 写入上限（整屏 15000 字节需分片） */
#define BOARD_LCD_SPI_CHUNK_BYTES (4096)

/* Waveshare 4.2" ST7305 参考窗口: 400x300 landscape */
#define BOARD_ST7305_COL_START    (0x12)
#define BOARD_ST7305_COL_END      (0x2A)
#define BOARD_ST7305_ROW_START    (0x00)
#define BOARD_ST7305_ROW_END      (0xC7)
#define BOARD_ST7305_B0_PARAM     (0x64)

#define BOARD_ST7305_MADCTL       (0x48)

#define BOARD_ST7305_COL_OFFSET   (0)

#define BOARD_LCD_PATTERN_CYCLE   (0)
#define BOARD_LCD_PATTERN_HOLD_MS (1500)

#define BOARD_TOUCH_CROSS_THICK   (3)
