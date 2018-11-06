/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __WM8731DRV_H__
#define __WM8731DRV__

#include "esp_types.h"
#include "audio_hal.h"
#include "es8388.h"
#include "driver/i2c.h"

#define WM8731_CSB     23
#define WM8731_CLK     13
#define WM8731_SDIN    18

#define CSB_8731_ON   gpio_set_level(WM8731_CSB,1)//0x0020
#define CSB_8731_OFF  gpio_set_level(WM8731_CSB,0)//0x0020
#define SCL_8731_ON   gpio_set_level(WM8731_CLK,1)//0x0200
#define SCL_8731_OFF  gpio_set_level(WM8731_CLK,0)//0x0200
#define SDA_8731_ON   gpio_set_level(WM8731_SDIN,1)//0x0200
#define SDA_8731_OFF  gpio_set_level(WM8731_SDIN,0)//0x0200

#define WM8731_LINVOL   0x00
#define WM8731_RINVOL   0x01
#define WM8731_LOUT1V   0x02
#define WM8731_ROUT1V   0x03
#define WM8731_APANA    0x04
#define WM8731_APDIGI   0x05
#define WM8731_PWR      0x06
#define WM8731_IFACE    0x07
#define WM8731_SRATE    0x08
#define WM8731_ACTIVE   0x09
#define WM8731_RESET    0x0F


/**
 * @brief Configure ES8388 clock
 */

/**
 * @brief Initialize ES8388 codec chip
 *
 * @param cfg configuration of ES8388
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t wm8731_init(audio_hal_codec_config_t *cfg);

/**
 * @brief Deinitialize ES8388 codec chip
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t wm8731_deinit(void);

/**
 * @brief Configure ES8388 I2S format
 *
 * @param mod:  set ADC or DAC or both
 * @param cfg:   ES8388 I2S format
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t wm8731_config_fmt(es_module_t mod, es_i2s_fmt_t cfg);

/**
 * @brief Configure I2s clock in MSATER mode
 *
 * @param cfg:  set bits clock and WS clock
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t wm8731_i2s_config_clock(es_i2s_clock_t cfg);

/**
 * @brief Configure ES8388 data sample bits
 *
 * @param mode:  set ADC or DAC or both
 * @param bit_per_sample:  bit number of per sample
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t wm8731_set_bits_per_sample(es_module_t mode, es_bits_length_t bit_per_sample);

/**
 * @brief  Start ES8388 codec chip
 *
 * @param mode:  set ADC or DAC or both
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t wm8731_start(es_module_t mode);

/**
 * @brief  Stop ES8388 codec chip
 *
 * @param mode:  set ADC or DAC or both
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t wm8731_stop(es_module_t mode);

/**
 * @brief  Set voice volume
 *
 * @param volume:  voice volume (0~100)
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t wm8731_set_voice_volume(int volume);

/**
 * @brief Get voice volume
 *
 * @param[out] *volume:  voice volume (0~100)
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t wm8731_get_voice_volume(int *volume);

/**
 * @brief Configure ES8388 DAC mute or not. Basically you can use this function to mute the output or not
 *
 * @param enable enable(1) or disable(0)
 *
 * @return
 *     - ESP_FAIL Parameter error
 *     - ESP_OK   Success
 */
esp_err_t wm8731_set_voice_mute(int enable);

/**
 * @brief Get ES8388 DAC mute status
 *
 * @return
 *     - -1 Parameter error
 *     - 0 voice mute disable
 *     - 1 voice mute enable
 */
esp_err_t wm8731_get_voice_mute(void);

/**
 * @brief Set ES8388 mic gain
 *
 * @param gain db of mic gain
 *
 * @return
 *     - ESP_FAIL Parameter error
 *     - ESP_OK   Success
 */
esp_err_t wm8731_set_mic_gain(es_mic_gain_t gain);

/**
 * @brief Set ES8388 adc input mode
 *
 * @param input adc input mode
 *
 * @return
 *     - ESP_FAIL Parameter error
 *     - ESP_OK   Success
 */
esp_err_t wm8731_config_adc_input(es_adc_input_t input);

/**
 * @brief Set ES8388 dac output mode
 *
 * @param output dac output mode
 *
 * @return
 *     - ESP_FAIL Parameter error
 *     - ESP_OK   Success
 */
esp_err_t wm8731_config_dac_output(es_dac_output_t output);

/**
 * @brief Write ES8388 register
 *
 * @param reg_add address of register
 * @param data data of register
 *
 * @return
 *     - ESP_FAIL Parameter error
 *     - ESP_OK   Success
 */
esp_err_t wm8731_write_reg(uint8_t reg_add, uint16_t data);

/**
 * @brief Print all ES8388 registers
 *
 * @return
 *     - void
 */
void wm8731_read_all();

/**
 * @brief Configure ES8388 codec mode and I2S interface
 *
 * @param mode codec mode
 * @param iface I2S config
 *
 * @return
 *     - ESP_FAIL Parameter error
 *     - ESP_OK   Success
 */
esp_err_t wm8731_config_i2s(audio_hal_codec_mode_t mode, audio_hal_codec_i2s_iface_t *iface);

/**
 * @brief Control ES8388 codec chip
 *
 * @param mode codec mode
 * @param ctrl_state start or stop decode or encode progress
 *
 * @return
 *     - ESP_FAIL Parameter error
 *     - ESP_OK   Success
 */
esp_err_t wm8731_ctrl_state(audio_hal_codec_mode_t mode, audio_hal_ctrl_t ctrl_state);
void wm8731_pa_power(bool enable);
#endif

