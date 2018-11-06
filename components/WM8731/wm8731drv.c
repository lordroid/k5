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

#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "wm8731drv.h"
#include "board.h"

static const char *ES_TAG = "WM8731_DRIVER";

static void delay(uint16_t delaytime)
{
   
    while(delaytime--);
}
static void wm_write_reg(uint8_t reg_add, uint16_t data)
{
    unsigned char i;
 
    CSB_8731_ON;
    delay(100);
    CSB_8731_OFF;
    for (i = 0; i < 7; i++) {
        delay(15);
        SCL_8731_OFF;
        if (reg_add & 0x40) {
            SDA_8731_ON;
        } else {
            SDA_8731_OFF;
        }
        reg_add <<= 1;
        delay(15);
        SCL_8731_ON;
    }
    for (i = 0; i < 9; i++) {
        delay(15);
        SCL_8731_OFF;
        if (data & 0x0100) {
            SDA_8731_ON;
        } else {
            SDA_8731_OFF;
        }
        data <<= 1;
        delay(15);
        SCL_8731_ON;
    }
    delay(100);
    CSB_8731_ON;
}

static int wm_read_reg(uint8_t reg_add, uint8_t *pData)
{

    return ESP_FAIL;
}

static void wm_io_init()
{    
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO18_U, FUNC_GPIO18_GPIO18);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO23_U, FUNC_GPIO23_GPIO23);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_MTCK_GPIO13);

        gpio_set_direction(WM8731_CSB , GPIO_MODE_OUTPUT);
	gpio_set_pull_mode(WM8731_CSB,GPIO_PULLUP_ONLY);
	gpio_set_direction(WM8731_CLK , GPIO_MODE_OUTPUT);
	gpio_set_pull_mode(WM8731_CLK,GPIO_PULLUP_ONLY);
	gpio_set_direction(WM8731_SDIN , GPIO_MODE_OUTPUT);
	gpio_set_pull_mode(WM8731_SDIN,GPIO_PULLUP_ONLY);      
}

void wm8731_read_all()
{
    for (int i = 0; i < 50; i++) {
        uint8_t reg = 0;
        wm_read_reg(i, &reg);        
    }
}

int wm8731_write_reg(uint8_t reg_add, uint16_t data)
{
    wm_write_reg(reg_add, data);
    return ESP_OK;
}

