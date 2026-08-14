/*
 * ST7305 SPI driver for Waveshare 4.2" landscape mode.
 * The 400x300 panel uses a 2x4 pixel packing per byte.
 */

#include "st7305_lcd.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "board_config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "st7305_lcd";

#define LCD_WIDTH               BOARD_LCD_H_RES
#define LCD_HEIGHT              BOARD_LCD_V_RES
#define LCD_DATA_WIDTH          (LCD_WIDTH / 2)
#define LCD_DATA_HEIGHT         (LCD_HEIGHT / 4)
#define DISPLAY_BUFFER_LENGTH   (LCD_DATA_WIDTH * LCD_DATA_HEIGHT)

static gpio_num_t s_dc;
static gpio_num_t s_rst;
#if BOARD_LCD_CS_MANUAL
static gpio_num_t s_cs;
#endif
static spi_device_handle_t s_spi_dev;
static uint8_t *s_buf;

#if BOARD_LCD_CS_MANUAL
static void cs_low(void)
{
    gpio_set_level(s_cs, 0);
}

static void cs_high(void)
{
    gpio_set_level(s_cs, 1);
}

static esp_err_t spi_send(const uint8_t *data, size_t len)
{
    cs_low();
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    esp_err_t err = spi_device_polling_transmit(s_spi_dev, &t);
    cs_high();
    return err;
}
#endif

static esp_err_t write_cmd(uint8_t cmd)
{
    gpio_set_level(s_dc, 0);
#if BOARD_LCD_CS_MANUAL
    return spi_send(&cmd, 1);
#else
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    return spi_device_polling_transmit(s_spi_dev, &t);
#endif
}

static esp_err_t write_param(uint8_t p)
{
    gpio_set_level(s_dc, 1);
#if BOARD_LCD_CS_MANUAL
    return spi_send(&p, 1);
#else
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &p,
    };
    return spi_device_polling_transmit(s_spi_dev, &t);
#endif
}

static esp_err_t write_data(const uint8_t *data, size_t len)
{
    gpio_set_level(s_dc, 1);
#if BOARD_LCD_CS_MANUAL
    return spi_send(data, len);
#else
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    return spi_device_polling_transmit(s_spi_dev, &t);
#endif
}

