/*
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 *	project:	smart-n6k5
 *  name:		main.c
 *	introduce:	智慧K5主程序
 *  author:		Kavinkun on 2018/08/04
 *  company:	www.hengdawb.com
 */

#include <string.h>
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "common.h"
#include "esp_gap_ble_api.h"
#include "wifi_sta.h"
#include "ota_manage.h"
#include "serve_response.h"
#include "uart_manage.h"
#include "app_ble.h"
#include "sntp_get_time.h"
#include "file_upload.h"
#include "record_sdcard.h"

#define TAG "app_main"



/*****************************************************************
  * 外设初始化，如果需要初始化其他外设，请在该函数中添加功能
 ****************************************************************/
static void init_hardware()
{
	FF_DIR DIRinfo;
//	esp_log_level_set("*", ESP_LOG_WARN);
//	esp_log_level_set(TAG, ESP_LOG_INFO);		
    nvs_flash_init();
    ESP_LOGI(TAG, "hardware initialized");
	esp_periph_config_t periph_cfg = { 0 };
    esp_periph_init(&periph_cfg);

    // Initialize SD Card peripheral
    periph_sdcard_cfg_t sdcard_cfg = {
        .root = "/sdcard",
        .card_detect_pin = -1,
    };
    esp_periph_handle_t sdcard_handle = periph_sdcard_init(&sdcard_cfg);
    // Start sdcard & button peripheral
    esp_periph_start(sdcard_handle);

    // Wait until sdcard was mounted
    while (!periph_sdcard_is_mounted(sdcard_handle)) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    audio_hal_codec_config_t audio_hal_codec_cfg=AUDIO_HAL_ES8388_DEFAULT();
	audio_hal_handle_t hal = audio_hal_init(&audio_hal_codec_cfg, 0);
	audio_hal_ctrl_codec(hal, AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_START);
	if(f_opendir(&DIRinfo,"record")==FR_OK)
	{
	    ESP_LOGI(TAG, "/sdcard/record already exisit");
		f_closedir(&DIRinfo);
	}
	else
	{
		f_mkdir("record");
		ESP_LOGI(TAG, "create dir /sdcard/record");
	}
	if(f_opendir(&DIRinfo,"player")==FR_OK)
	{
	    ESP_LOGI(TAG, "/sdcard/player already exisit");		
		f_closedir(&DIRinfo);
	}
	else
	{
	    f_mkdir("player");
	    ESP_LOGI(TAG, "create dir /sdcard/player");
	}	


}


/*****************************************************************
  * 函数入口
 ****************************************************************/
void app_main()
{
	char fntmbuf[20];


    ESP_LOGI(TAG, "---------- Intialization started ----------");
    ESP_LOGI(TAG, "---------- Software version: %s -----------", SOFTWARE_VERSION);
	ESP_LOGI(TAG, "RAM left: %u", esp_get_free_heap_size());
	
	init_hardware();
	/*************************************************************
	 * 根据UART串口接受到的GD32的指令进行处理，将逐步进行完善，下只罗列已实现的功能逻辑
	 * 1，从GD32获取开启WIFI命令，进行WIFI STA 的开启(或者更改为开机自动开启WIFI，BLE)
	 * 2，从GD32获取OTA自检升级进行，进行处理完成之后，将重启
	 * 3，从GD32获取命令推送，推送与HTTP服务器进行数据交互
	 **************************************************************/
	wifi_process();
	sntp_get_time();
	esp_wifi_stop();
	uart_process();
	app_ble_init();
	xTaskCreate((void*)rec_event_task, "rec_event_task", 4096, NULL, 10, NULL);

    ESP_LOGI(TAG, "app_main task will be deleted!");
	vTaskDelete(NULL);

}
void restart_rec(void)
{
	char fntmbuf[20];

    ESP_LOGE(TAG, "RAM left: %u", esp_get_free_heap_size());
	ESP_LOGE(TAG, "restart to record!");
	app_ble_open();
    esp_ble_gap_start_scanning(0);    
    xTaskCreate((void*)rec_event_task, "rec_event_task", 4096, NULL, 10, NULL);

	ESP_LOGE(TAG, "restart_rec task will be deleted!");
	vTaskDelete(NULL);
}


