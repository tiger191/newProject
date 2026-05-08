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

    wifi_sta_init();      // 启动WiFi

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}