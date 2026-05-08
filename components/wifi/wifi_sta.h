#ifndef WIFI_STA_H
#define WIFI_STA_H

#include "esp_err.h"
#include <stdbool.h>   //


// 初始化WiFi STA模式（连接到路由器）
void wifi_sta_init(void);

// 获取当前WiFi连接状态
bool wifi_sta_is_connected(void);

#endif