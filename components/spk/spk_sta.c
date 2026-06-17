#include "spk_sta.h"
#include "mic_sta.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "spk";
static i2s_chan_handle_t spk_tx_handle = NULL;
static bool tx_channel_enabled = false;



/**
 * @brief 初始化 I2S 输出接口用于 MAX98357A 功放
 *
 * MAX98357A 是一个数字音频功放，需要特定的 I2S 配置：
 * - 使用标准 I2S 协议 (Philips 格式)
 * - 单声道或立体声模式
 * - 16 位数据宽度
 *
 * @param sample_rate 采样率 (Hz)
 * @param channel_format 声道数 (1=单声道, 2=立体声)
 * @param bits_per_chan 每个采样点的位数 (16 或 32)
 * @return esp_err_t 初始化结果
 */
esp_err_t init_spk(uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret = ESP_OK;

    // 创建 I2S 发送通道配置
    // 设置为主模式，ESP32-S3 作为时钟源
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    ret = i2s_new_channel(&chan_cfg, &spk_tx_handle, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "创建 I2S 发送通道失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 确定正确的数据位宽度枚举值
    i2s_data_bit_width_t bit_width = (bits_per_chan == 32) ? I2S_DATA_BIT_WIDTH_32BIT : I2S_DATA_BIT_WIDTH_16BIT;

    // 配置 I2S 标准模式，专门针对 MAX98357A 优化
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = sample_rate,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bit_width, (channel_format == 1) ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk   = I2S_GPIO_UNUSED,  // MAX98357A 不需要主时钟
            .bclk   = AUDIO_I2S_SPK_GPIO_BCLK, // 位时钟引脚
            .ws     = AUDIO_I2S_SPK_GPIO_LRCK,    // 字选择引脚
            .dout   = AUDIO_I2S_SPK_GPIO_DOUT,  // 数据输出引脚
            .din    = I2S_GPIO_UNUSED,   // 不需要数据输入（仅播放）
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // 初始化 I2S 标准模式
    ret = i2s_channel_init_std_mode(spk_tx_handle, &std_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "初始化 I2S 发送标准模式失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 启用 I2S 发送通道开始播放数据
    ret = i2s_channel_enable(spk_tx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "启用 I2S 发送通道失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 设置通道状态标志
    tx_channel_enabled = true;

    ESP_LOGI(TAG, "I2S 音频播放初始化成功");
    return ESP_OK;
}

/**
 * @brief 通过 I2S 播放音频数据
 *
 * 这个函数将音频数据发送到 MAX98357A 功放进行播放：
 * 1. 将音频数据写入 I2S 发送通道
 * 2. 确保数据完全发送
 *
 * @param audio_data 指向音频数据的指针
 * @param data_len 音频数据长度（字节）
 * @return esp_err_t 播放结果
 */
esp_err_t spk_play_audio(const uint8_t *audio_data, size_t data_len)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_written = 0;

    if (spk_tx_handle == NULL)
    {
        ESP_LOGE(TAG, "I2S 发送通道未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (audio_data == NULL || data_len == 0)
    {
        ESP_LOGE(TAG, "无效的音频数据");
        return ESP_ERR_INVALID_ARG;
    }

    // 确保 I2S 发送通道已启用（如果之前被停止了）
    if (!tx_channel_enabled)
    {
        ret = i2s_channel_enable(spk_tx_handle);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "启用 I2S 发送通道失败: %s", esp_err_to_name(ret));
            return ret;
        }
        tx_channel_enabled = true;
        ESP_LOGD(TAG, "I2S 发送通道已重新启用");
    }

    // 将音频数据写入 I2S 发送通道
    ret = i2s_channel_write(spk_tx_handle, audio_data, data_len, &bytes_written, portMAX_DELAY);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "写入 I2S 音频数据失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 检查写入的数据长度是否符合预期
    if (bytes_written != data_len)
    {
        ESP_LOGW(TAG, "预期写入 %d 字节，实际写入 %d 字节", data_len, bytes_written);
    }

    // 播放完成后停止I2S输出以防止噪音
    esp_err_t stop_ret = spk_audio_stop();
    if (stop_ret != ESP_OK)
    {
        ESP_LOGW(TAG, "停止音频输出时出现警告: %s", esp_err_to_name(stop_ret));
    }

    ESP_LOGI(TAG, "音频播放完成，播放了 %d 字节", bytes_written);
    return ESP_OK;
}


/**
 * @brief 停止 I2S 音频输出以防止噪音
 *
 * 这个函数会暂时禁用 I2S 发送通道，停止向 MAX98357A 发送数据，
 * 从而消除播放完成后的噪音。当需要再次播放音频时，
 * 可以重新启用通道。
 *
 * @return esp_err_t 停止结果
 */
esp_err_t spk_audio_stop(void)
{
    esp_err_t ret = ESP_OK;

    if (spk_tx_handle == NULL)
    {
        ESP_LOGW(TAG, "I2S 发送通道未初始化，无需停止");
        return ESP_OK;
    }

    // 只有在通道启用时才禁用它
    if (tx_channel_enabled)
    {
        ret = i2s_channel_disable(spk_tx_handle);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "禁用 I2S 发送通道失败: %s", esp_err_to_name(ret));
            return ret;
        }
        tx_channel_enabled = false;
        ESP_LOGI(TAG, "I2S 音频输出已停止");
    }
    else
    {
        ESP_LOGD(TAG, "I2S 发送通道已经是禁用状态");
    }

    return ESP_OK;
}