#include <stdio.h>
#include <string.h>

#include "weather.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_http_client.h"

static bool s_weather_ok    = false;   // 天气数据获取状态：true=获取成功
static char s_temp_str[32]   = {0};    // 存储格式化后的温度字符串
static char s_weather_str[128] = {0};  // 存储格式化后的天气字符串

// 天气JSON数据缓冲区：存储HTTP请求返回的原始JSON文本
static char s_json_buf[2048] = {0};
static int  s_json_len      = 0;              // JSON数据实际长度

static const char *TAG = "weather";

//=============================================================================
// 天气获取功能：HTTP请求+JSON解析
//=============================================================================
/**
 * @brief  HTTP事件回调：接收天气接口返回的数据
 */
static esp_err_t http_weather_event(esp_http_client_event_t *evt) {
    // 事件：收到数据
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        // 检查缓冲区是否足够，避免溢出
        int need = s_json_len + evt->data_len;
        if (need < sizeof(s_json_buf)-1) {
            // 将数据追加到缓冲区
            memcpy(s_json_buf + s_json_len, evt->data, evt->data_len);
            s_json_len += evt->data_len;
        }
    }
    return ESP_OK;
}

/**
 * @brief  天气获取任务：独立任务执行HTTP请求，不阻塞主循环
 */
void weather_task(void *p) {
    ESP_LOGI(TAG, "开始获取天气数据...");
    // 清空缓冲区
    memset(s_json_buf, 0, sizeof(s_json_buf));
    s_json_len = 0;

    // 配置HTTP客户端：天气接口地址
    esp_http_client_config_t cfg = {
        .url = "https://uapis.cn/api/v1/misc/weather",
        .skip_cert_common_name_check = true,  // 跳过HTTPS证书检查
        .event_handler = http_weather_event,          // 数据接收回调
        .timeout_ms = 10000,                  // 超时10秒
    };

    // 初始化并执行HTTP请求
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_perform(client);

    // 解析返回的JSON数据
    if (s_json_len > 0) {
        cJSON *root = cJSON_Parse(s_json_buf);  // 解析JSON
        if (root) {
            // 从JSON中提取字段：城市、天气、温度
            const char *city    = cJSON_GetStringValue(cJSON_GetObjectItem(root, "city"));
            const char *weather = cJSON_GetStringValue(cJSON_GetObjectItem(root, "weather"));
            int temp            = cJSON_GetObjectItem(root, "temperature")->valueint;

            // 提取成功 → 格式化字符串
            if (city && weather) {
                snprintf(s_temp_str, sizeof(s_temp_str), "%d°C", temp);
                snprintf(s_weather_str, sizeof(s_weather_str), "%s %s", city, weather);
                s_weather_ok = true;  // 标记天气数据就绪
                ESP_LOGI(TAG, "天气获取成功 ✅");
            }
            cJSON_Delete(root);  // 释放JSON内存
        }
    }
    esp_http_client_cleanup(client);  // 释放HTTP客户端
    vTaskDelete(NULL);                // 任务完成，自我删除
}

bool get_weather_ok(void) {
    return s_weather_ok; 
}

char* get_weather_str(void) {
    return s_weather_str; 
}

char* get_temp_str(void) {
    return s_temp_str; 
}

void set_weather_state(bool state){
    s_weather_ok = state;
}