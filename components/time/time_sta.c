#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "esp_http_client.h"

static bool s_time_synced = false;  // 网络时间同步状态：true=已同步
static char s_time_str[64]   = {0};    // 存储格式化后的时间字符串

static const char *TAG = "time_sta";

//=============================================================================
// SNTP 网络时间同步
//=============================================================================
/**
 * @brief  时间同步完成回调函数
 */
static void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "时间同步完成 ✅");
    s_time_synced = true;  // 标记时间同步成功
}

/**
 * @brief  SNTP时间同步初始化：配置时区、NTP服务器
 */
void time_sync_init(void) {
    ESP_LOGI(TAG, "开始SNTP时间同步...");

    // 设置时区为东八区（北京时间）
    setenv("TZ", "CST-8", 1);
    tzset();

    // 配置SNTP：轮询模式
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    // 设置NTP服务器（阿里云+腾讯云，国内速度快）
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_setservername(1, "ntp.tencent.com");
    // 注册同步完成回调
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    // 启动SNTP
    esp_sntp_init();
}

/**
 * @brief  更新时间字符串：每秒调用一次，格式化当前时间
 */
void update_time_str(void) {
    // 未同步时间 → 显示提示文字
    if (!s_time_synced) {
        strcpy(s_time_str, "等待时间同步...");
        return;
    }

    time_t now;
    struct tm timeinfo;
    time(&now);                  // 获取当前时间戳
    localtime_r(&now, &timeinfo); // 转为本地时间（东八区）

    // 格式化时间：年-月-日 时:分:秒
    strftime(s_time_str, sizeof(s_time_str),
             "%Y-%m-%d %H:%M:%S", &timeinfo);

}


bool is_time_synced(void) {
    return s_time_synced;
}

char* get_time_str(void){
    return s_time_str;
}