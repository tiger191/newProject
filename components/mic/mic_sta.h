#ifndef MIC_STA_H
#define MIC_STA_H

#include "esp_err.h"
#include "driver/i2s_std.h"

//麦克风引脚配置 (MIC)
#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_4   // 字选择
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_5   // 时钟
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_6   // 数据输入

esp_err_t init_mic(uint32_t sample_rate, int channel_format, int bits_per_chan);
i2s_chan_handle_t get_mic_handle(void);
void test_mic_read(void *arg);
esp_err_t bsp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len);

#endif