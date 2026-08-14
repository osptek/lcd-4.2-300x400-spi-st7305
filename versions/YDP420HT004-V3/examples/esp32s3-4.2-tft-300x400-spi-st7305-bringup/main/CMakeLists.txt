idf_component_register(SRCS "board_lcd_demo.c" "board_ui_draw.c" "main.c"
                            "board_lcd.c"
                            "board_touch.c"
                            "board_touch_demo.c"
                            "ft3269_touch/ft3269_touch.c"
                            "st7305_lcd/st7305_lcd.c"
                    INCLUDE_DIRS "." "ft3269_touch" "st7305_lcd"
                    REQUIRES driver esp_driver_gpio esp_driver_i2c esp_driver_spi)
