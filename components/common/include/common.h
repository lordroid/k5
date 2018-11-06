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
 *  name:		common.h
 *	introduce:	公共声明和公共声明
 *  author:		Kavinkun on 2018/07/18
 *  company:	www.hengdawb.com
 */
 
#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#define	SOFTWARE_VERSION		"V1.0"
#define EXAMPLE_WIFI_SSID		"lidg"
#define EXAMPLE_WIFI_PASS		"123456789"
#define FTP_SERVER_PORT			21
#define FTP_REMOTE_FILE_PATH	"D:/ftpServer/upload"
#define FTP_LOGIN_USER			"root"
#define FTP_LOGIN_PASSWD		"root"

esp_err_t new_firmware_set(int firmware_flag);
esp_err_t new_firmware_installed();
esp_err_t set_device_id(int deviceid);
esp_err_t get_device_id();
esp_err_t set_task_id(int taskid);
esp_err_t get_task_id();
esp_err_t set_rssinum(int rssinum);
esp_err_t get_rssinum();

#ifdef __cplusplus
}
#endif

#endif
