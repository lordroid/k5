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
 *  name:		wifi_sta.h
 *	introduce:	该模块负责建立和维护WiFi连接到所定义的接入点。
 *  author:		Kavinkun on 2018/08/04
 *  company:	www.hengdawb.com
 */

#ifndef __WIFI_STA__
#define __WIFI_STA__

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "freertos/event_groups.h"

/*WIFI_STA的状态FLAG*/
#define WIFI_STA_EVENT_GROUP_CONNECTED_FLAG (1 << 0)
#define WIFI_STA_EVENT_GROUP_DISCONNECTED_FLAG (1 << 1)

typedef struct wifi_params_struct_ {
    const char *wifi_ssid;
    const char *wifi_password;
} wifi_params_struct_t;

bool wifi_sta_is_connected();
EventGroupHandle_t wifi_sta_get_event_group();
void wifi_process();


#ifdef __cplusplus
}
#endif

#endif

