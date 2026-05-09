#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_sta.h"
#include "lcd_sta.h"
#include "ui.h"
#include "lvgl.h"

static const char *TAG = "main";

extern lv_obj_t *g_label_time;


void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "System start...");

    lcd_init();           // 初始化屏幕 + LVGL

    lvgl_init(); 

    create_ui(); 

    wifi_sta_init();      // 启动WiFi

    while (1) {
        // 每秒更新一次时间显示
        static uint32_t last_tick = 0;
        if (lv_tick_get() - last_tick >= 1000) {
            last_tick = lv_tick_get();
            //update_time_str();                  // 更新时间字符串
        }

        lv_timer_handler();  // LVGL内核处理（必须调用）
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}