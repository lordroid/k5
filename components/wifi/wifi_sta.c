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
 *  name:		wifi_sta.c
 *	introduce:	��ģ�鸺������ά��WiFi���ӵ�������Ľ���㡣
 *  author:		Kavinkun on 2018/08/04
 *  company:	www.hengdawb.com
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h" 

#include "wifi_sta.h"
#include "common.h"

#define TAG "wifi_sta"

/*�¼��飬��������״̬*/
static EventGroupHandle_t wifi_sta_event_group;


/*****************************************************************
  * witi_sta���ӡ�
  * @param to cmd
 ****************************************************************/
static void wifi_sta_set_connected(bool c)
{
    if (wifi_sta_is_connected() == c) {
        return;
    }
    
    if (c) {
        xEventGroupSetBits(wifi_sta_event_group, WIFI_STA_EVENT_GROUP_CONNECTED_FLAG);
		xEventGroupClearBits(wifi_sta_event_group, WIFI_STA_EVENT_GROUP_DISCONNECTED_FLAG);
    } else {
        xEventGroupClearBits(wifi_sta_event_group, WIFI_STA_EVENT_GROUP_CONNECTED_FLAG);
		 xEventGroupSetBits(wifi_sta_event_group, WIFI_STA_EVENT_GROUP_DISCONNECTED_FLAG);
    }
    
    ESP_LOGI(TAG, "Device is now %s WIFI network", c ? "connected to" : "disconnected from");
}


/*****************************************************************
  * ��WiFi_Staģ�鴦������WiFi_STA�¼���
  * @param pointer to ctx
  * @param pointer to event
  * @param ����¼��������򽫡�handled������Ϊ1����������0��
 ****************************************************************/
static esp_err_t wifi_sta_handle_event(void *ctx, system_event_t *event)
{
    esp_err_t result = ESP_OK;
    
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGD(TAG, "wifi_sta_handle_event: SYSTEM_EVENT_STA_START");
            result = esp_wifi_connect();
            break;
            
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGD(TAG, "wifi_sta_handle_event: SYSTEM_EVENT_STA_GOT_IP");
            wifi_sta_set_connected(true);
            break;
            
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGD(TAG, "wifi_sta_handle_event: SYSTEM_EVENT_STA_CONNECTED");
            break;
            
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGE(TAG, "wifi_sta_handle_event: SYSTEM_EVENT_STA_DISCONNECTED");
            wifi_sta_set_connected(false);
            // try to re-connect
//            result = esp_wifi_connect();
            break;
            
        default:
            ESP_LOGD(TAG, "wifi_sta_handle_event: event is not for us: %d", event->event_id);
            break;
    }
	
    return result;
}


/*****************************************************************
  * �ڡ�sta��ģʽ�����ø��豸�����ӵ�ָ�������硣
  * @param pointer to config
 ****************************************************************/
static esp_err_t wifi_sta_init(wifi_params_struct_t *param)
{
	//wifi_config�Ķ��壬����sta��AP������
    static wifi_config_t wifi_config_struct;

	//SSID����
    if (strlen(param->wifi_ssid) >= sizeof(wifi_config_struct.sta.ssid) / sizeof(char)) {
        ESP_LOGE(TAG, "wifi_sta_init: invalid parameter: wifi_ssid too long");
        return ESP_ERR_INVALID_ARG;
    }

	//���볬��
    if (strlen(param->wifi_password) >= sizeof(wifi_config_struct.sta.password) / sizeof(char)) {
        ESP_LOGE(TAG, "wifi_sta_init: invalid parameter: wifi_password too long");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "wifi_sta_init: network = '%s'", param->wifi_ssid);

    //��ʼ��TCP/IP
    tcpip_adapter_init();

	//������ϵͳ�¼����񲢳�ʼ��Ӧ�ó����¼��Ļص�����
	esp_err_t set_wifi_sta_result = esp_event_loop_init(wifi_sta_handle_event,NULL);
    if (ESP_OK != set_wifi_sta_result) {
        ESP_LOGE(TAG, "wifi_sta_init: set_wifi_sta_result failed: %d", set_wifi_sta_result);
        return set_wifi_sta_result;
    }
    
    //��ʼ��WIFI(driver memory, buffers and so on).
    ESP_LOGD(TAG, "wifi_sta_init: esp_wifi_init");
    wifi_init_config_t init_config_struct = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t init_result = esp_wifi_init(&init_config_struct);
    if (ESP_OK != init_result) {
        ESP_LOGE(TAG, "wifi_sta_init: esp_wifi_init failed: %d", init_result);
        return init_result;
    }
    
    //�������ô洢
    ESP_LOGD(TAG, "wifi_sta_init: esp_wifi_set_storage");
    esp_err_t storage_result = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (ESP_OK != storage_result) {
        ESP_LOGE(TAG, "wifi_sta_init: esp_wifi_set_storage failed: %d", storage_result);
        return storage_result;
    }

    //��ESP32 WiFi����STAģʽ
    ESP_LOGD(TAG, "wifi_sta_init: esp_wifi_set_mode");
    esp_err_t set_mode_result = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ESP_OK != set_mode_result) {
        ESP_LOGE(TAG, "wifi_sta_init: esp_wifi_set_mode failed: %d", set_mode_result);
        return set_mode_result;
    }

    //����ESP32 STAģʽ������
    ESP_LOGD(TAG, "wifi_sta_init: esp_wifi_set_config");
    strcpy((char*)wifi_config_struct.sta.ssid, param->wifi_ssid); // max 32 bytes
    strcpy((char*)wifi_config_struct.sta.password, param->wifi_password); // max 32 bytes
    wifi_config_struct.sta.bssid_set = false;
    esp_err_t set_config_result = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_struct);
    if (ESP_OK != set_config_result) {
        ESP_LOGE(TAG, "wifi_sta_init: esp_wifi_set_config failed: %d", set_config_result);
        return set_config_result;
    }
    
    vTaskDelay(200 / portTICK_PERIOD_MS);
	//����workGroup
    wifi_sta_event_group = xEventGroupCreate();
 
    //���ݵ�ǰ��������WiFi
    ESP_LOGD(TAG, "wifi_sta_init: esp_wifi_start");
    esp_err_t start_result = esp_wifi_start();
    if (ESP_OK != start_result) {
        ESP_LOGE(TAG, "wifi_sta_init: esp_wifi_set_config failed: %d", start_result);
        return start_result;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);

    return ESP_OK;
}


/*****************************************************************
  * ���wifi_sta�Ƿ�����
  * ����豸��ǰ���ӵ�ָ�������磬�򷵻�1������Ϊ0
 ****************************************************************/
bool wifi_sta_is_connected()
{
    return (xEventGroupGetBits(wifi_sta_event_group) & WIFI_STA_EVENT_GROUP_CONNECTED_FLAG) ? 1 : 0;
}


/*****************************************************************
  * ������ģ���ȡWIFI_STA��״̬
 ****************************************************************/
EventGroupHandle_t wifi_sta_get_event_group()
{
    return wifi_sta_event_group;
}


/*****************************************************************
  * WIFI���̵Ĵ���
 ****************************************************************/
void wifi_process()
{
	wifi_params_struct_t wifi_params;		//WIFI STA ״̬�������������ڴ��wifi���ӵ����ò�������wifi_staģ���ж���
	wifi_params.wifi_ssid = EXAMPLE_WIFI_SSID;
	wifi_params.wifi_password = EXAMPLE_WIFI_PASS;
	ESP_LOGI(TAG, "Set up WIFI network connection.");
	wifi_sta_init(&wifi_params);
}

