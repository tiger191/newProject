#include "mic_sta.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mic";
static i2s_chan_handle_t mic_rx_handle = NULL;

esp_err_t init_mic(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 6;
    chan_cfg.dma_frame_num = 240;
    esp_err_t ret = i2s_new_channel(&chan_cfg, NULL, &mic_rx_handle);
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
            .bclk = AUDIO_I2S_MIC_GPIO_SCK,   //5
            .ws   = AUDIO_I2S_MIC_GPIO_WS,    //4
            .din  = AUDIO_I2S_MIC_GPIO_DIN,   //6
            .dout = I2S_GPIO_UNUSED,
        },
    };

    ret = i2s_channel_init_std_mode(mic_rx_handle, &std_cfg);
    if (ret != ESP_OK) return ret;

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