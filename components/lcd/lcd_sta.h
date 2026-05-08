#ifndef LCD_STA_H
#define LCD_STA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> 

void lcd_init(void);
void lcd_send_cmd(uint8_t cmd);
void lcd_send_data(const uint8_t *data, size_t len);
void lcd_clear(uint16_t color);
void lcd_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_fill(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color);
void lcd_show_str(uint16_t x, uint16_t y, char *str, uint16_t color, uint8_t size);
void lcd_show_wifi_status(const char *status);

#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define WHITE   0xFFFF
#define BLACK   0x0000
#define YELLOW  0xFFE0

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

#endif