#include "lcd_sta.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>


static spi_device_handle_t spi_dev;

//=============================================================================
// 你原来的正确底层函数
//=============================================================================
void lcd_send_cmd(uint8_t cmd) {
    gpio_set_level(LCD_PIN_DC, 0);
    spi_transaction_t t = {.length = 8, .tx_buffer = &cmd};
    spi_device_transmit(spi_dev, &t);
}

void lcd_send_data(const uint8_t *data, size_t len) {
    gpio_set_level(LCD_PIN_DC, 1);
    spi_transaction_t t = {.length = len*8, .tx_buffer = data};
    spi_device_transmit(spi_dev, &t);
}

void lcd_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    lcd_send_cmd(0x2A);
    lcd_send_data((uint8_t[]){x1>>8, x1&0xFF, x2>>8, x2&0xFF}, 4);
    lcd_send_cmd(0x2B);
    lcd_send_data((uint8_t[]){y1>>8, y1&0xFF, y2>>8, y2&0xFF}, 4);
    lcd_send_cmd(0x2C);
}

void lcd_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    lcd_address_set(x1, y1, x2, y2);
    uint8_t col[2] = {color >> 8, color & 0xFF};
    uint32_t size = (x2-x1+1)*(y2-y1+1);

    for(uint32_t i=0; i<size; i++){
        lcd_send_data(col, 2);
    }
}

void lcd_clear(uint16_t color) {
    lcd_fill(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1, color);
}

// ===================== 你原来的正确初始化 =====================
void lcd_init(void) {
    gpio_config_t gpio_conf = {
        .pin_bit_mask = (1ULL<<LCD_PIN_RST)|(1ULL<<LCD_PIN_DC)|(1ULL<<LCD_PIN_BLK),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&gpio_conf);

    gpio_set_level(LCD_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(LCD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = LCD_PIN_MOSI,
        .sclk_io_num = LCD_PIN_SCLK,
        .miso_io_num = LCD_PIN_MISO,
    };
    spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_DISABLED);

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_FREQ,
        .mode = 3,
        .spics_io_num = LCD_PIN_CS,
        .queue_size = 16,
    };
    spi_bus_add_device(SPI3_HOST, &dev_cfg, &spi_dev);

    lcd_send_cmd(ST7789_SWRESET); vTaskDelay(pdMS_TO_TICKS(150));
    lcd_send_cmd(ST7789_SLPOUT);  vTaskDelay(pdMS_TO_TICKS(250));
    lcd_send_cmd(ST7789_COLMOD);  lcd_send_data((uint8_t[]){0x55}, 1);
    lcd_send_cmd(ST7789_MADCTL);  lcd_send_data((uint8_t[]){0x00}, 1);
    lcd_send_cmd(ST7789_DISPON);
    gpio_set_level(LCD_PIN_BLK, 1);
}

void lcd_show_str(uint16_t x, uint16_t y, char *str, uint16_t color, uint8_t size) {

}

void lcd_show_wifi_status(const char *status) {
    lcd_clear(BLACK);
}
