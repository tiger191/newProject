#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_sta.h"
#include "lcd_sta.h"
#include "ui.h"
#include "lvgl.h"
#include "time_sta.h"
#include "weather.h"

static const char *TAG = "main";

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

    // 等待WiFi连接成功（最多等待10秒：100*100ms）
    int wait = 0;
    while (wait < 100 && !wifi_sta_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(100));
        wait++;
    }

    // WiFi连接成功后，执行网络功能
    if (wifi_sta_is_connected()) {
        time_sync_init();  // 1. 启动网络时间同步

        // 等待时间同步（最多10秒）
        wait = 0;
        while (wait < 100 && !is_time_synced()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            wait++;
        }

        // 2. 创建独立任务获取天气（优先级2，堆栈10KB）
        xTaskCreate(weather_task, "weather", 10240, NULL, 2, NULL);
    }

    while (1) {
        // 每秒更新一次时间显示
        static uint32_t last_tick = 0;
        if (lv_tick_get() - last_tick >= 1000) {
            last_tick = lv_tick_get();
            update_time_str();                  // 更新时间字符串
            lv_label_set_text(get_label_time(), get_time_str());  // 更新屏幕文字
        }

                // 天气数据就绪 → 更新天气/温度显示
        if (get_weather_ok()) {
            lv_label_set_text(get_label_temp(), get_temp_str());
            lv_label_set_text(get_label_weather(), get_weather_str());
            set_weather_state(false);  // 清除标志，等待下次更新
        }

        lv_timer_handler();  // LVGL内核处理（必须调用）
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}