/**
 * @brief Configure ES8388 ADC and DAC volume. Basicly you can consider this as ADC and DAC gain
 *
 * @param mode:             set ADC or DAC or all
 * @param volume:           -96 ~ 0              for example Es8388SetAdcDacVolume(ES_MODULE_ADC, 30, 6); means set ADC volume -30.5db
 * @param dot:              whether include 0.5. for example Es8388SetAdcDacVolume(ES_MODULE_ADC, 30, 4); means set ADC volume -30db
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
static int wm8731_set_adc_dac_volume(int mode, int volume, int dot)
{
    
	int volumetmp,dottmp;
	volumetmp=volume;
	dottmp=dot;
	if(volumetmp<-34)
		volumetmp=-34;
	if(volumetmp>12)
		volumetmp =12;
    dottmp = (dottmp >= 5 ? 1 : 0);
    volumetmp =23+(volumetmp+dottmp)*2/3;
	if(volumetmp<0)
		volumetmp=0;
	if(volumetmp>31)
		volumetmp=31;
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {
        wm_write_reg(WM8731_LINVOL, volumetmp|1<<7);
        wm_write_reg(WM8731_RINVOL, volumetmp|1<<7);  //ADC Right Volume=0db
    }
	volumetmp=volume;
	dottmp=dot;
	if(volumetmp<-73)
		volumetmp=-73;
	if(volumetmp>6)
		volumetmp =6;
    dottmp = (dottmp >= 5 ? 1 : 0);
    volumetmp =121+(volume+dot);
	if(volumetmp<48)
		volume=48;
	if(volume>127)
		volume=127;	
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
        wm_write_reg(WM8731_LOUT1V, volume);
        wm_write_reg(WM8731_ROUT1V, volume);
    }
    return ESP_OK;
}


/**
 * @brief Power Management
 *
 * @param mod:      if ES_POWER_CHIP, the whole chip including ADC and DAC is enabled
 * @param enable:   false to disable true to enable
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int wm8731_start(es_module_t mode)
{
    
    uint8_t prev_data = 0, data = 0;   
    if (mode == ES_MODULE_LINE) {
       wm_write_reg(WM8731_PWR, 0x0000); 
    } else {
        wm_write_reg(WM8731_PWR, 0x0000);   
    }	
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC || mode == ES_MODULE_LINE) {
        wm_write_reg(WM8731_PWR, 0x00);   //power up adc and line in
    }
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC || mode == ES_MODULE_LINE) {
        wm_write_reg(WM8731_PWR, 0x00); 
        wm8731_set_voice_mute(false);
        
    }

    return ESP_OK;
}

/**
 * @brief Power Management
 *
 * @param mod:      if ES_POWER_CHIP, the whole chip including ADC and DAC is enabled
 * @param enable:   false to disable true to enable
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int wm8731_stop(es_module_t mode)
{
//    int res = 0;
//    if (mode == ES_MODULE_LINE) {
        wm_write_reg(WM8731_PWR, 0x00e0); 
	wm8731_set_voice_mute(true);
//        return res;
//    }
//    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
//        res |= es_write_reg(ES8388_ADDR, ES8388_DACPOWER, 0x00);
//        res |= es8388_set_voice_mute(true); //res |= Es8388SetAdcDacVolume(ES_MODULE_DAC, -96, 5);      // 0db
//        //res |= es_write_reg(ES8388_ADDR, ES8388_DACPOWER, 0xC0);  //power down dac and line out
//    }
//    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {
//        //res |= Es8388SetAdcDacVolume(ES_MODULE_ADC, -96, 5);      // 0db
//        res |= es_write_reg(ES8388_ADDR, ES8388_ADCPOWER, 0xFF);  //power down adc and line in
//    }
//    if (mode == ES_MODULE_ADC_DAC) {
//        res |= es_write_reg(ES8388_ADDR, ES8388_DACCONTROL21, 0x9C);  //disable mclk
//        res |= es_write_reg(ES8388_ADDR, ES8388_CONTROL1, 0x00);
//        res |= es_write_reg(ES8388_ADDR, ES8388_CONTROL2, 0x58);
//        res |= es_write_reg(ES8388_ADDR, ES8388_CHIPPOWER, 0xF3);  //stop state machine
//    }

    return ESP_OK;
}


/**
 * @brief Config I2s clock in MSATER mode
 *
 * @param cfg.sclkDiv:      generate SCLK by dividing MCLK in MSATER mode
 * @param cfg.lclkDiv:      generate LCLK by dividing MCLK in MSATER mode
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int wm8731_i2s_config_clock(es_i2s_clock_t cfg)
{

    wm_write_reg(WM8731_IFACE, 0x0012); 
    return ESP_OK;
}

esp_err_t wm8731_deinit(void)
{
    
    wm_write_reg(WM8731_RESET, 0x0000);  //reset and stop es8388
    wm_write_reg(WM8731_ACTIVE, 0x0000); 
    return ESP_OK;
}

/**
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
esp_err_t wm8731_init(audio_hal_codec_config_t *cfg)
{
 
    wm_io_init();
    wm_write_reg(WM8731_PWR, 0x0000); 
    wm_write_reg(WM8731_RESET, 0x0000); 
    wm_write_reg(WM8731_PWR, 0x0000); 
    wm_write_reg(WM8731_ACTIVE, 0x0000); 
    wm_write_reg(WM8731_APANA, 0x0035);
    wm_write_reg(WM8731_IFACE, 0x0012); //slave mode,i2s format
    wm_write_reg(WM8731_SRATE , 0x0000);//2e:8K;22:44.1K
    wm_write_reg(WM8731_ACTIVE, 0x0001); 
    return ESP_OK;
}

/**
 * @brief Configure ES8388 I2S format
 *
 * @param mode:           set ADC or DAC or all
 * @param bitPerSample:   see Es8388I2sFmt
 *
 * @return
 *     - (-1) Error
 *     - (0)  Success
 */