static void panel_reset(void)
{
    gpio_set_level(s_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(s_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(s_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static void set_address_window(void)
{
    write_cmd(0x2A);
    write_param(BOARD_ST7305_COL_START);
    write_param(BOARD_ST7305_COL_END);
    write_cmd(0x2B);
    write_param(BOARD_ST7305_ROW_START);
    write_param(BOARD_ST7305_ROW_END);
    write_cmd(0x2C);
}

/* examples Initial_ST7305 (219Q168384) */
static void panel_init_st7305(void)
{
    write_cmd(0xD6);
    write_param(0x13);
    write_param(0x02);
    write_cmd(0xD1);
    write_param(0x01);

    write_cmd(0xC0);
    write_param(0x12);
    write_param(0x0A);
    write_cmd(0xC1);
    write_param(115);
    write_param(0x3E);
    write_param(0x3C);
    write_param(0x3C);
    write_cmd(0xC2);
    write_param(0x00);
    write_param(0x21);
    write_param(0x23);
    write_param(0x23);
    write_cmd(0xC4);
    write_param(50);
    write_param(0x5C);
    write_param(0x5A);
    write_param(0x5A);
    write_cmd(0xC5);
    write_param(50);
    write_param(0x35);
    write_param(0x37);
    write_param(0x37);

    write_cmd(0xD8);
    write_param(0x80);
    write_param(0xE9);
    write_cmd(0xB2);
    write_param(0x12);

    write_cmd(0xB3);
    {
        const uint8_t b3[] = {0xE5, 0xF6, 0x17, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x71};
        write_data(b3, sizeof(b3));
    }

    write_cmd(0xB4);
    {
        const uint8_t b4[] = {0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
        write_data(b4, sizeof(b4));
    }

    write_cmd(0x62);
    {
        const uint8_t gt[] = {0x32, 0x03, 0x1F};
        write_data(gt, sizeof(gt));
    }

    write_cmd(0xB7);
    write_param(0x13);

    write_cmd(0xB0);
    write_param(BOARD_ST7305_B0_PARAM);

    write_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    write_cmd(0xC9);
    write_param(0x00);
    write_cmd(0x36);
    write_param(BOARD_ST7305_MADCTL);
    write_cmd(0x3A);
    write_param(0x11);
    write_cmd(0xB9);
    write_param(0x20);
    write_cmd(0xB8);
    write_param(0x29);

    write_cmd(0x2A);
    write_param(BOARD_ST7305_COL_START);
    write_param(BOARD_ST7305_COL_END);
    write_cmd(0x2B);
    write_param(BOARD_ST7305_ROW_START);
    write_param(BOARD_ST7305_ROW_END);

    write_cmd(0x35);
    write_param(0x00);
    write_cmd(0xD0);
    write_param(0xFF);
    write_cmd(0x38);

    write_cmd(0x29);
    write_cmd(0x20);

    write_cmd(0xBB);
    write_param(0x4F);
}

/* examples High_Power_Mode */
static void panel_high_power_mode(void)
{
    write_cmd(0x38);
    vTaskDelay(pdMS_TO_TICKS(300));

    write_cmd(0xC1);
    write_param(115);
    write_param(0x3E);
    write_param(0x3C);
    write_param(0x3C);
    write_cmd(0xC2);
    write_param(0x00);
    write_param(0x21);
    write_param(0x23);
    write_param(0x23);
    write_cmd(0xC4);
    write_param(50);
    write_param(0x5C);
    write_param(0x5A);
    write_param(0x5A);
    write_cmd(0xC5);
    write_param(50);
    write_param(0x35);
    write_param(0x37);
    write_param(0x37);
    write_cmd(0xC9);
    write_param(0x00);
    vTaskDelay(pdMS_TO_TICKS(20));
}

static void panel_display_on(bool on)
{
    write_cmd(on ? 0x29 : 0x28);
}

esp_err_t st7305_lcd_init(gpio_num_t dc,
                          gpio_num_t rst,
                          gpio_num_t cs,
                          gpio_num_t sclk,
                          gpio_num_t mosi,
                          spi_host_device_t host,
                          int spi_clock_hz)
{
    s_dc = dc;
    s_rst = rst;
#if BOARD_LCD_CS_MANUAL
    s_cs = cs;
#endif

    s_buf = heap_caps_malloc(DISPLAY_BUFFER_LENGTH, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!s_buf) {
        return ESP_ERR_NO_MEM;
    }
    memset(s_buf, 0x00, DISPLAY_BUFFER_LENGTH);

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << dc) | (1ULL << rst),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
#if BOARD_LCD_CS_MANUAL
    io.pin_bit_mask |= (1ULL << cs);
#endif
    ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "gpio config failed");
#if BOARD_LCD_CS_MANUAL
    gpio_set_level(s_cs, 1);
#endif

    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi,
        .miso_io_num = -1,
        .sclk_io_num = sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_BUFFER_LENGTH + 16,
    };
    esp_err_t err = spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = spi_clock_hz,
        .mode = 0,
#if BOARD_LCD_CS_MANUAL
        .spics_io_num = -1,
#else
        .spics_io_num = cs,
#endif
        .queue_size = 4,
        .flags = SPI_DEVICE_NO_DUMMY,
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(host, &devcfg, &s_spi_dev), TAG, "spi add device failed");

    panel_reset();
    panel_init_st7305();
    panel_high_power_mode();
    panel_display_on(true);
    write_cmd(0x20);
    st7305_lcd_clear();

    ESP_LOGI(TAG,
             "ST7305 %dx%d buf=%u col %02X-%02X row %02X-%02X B0=%02X MADCTL=%02X spi=%dHz",
             LCD_WIDTH,
             LCD_HEIGHT,
             (unsigned)DISPLAY_BUFFER_LENGTH,
             BOARD_ST7305_COL_START,
             BOARD_ST7305_COL_END,
             BOARD_ST7305_ROW_START,
             BOARD_ST7305_ROW_END,
             BOARD_ST7305_B0_PARAM,
             BOARD_ST7305_MADCTL,
             spi_clock_hz);
    return ESP_OK;
}

void st7305_lcd_fill(uint8_t pattern)
{
    memset(s_buf, pattern, DISPLAY_BUFFER_LENGTH);
}

void st7305_lcd_clear(void)
{
    st7305_lcd_fill(0x00);
}

static bool map_x(uint16_t x, uint16_t *out_x)
{
    int32_t mx = (int32_t)x + BOARD_ST7305_COL_OFFSET;
    if (mx < 0 || mx >= LCD_WIDTH) {
        return false;
    }
    *out_x = (uint16_t)mx;
    return true;
}

void st7305_lcd_write_point(uint16_t x, uint16_t y, uint16_t color)
{
    if (!map_x(x, &x) || y >= LCD_HEIGHT) {
        return;
    }

    uint16_t byte_x = x / 2;
    uint16_t inv_y = (uint16_t)(LCD_HEIGHT - 1 - y);
    uint16_t block_y = inv_y / 4;
    uint32_t idx = (uint32_t)byte_x * LCD_DATA_HEIGHT + block_y;

    uint8_t local_x = x % 2;
    uint8_t local_y = inv_y % 4;
    uint8_t write_bit = 7 - (uint8_t)((local_y << 1) | local_x);

    if (color) {
        s_buf[idx] |= (1U << write_bit);
    } else {
        s_buf[idx] &= ~(1U << write_bit);
    }
}

void st7305_lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (w == 0 || h == 0) {
        return;
    }

    if (!map_x(x, &x)) {
        return;
    }
    if (y >= LCD_HEIGHT) {
        return;
    }
    if (x + w > LCD_WIDTH) {
        w = LCD_WIDTH - x;
    }
    if (y + h > LCD_HEIGHT) {
        h = LCD_HEIGHT - y;
    }

    for (uint16_t py = y; py < y + h; ++py) {
        for (uint16_t px = x; px < x + w; ++px) {
            st7305_lcd_write_point(px, py, color);
        }
    }
}

