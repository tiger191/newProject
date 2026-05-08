#include "lcd_sta.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>

// #include "lvgl.h"

// 引脚与屏幕硬件配置（根据你的ESP32硬件接线定义）
#define LCD_PIN_SCLK    21
#define LCD_PIN_MOSI    47
#define LCD_PIN_MISO    -1
#define LCD_PIN_CS      41
#define LCD_PIN_DC      40
#define LCD_PIN_RST     45
#define LCD_PIN_BLK     42

#define LCD_WIDTH       240
#define LCD_HEIGHT      240
#define SPI_FREQ        40*1000*1000

// ST7789 屏幕驱动命令（芯片手册定义的固定指令）
#define ST7789_SWRESET   0x01    // 软件复位命令
#define ST7789_SLPOUT    0x11    // 退出睡眠模式
#define ST7789_COLMOD    0x3A    // 设置颜色格式
#define ST7789_MADCTL    0x36    // 设置显示方向
#define ST7789_DISPON    0x29    // 开启显示
#define ST7789_CASET     0x2A    // 设置列地址范围
#define ST7789_RASET     0x2B    // 设置行地址范围
#define ST7789_RAMWR     0x2C    // 写入显存数据

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

// void lv_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
//     // 取出需要刷新的区域坐标
//     int16_t x1 = area->x1, y1 = area->y1, x2 = area->x2, y2 = area->y2;

//     // 设置列地址范围（X轴）
//     lcd_send_cmd(ST7789_CASET);
//     uint8_t x_buf[4] = {x1>>8, x1&0xFF, x2>>8, x2&0xFF};
//     lcd_send_data(x_buf, 4);

//     // 设置行地址范围（Y轴）
//     lcd_send_cmd(ST7789_RASET);
//     uint8_t y_buf[4] = {y1>>8, y1&0xFF, y2>>8, y2&0xFF};
//     lcd_send_data(y_buf, 4);

//     // 写入显存命令
//     lcd_send_cmd(ST7789_RAMWR);
//     uint16_t *color = (uint16_t*)px_map;  // 转为16位RGB565颜色格式
//     int total = (x2-x1+1)*(y2-y1+1);     // 计算总像素数
//     // 逐点发送像素数据
//     for(int i=0; i<total; i++){
//         uint8_t p[2] = {color[i]>>8, color[i]&0xFF};
//         lcd_send_data(p, 2);
//     }
//     lv_display_flush_ready(disp);  // 告诉LVGL刷新完成
// }

// void lv_tick_cb(void *arg) {
//     lv_tick_inc(1);  // LVGL时钟+1ms
// }

// void lvgl_init(void) {
//     lv_init();  // 初始化LVGL内核
//     // 定义LVGL显示缓冲区（10行高度，节省内存）
//     static lv_color_t buf[LCD_WIDTH*10];
//     // 创建LVGL显示设备，指定分辨率
//     lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
//     // 设置显示缓冲区
//     lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
//     // 绑定刷新回调函数
//     lv_display_set_flush_cb(disp, lv_display_flush);

//     // 创建硬件定时器，每1ms触发一次LVGL时钟
//     esp_timer_create_args_t args = {.callback = lv_tick_cb};
//     esp_timer_handle_t timer;
//     esp_timer_create(&args, &timer);
//     esp_timer_start_periodic(timer, 1000);  // 1000us = 1ms
// }