#ifndef SPK_STA_H
#define SPK_STA_H
#include "esp_err.h"

// 扬声器引脚配置 (SPK)
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_7   // 数据输出
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15  // 位时钟
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_16 // 左右时钟

esp_err_t init_spk(uint32_t sample_rate, int channel_format, int bits_per_chan);
esp_err_t spk_play_audio(const uint8_t *audio_data, size_t data_len);
esp_err_t spk_audio_stop(void);

#endif