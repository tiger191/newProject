
#include "ui.h"
#include "lcd_sta.h"
#include "esp_timer.h"
#include "lv_font_simsun_han_ht_16.h"

static lv_obj_t *s_label_time;    // LVGL标签对象：显示时间（2025-xx-xx xx:xx:xx）
static lv_obj_t *s_label_temp;    // LVGL标签对象：显示温度（xx°C）
static lv_obj_t *s_label_weather; // LVGL标签对象：显示城市+天气

 void lv_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    // 取出需要刷新的区域坐标
    int16_t x1 = area->x1, y1 = area->y1, x2 = area->x2, y2 = area->y2;

    // 设置列地址范围（X轴）
    lcd_send_cmd(ST7789_CASET);
    uint8_t x_buf[4] = {x1>>8, x1&0xFF, x2>>8, x2&0xFF};
    lcd_send_data(x_buf, 4);

    // 设置行地址范围（Y轴）
    lcd_send_cmd(ST7789_RASET);
    uint8_t y_buf[4] = {y1>>8, y1&0xFF, y2>>8, y2&0xFF};
    lcd_send_data(y_buf, 4);

    // 写入显存命令
    lcd_send_cmd(ST7789_RAMWR);
    uint16_t *color = (uint16_t*)px_map;  // 转为16位RGB565颜色格式
    int total = (x2-x1+1)*(y2-y1+1);     // 计算总像素数
    // 逐点发送像素数据
    for(int i=0; i<total; i++){
        uint8_t p[2] = {color[i]>>8, color[i]&0xFF};
        lcd_send_data(p, 2);
    }
    lv_display_flush_ready(disp);  // 告诉LVGL刷新完成
}

void lv_tick_cb(void *arg) {
    lv_tick_inc(1);  // LVGL时钟+1ms
}

void lvgl_init(void) {
    lv_init();  // 初始化LVGL内核
    // 定义LVGL显示缓冲区（10行高度，节省内存）
    static lv_color_t buf[LCD_WIDTH*10];
    // 创建LVGL显示设备，指定分辨率
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    // 设置显示缓冲区
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    // 绑定刷新回调函数
    lv_display_set_flush_cb(disp, lv_display_flush);

    // 创建硬件定时器，每1ms触发一次LVGL时钟
    esp_timer_create_args_t args = {.callback = lv_tick_cb};
    esp_timer_handle_t timer;
    esp_timer_create(&args, &timer);
    esp_timer_start_periodic(timer, 1000);  // 1000us = 1ms
}

void create_ui(void) {
    // 清空当前屏幕
    lv_obj_clean(lv_screen_active());
    // 设置屏幕背景为黑色
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    // 创建一个容器，用于排版三个标签
    lv_obj_t *cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, 240, 240);  // 容器大小=屏幕大小
    lv_obj_center(cont);             // 居中显示
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);  // 垂直布局
    // 水平/垂直/间距都居中
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // 容器透明无边框
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, 0);

    // ========== 创建时间标签 ==========
    s_label_time = lv_label_create(cont);
    lv_label_set_text(s_label_time, "同步时间中...");
    lv_obj_set_style_text_font(s_label_time, &lv_font_simsun_han_ht_16, 0);  // 设置中文字体
    lv_obj_set_style_text_color(s_label_time, lv_color_white(), 0);  // 白色文字

    // ========== 创建温度标签 ==========
    s_label_temp = lv_label_create(cont);
    lv_label_set_text(s_label_temp, "--°C");
    lv_obj_set_style_text_font(s_label_temp, &lv_font_simsun_han_ht_16, 0);
    lv_obj_set_style_text_color(s_label_temp, lv_color_white(), 0);

    // ========== 创建天气标签 ==========
    s_label_weather = lv_label_create(cont);
    lv_label_set_text(s_label_weather, "获取天气中...");
    lv_obj_set_style_text_font(s_label_weather, &lv_font_simsun_han_ht_16, 0);
    lv_obj_set_style_text_color(s_label_weather, lv_color_white(), 0);
}

lv_obj_t *get_label_time(){
    return s_label_time;
}