int wm8731_config_fmt(es_module_t mode, es_i2s_fmt_t fmt)
{
    wm_write_reg(WM8731_IFACE, 0x0012); //slave mode,i2s format
    return ESP_OK;
}

/**
 * @param volume: 0 ~ 100
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int wm8731_set_voice_volume(int volume)
{
	int volumetmp=volume;
	if(volumetmp<-73)
		volumetmp=-73;
	if(volumetmp>6)
		volumetmp =6;
  
    volumetmp =121+volume;
	if(volumetmp<48)
		volume=48;
	if(volume>127)
		volume=127;	

    wm_write_reg(WM8731_LOUT1V, volume);
    wm_write_reg(WM8731_ROUT1V, volume);
    
    return ESP_OK;
}
/**
 *
 * @return
 *           volume
 */
int wm8731_get_voice_volume(int *volume)
{
    int res;
    uint8_t reg = 0;
    res = wm_read_reg(WM8731_LOUT1V, &reg);
    if (res == ESP_FAIL) {
        *volume = 0;
    } else {
        *volume = reg;
        *volume *= 3;
        if (*volume == 99)
            *volume = 100;
    }
    return res;
}

/**
 * @brief Configure ES8388 data sample bits
 *
 * @param mode:             set ADC or DAC or all
 * @param bitPerSample:   see BitsLength
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int wm8731_set_bits_per_sample(es_module_t mode, es_bits_length_t bits_length)
{
    wm_write_reg(WM8731_SRATE , 0x0000);
    return ESP_OK;
}

/**
 * @brief Configure ES8388 DAC mute or not. Basicly you can use this function to mute the output or don't
 *
 * @param enable: enable or disable
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int wm8731_set_voice_mute(int enable)
{
    if(enable)
    {    
      wm_write_reg(WM8731_LOUT1V, 0);
      wm_write_reg(WM8731_ROUT1V, 0);
    }
    return ESP_OK;
}

int wm8731_get_voice_mute(void)
{

    return ESP_FAIL;
}

/**
 * @param gain: Config DAC Output
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int wm8731_config_dac_output(int output)
{

    return ESP_OK;
}

/**
 * @param gain: Config ADC input
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int wm8731_config_adc_input(es_adc_input_t input)
{

    return ESP_OK;
}

/**
 * @param gain: see es_mic_gain_t
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int wm8731_set_mic_gain(es_mic_gain_t gain)
{
    return ESP_OK;
}

int wm8731_ctrl_state(audio_hal_codec_mode_t mode, audio_hal_ctrl_t ctrl_state)
{
    int res = 0;
    int es_mode_t = 0;
    switch (mode) {
        case AUDIO_HAL_CODEC_MODE_ENCODE:
            es_mode_t  = ES_MODULE_ADC;
            break;
        case AUDIO_HAL_CODEC_MODE_LINE_IN:
            es_mode_t  = ES_MODULE_LINE;
            break;
        case AUDIO_HAL_CODEC_MODE_DECODE:
            es_mode_t  = ES_MODULE_DAC;
            break;
        case AUDIO_HAL_CODEC_MODE_BOTH:
            es_mode_t  = ES_MODULE_ADC_DAC;
            break;
        default:
            es_mode_t = ES_MODULE_DAC;            
            break;
    }
    if (AUDIO_HAL_CTRL_STOP == ctrl_state) {
        wm8731_stop(es_mode_t);
    } else {
        wm8731_start(es_mode_t);        
    }
    return res;
}

int wm8731_config_i2s(audio_hal_codec_mode_t mode, audio_hal_codec_i2s_iface_t *iface)
{
    int tmp = 0;
    wm8731_config_fmt(ES_MODULE_ADC_DAC, iface->fmt);
    if (iface->bits == AUDIO_HAL_BIT_LENGTH_16BITS) {
        tmp = BIT_LENGTH_16BITS;
    } else if (iface->bits == AUDIO_HAL_BIT_LENGTH_24BITS) {
        tmp = BIT_LENGTH_24BITS;
    } else {
        tmp = BIT_LENGTH_32BITS;
    }
    wm8731_set_bits_per_sample(ES_MODULE_ADC_DAC, tmp);
    return ESP_OK;
}

void wm8731_pa_power(bool enable)
{


}

