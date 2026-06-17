#include "wakeword_sta.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mic_sta.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_process_sdkconfig.h"
#include "model_path.h"


#include <math.h>

#define TAG "wakeword"

static char *s_model_name = NULL;

static void wakeword_task(void *arg)
{
        // 获取唤醒词检测接口
    esp_wn_iface_t *wakenet = (esp_wn_iface_t *)esp_wn_handle_from_name(s_model_name);
    if (wakenet == NULL)
    {
        ESP_LOGE(TAG, "获取唤醒词接口失败，模型: %s", s_model_name);
        return;
    }

        // 创建唤醒词模型数据实例
    // DET_MODE_90: 检测模式，90%置信度阈值，平衡准确率和误触发率
    model_iface_data_t *model_data = wakenet->create(s_model_name, DET_MODE_90);
    if (model_data == NULL)
    {
        ESP_LOGE(TAG, "创建唤醒词模型数据失败");
        return;
    }

    // 获取模型要求的音频数据块大小（样本数 × 每样本字节数）
    int audio_chunksize = wakenet->get_samp_chunksize(model_data) * sizeof(int16_t);

    // 分配音频数据缓冲区内存
    int16_t *buffer = (int16_t *)malloc(audio_chunksize);
    if (buffer == NULL)
    {
        ESP_LOGE(TAG, "音频缓冲区内存分配失败，需要 %d 字节", audio_chunksize);
        ESP_LOGE(TAG, "请检查系统可用内存");
        return;
    }

    // 显示系统配置信息
    ESP_LOGI(TAG, "✓ 系统配置完成:");
    ESP_LOGI(TAG, "  - 唤醒词模型: %s", s_model_name);
    ESP_LOGI(TAG, "  - 音频块大小: %d 字节", audio_chunksize);
    ESP_LOGI(TAG, "  - 检测置信度: 90%%");
    ESP_LOGI(TAG, "正在启动麦克风唤醒词检测...");
    ESP_LOGI(TAG, "请对着麦克风说出配置的唤醒词");

    ESP_LOGI(TAG, "启动唤醒任务");

    // ==========主循环 - 实时音频采集与唤醒词检测 ==========
    while (1)
    {
        // 从INMP441麦克风获取一帧音频数据
        // false参数表示获取处理后的音频数据（非原始通道数据）
        esp_err_t ret = bsp_get_feed_data(false, buffer, audio_chunksize);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "麦克风音频数据获取失败: %s", esp_err_to_name(ret));
            ESP_LOGE(TAG, "请检查INMP441硬件连接");
            vTaskDelay(pdMS_TO_TICKS(10)); // 等待10ms后重试
            continue;
        }

        // 将音频数据送入唤醒词检测算法
        // 返回检测状态：WAKENET_NO_DETECT(未检测到) 或 WAKENET_DETECTED(检测到)
        wakenet_state_t state = wakenet->detect(model_data, buffer);

        // 检查是否检测到唤醒词
        if (state == WAKENET_DETECTED)
        {
            ESP_LOGI(TAG, "🎉 检测到唤醒词！");

            // 输出检测结果到串口
            printf("=== 唤醒词检测成功！模型: %s ===\n", s_model_name);
            printf("=== Wake word detected! Model: %s ===\n", s_model_name);


            // 这里可以添加唤醒后的处理逻辑
            // 例如：启动语音识别、播放提示音、发送网络请求等
        }

        // 短暂延时，避免CPU占用过高，同时保证实时性
        // 1ms延时确保检测的实时性
        vTaskDelay(pdMS_TO_TICKS(1));
    }

        // ========== 资源清理 ==========
    // 注意：由于主循环是无限循环，以下代码正常情况下不会执行
    // 仅在程序异常退出时进行资源清理
    ESP_LOGI(TAG, "正在清理系统资源...");

    // 销毁唤醒词模型数据
    if (model_data != NULL)
    {
        wakenet->destroy(model_data);
    }

    // 释放音频缓冲区内存
    if (buffer != NULL)
    {
        free(buffer);
    }

    // 删除当前任务
    vTaskDelete(NULL);

}


void wakeword_init(void)
{
    ESP_LOGI(TAG, "初始化唤醒词模块...");

    // 从模型目录加载所有可用的语音识别模型
    srmodel_list_t *models = esp_srmodel_init("model");
    if (models == NULL)
    {
        ESP_LOGE(TAG, "语音识别模型初始化失败");
        ESP_LOGE(TAG, "请检查模型文件是否正确烧录到Flash分区");
        return;
    }

    // 自动选择sdkconfig中配置的唤醒词模型（如果配置了多个模型则选择以"wn9"开头的模型）
    s_model_name = esp_srmodel_filter(models, "wn9", NULL);
    if (s_model_name == NULL) {
        ESP_LOGE(TAG, "未找到任何唤醒词模型！");
        ESP_LOGE(TAG, "请确保已正确配置并烧录唤醒词模型文件");
        ESP_LOGE(TAG, "可通过 'idf.py menuconfig' 配置唤醒词模型");
        return;
    }

    //启动语音识别任务
    xTaskCreate(wakeword_task, "wakeword", 16384, NULL, 7, NULL);    
}