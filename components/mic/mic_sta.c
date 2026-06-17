#include "mic_sta.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mic";
static i2s_chan_handle_t mic_rx_handle = NULL;

esp_err_t init_mic(uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret = ESP_OK;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ret = i2s_new_channel(&chan_cfg, NULL, &mic_rx_handle);
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "创建 I2S 通道失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 确定正确的数据位宽度枚举值
    i2s_data_bit_width_t bit_width = (bits_per_chan == 32) ? I2S_DATA_BIT_WIDTH_32BIT : I2S_DATA_BIT_WIDTH_16BIT;


    // 配置 I2S 标准模式，专门针对 INMP441 优化
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = sample_rate,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bit_width, I2S_SLOT_MODE_MONO),  // 插槽配置
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,    // INMP441 不需要主时钟
            .bclk = AUDIO_I2S_MIC_GPIO_SCK,        // 位时钟引脚
            .ws = AUDIO_I2S_MIC_GPIO_WS,           // 字选择引脚
            .dout = I2S_GPIO_UNUSED,    // 不需要数据输出（仅录音）
            .din = AUDIO_I2S_MIC_GPIO_DIN,          // 数据输入引脚
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // INMP441 特定配置调整
    // INMP441 输出左对齐数据，我们只使用左声道
    std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_MONO;
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    // 初始化 I2S 标准模式
    ret = i2s_channel_init_std_mode(mic_rx_handle, &std_cfg);
    if (ret != ESP_OK){
        ESP_LOGI(TAG, "初始化 I2S 标准模式失败: %s", esp_err_to_name(ret));
        return ret;
    }


    // 启用 I2S 通道开始接收数据
    i2s_channel_enable(mic_rx_handle);
    ESP_LOGI(TAG, "✅ INMP441 初始化完成");
    return ESP_OK;
}

void test_mic_read(void *arg)
{
    int32_t buf[128];
    size_t bytes_read;
    const int16_t NOISE_FLOOR = 300;

    while (1)
    {
        if (i2s_channel_read(mic_rx_handle, buf, sizeof(buf), &bytes_read, 100) == ESP_OK)
        {
            //int16_t raw = (int16_t)(buf[0] >> 16);
            //int16_t clean = (abs(raw) < NOISE_FLOOR) ? 0 : raw;
            //ESP_LOGI(TAG, "raw: %d | clean: %d", raw, clean);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

i2s_chan_handle_t get_mic_handle(void)
{
    return mic_rx_handle;
}

/**
 * @brief 从麦克风获取音频数据
 *
 * 这个函数从 INMP441 麦克风读取音频数据，并进行必要的信号处理：
 * 1. 从 I2S 接口读取原始数据
 * 2. 对 INMP441 的输出进行格式转换和增益调整
 * 3. 确保数据适合后续的语音识别处理
 *
 * @param is_get_raw_channel 是否获取原始通道数据（不进行处理）
 * @param buffer 存储音频数据的缓冲区
 * @param buffer_len 缓冲区长度（字节）
 * @return esp_err_t 读取结果
 */
esp_err_t bsp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read = 0;

    // 从 I2S 通道读取音频数据
    ret = i2s_channel_read(mic_rx_handle, buffer, buffer_len, &bytes_read, portMAX_DELAY);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取 I2S 数据失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 检查读取的数据长度是否符合预期
    if (bytes_read != buffer_len) {
        ESP_LOGW(TAG, "预期读取 %d 字节，实际读取 %d 字节", buffer_len, bytes_read);
    }

    // INMP441 特定的数据处理
    // INMP441 输出 24 位数据在 32 位帧中，左对齐
    // 我们需要提取最高有效的 16 位用于 16 位音频处理
    if (!is_get_raw_channel) {
        int samples = buffer_len / sizeof(int16_t);

        // 对 INMP441 的数据进行处理
        // 麦克风输出左对齐数据，进行信号电平调整
        for (int i = 0; i < samples; i++) {
            // 当前使用原始信号电平（无增益）
            // 测试表明原始电平已足够满足唤醒词检测需求
            int32_t sample = (int32_t)(buffer[i]);

            // 可选：应用 2 倍增益以提升信号强度（当前已禁用）
            // 如果发现信号电平不足，可以取消下面这行的注释
            // sample = sample * 2;

            // 限制在 16 位有符号整数范围内
            if (sample > 32767) {
                sample = 32767;
            }
            if (sample < -32768) {
                sample = -32768;
            }

            buffer[i] = (int16_t)(sample);
        }
    }

    return ESP_OK;
}