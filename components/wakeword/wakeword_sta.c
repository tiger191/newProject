#include "wakeword_sta.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mic_sta.h"

//#include "esp_afe.h"

#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_afe_sr_models.h"
#include "esp_process_sdkconfig.h" 

#include "model_path.h"

#define TAG "wakeword"

static const esp_afe_sr_iface_t *s_afe = NULL;
static esp_afe_sr_data_t *s_afe_data = NULL;

// 32-bit to 16-bit 转换（INMP441 输出 32-bit，有效数据在高16位）
static void convert_32bit_to_16bit(int32_t *src, int16_t *dst, int samples)
{
    for (int i = 0; i < samples; i++) {
        dst[i] = (int16_t)(src[i] >> 16);
    }
}


static void wakeword_task(void *arg)
{
    int chunk = s_afe->get_feed_chunksize(s_afe_data);

    // 分配 32-bit 缓冲区读取 I2S 数据
    int32_t *buf_32bit = malloc(chunk * sizeof(int32_t));
    // 分配 16-bit 缓冲区给 AFE
    int16_t *buf_16bit = malloc(chunk * sizeof(int16_t));

        // ========== 这里会打印！==========
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "✅ 已启动：等待唤醒词 → 你好小智");
    ESP_LOGI(TAG, "================================");

     if (!buf_32bit || !buf_16bit){
        ESP_LOGE(TAG, "malloc fail");
        free(buf_32bit);
        free(buf_16bit);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "等待唤醒：你好小智");

    while (1) {
        size_t bytes_read;
        // 读取 32-bit 数据
        if (i2s_channel_read(get_mic_handle(), buf_32bit, chunk * sizeof(int32_t), 
                             &bytes_read, 20) == ESP_OK) {
            int samples = bytes_read / sizeof(int32_t);
            // 转换为 16-bit
            convert_32bit_to_16bit(buf_32bit, buf_16bit, samples);
            // 喂给 AFE
            s_afe->feed(s_afe_data, buf_16bit);

            // 取结果
            afe_fetch_result_t *res = s_afe->fetch(s_afe_data);
            if (res && res->wake_word_index > 0) {
                ESP_LOGI(TAG, "✅ 唤醒成功（你好小智）");
            }
            //s_afe->free_fetch_result(s_afe_data, res);
        }
        vTaskDelay(1);
    }
    free(buf_32bit);
    free(buf_16bit);
}

void wakeword_init(void)
{
    ESP_LOGI(TAG, "初始化唤醒词模块...");

    afe_config_t afe_config = AFE_CONFIG_DEFAULT();    
    
    // 单麦克风配置，禁用 AEC
    afe_config.aec_init = false;
    afe_config.pcm_config.total_ch_num = 1;
    afe_config.pcm_config.mic_num = 1;
    afe_config.pcm_config.ref_num = 0;
    afe_config.pcm_config.sample_rate = 16000;

    srmodel_list_t *models = esp_srmodel_init("model");

    if (!models || models->num == 0) {
        ESP_LOGE(TAG, "模型加载失败！");
        return;
    }

    ESP_LOGI(TAG, "成功加载 %d 个模型：", models->num);
    for (int i = 0; i < models->num; i++) {
        ESP_LOGI(TAG, "Model %d: %s", i, models->model_name[i]);
    }

        // 方案1：使用 ESP-SR 提供的过滤函数（推荐）
    char *wakenet_model = esp_srmodel_filter(models, "wn9", NULL);
    
    if (!wakenet_model) {
        ESP_LOGE(TAG, "未找到唤醒词模型！尝试使用第一个模型...");
        if (models->num > 0) {
            wakenet_model = models->model_name[0];
        } else {
            ESP_LOGE(TAG, "没有可用的模型");
            return;
        }
    }

    // 指定唤醒词模型（根据 menuconfig 中选择的模型）
    // afe_config.wakenet_model_name = "wn9_nihaoxiaozhi_tts";

    afe_config.wakenet_model_name = wakenet_model;
    ESP_LOGI(TAG, "使用唤醒词模型: %s", afe_config.wakenet_model_name);

    // 打印完整配置
    ESP_LOGI(TAG, "AFE 配置：");
    ESP_LOGI(TAG, "  wakenet_model_name: %s", afe_config.wakenet_model_name);
    ESP_LOGI(TAG, "  aec_init: %d", afe_config.aec_init);
    ESP_LOGI(TAG, "  total_ch_num: %d", afe_config.pcm_config.total_ch_num);
    ESP_LOGI(TAG, "  mic_num: %d", afe_config.pcm_config.mic_num);
    ESP_LOGI(TAG, "  ref_num: %d", afe_config.pcm_config.ref_num);
    ESP_LOGI(TAG, "  sample_rate: %d", afe_config.pcm_config.sample_rate);

    ESP_LOGI(TAG, "正在创建 AFE 实例...");

    s_afe = &ESP_AFE_SR_HANDLE;
    s_afe_data = s_afe->create_from_config(&afe_config);
    if (!s_afe_data) {
        ESP_LOGE(TAG, "AFE 创建失败，检查 menuconfig 唤醒词配置");
        return;
    }

    ESP_LOGI(TAG, "AFE 创建成功 → 启动唤醒任务");

    xTaskCreate(wakeword_task, "wakeword", 16384, NULL, 5, NULL);
}