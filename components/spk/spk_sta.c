#include "spk_sta.h"
#include "mic_sta.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "spk";
static i2s_chan_handle_t spk_tx_handle = NULL;
static bool spk_running = false;

#define SPK_PA_GPIO GPIO_NUM_3   // 功放使能脚

// ====================== 防啸叫核心参数 ======================
#define NOISE_FLOOR    350    // 1. 底噪过滤
#define VOLUME_ATTEN   0.4f   // 2. 音量衰减
#define VOLUME_LIMIT   4000   // 3. 峰值限制
#define MUTE_ON_NOISE  true   // 4. 纯静音区归零
// ===========================================================

esp_err_t init_spk(void)
{
    // 先打开功放
    gpio_config_t pa_conf = {
        .pin_bit_mask = 1ULL << SPK_PA_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&pa_conf);
    gpio_set_level(SPK_PA_GPIO, 1); // 打开喇叭功放

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 6;
    chan_cfg.dma_frame_num = 240;
    esp_err_t ret = i2s_new_channel(&chan_cfg, &spk_tx_handle, NULL);
    if (ret != ESP_OK) return ret;

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = 16000,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
            .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = AUDIO_I2S_SPK_GPIO_BCLK,
            .ws   = AUDIO_I2S_SPK_GPIO_LRCK,
            .dout = AUDIO_I2S_SPK_GPIO_DOUT,
            .din  = I2S_GPIO_UNUSED,
        },
    };

    ret = i2s_channel_init_std_mode(spk_tx_handle, &std_cfg);
    if (ret != ESP_OK) return ret;

    i2s_channel_enable(spk_tx_handle);
    ESP_LOGI(TAG, "✅ 扬声器初始化完成");
    return ESP_OK;
}

void mic_to_spk_task(void *arg)
{
    int32_t buf[128];
    size_t bytes_read;

    while (1)
    {
        if (i2s_channel_read(get_mic_handle(), buf, sizeof(buf), &bytes_read, 50) == ESP_OK)
        {
            for (int i = 0; i < bytes_read / 4; i++)
            {
                int16_t val = (int16_t)(buf[i] >> 16);

                // 1. 底噪过滤
                if (abs(val) < NOISE_FLOOR)
                {
                    buf[i] = 0;
                    continue;
                }

                // 2. 音量衰减
                val = (int16_t)(val * VOLUME_ATTEN);

                // 3. 峰值限制
                if (val > VOLUME_LIMIT) val = VOLUME_LIMIT;
                if (val < -VOLUME_LIMIT) val = -VOLUME_LIMIT;

                // 4. 弱音二次归零（纯静音区）
                if (MUTE_ON_NOISE && abs(val) < NOISE_FLOOR * 1.2f)
                {
                    val = 0;
                }

                buf[i] = (int32_t)val << 16;
            }

            i2s_channel_write(spk_tx_handle, buf, bytes_read, NULL, 50);
        }
        vTaskDelay(1);
    }
}

void start_mic_to_spk(void)
{
    if (spk_running) return;
    spk_running = true;
    xTaskCreate(mic_to_spk_task, "mic_spk", 8192, NULL, 5, NULL);
}

void stop_mic_to_spk(void)
{
    spk_running = false;
}