void st7305_lcd_invert_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if (w == 0 || h == 0) {
        return;
    }
    if (!map_x(x, &x) || y >= LCD_HEIGHT) {
        return;
    }
    if (x + w > LCD_WIDTH) {
        w = LCD_WIDTH - x;
    }
    if (y + h > LCD_HEIGHT) {
        h = LCD_HEIGHT - y;
    }

    for (uint16_t py = y; py < y + h; ++py) {
        for (uint16_t px = x; px < x + w; ++px) {
            uint16_t byte_x = px / 2;
            uint16_t inv_y = (uint16_t)(LCD_HEIGHT - 1 - py);
            uint16_t block_y = inv_y / 4;
            uint32_t idx = (uint32_t)byte_x * LCD_DATA_HEIGHT + block_y;
            uint8_t local_x = px % 2;
            uint8_t local_y = inv_y % 4;
            uint8_t bit = 7 - (uint8_t)((local_y << 1) | local_x);
            s_buf[idx] ^= (uint8_t)(1U << bit);
        }
    }
}

esp_err_t st7305_lcd_flush(void)
{
    set_address_window();
    gpio_set_level(s_dc, 1);
#if BOARD_LCD_CS_MANUAL
    cs_low();
#endif

    esp_err_t err = ESP_OK;
    size_t sent = 0;
    while (sent < DISPLAY_BUFFER_LENGTH) {
        size_t chunk = DISPLAY_BUFFER_LENGTH - sent;
        if (chunk > BOARD_LCD_SPI_CHUNK_BYTES) {
            chunk = BOARD_LCD_SPI_CHUNK_BYTES;
        }
        spi_transaction_t t = {
            .length = chunk * 8,
            .tx_buffer = s_buf + sent,
        };
        err = spi_device_polling_transmit(s_spi_dev, &t);
        if (err != ESP_OK) {
            break;
        }
        sent += chunk;
    }

#if BOARD_LCD_CS_MANUAL
    cs_high();
#endif
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "flush failed at %u/%u: %s",
                 (unsigned)sent,
                 (unsigned)DISPLAY_BUFFER_LENGTH,
                 esp_err_to_name(err));
    }
    return err;
}
