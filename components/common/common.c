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
 *  name:		common.c
 *	introduce:	���������͹�������
 *  author:		Kavinkun on 2018/08/04
 *  company:	www.hengdawb.com
 */

#include "esp_err.h"

/*�̼�����״̬*/
static esp_err_t firmware_installed_flag = 0;
static esp_err_t device_id = 0;
static esp_err_t task_id = 0;
static esp_err_t rssi = 0;


/*****************************************************************
  * ���ù̼�����״̬
 ****************************************************************/
esp_err_t new_firmware_set(int firmware_flag)
{
	firmware_installed_flag = firmware_flag;
	return firmware_installed_flag;
}


/*****************************************************************
  * ��ȡ�̼�����״̬
 ****************************************************************/
esp_err_t new_firmware_installed()
{
	return firmware_installed_flag;
}


/*****************************************************************
  * ���÷����device id
 ****************************************************************/
esp_err_t set_device_id(int deviceid)
{
	device_id = deviceid;
	return device_id;
}


/*****************************************************************
  * ��ȡ�����device id
 ****************************************************************/
esp_err_t get_device_id()
{
	return device_id;
}

/*****************************************************************
  * ����task id
 ****************************************************************/
esp_err_t set_task_id(int taskid)
{
	task_id = taskid;
	return task_id;
}


/*****************************************************************
  * ��ȡtask id
 ****************************************************************/
esp_err_t get_task_id()
{
	return task_id;
}


/*****************************************************************
  * ����RSSI
 ****************************************************************/
esp_err_t set_rssinum(int rssinum)
{
	rssi = rssinum;
	return rssi;
}


/*****************************************************************
  * ��ȡRSSI
 ****************************************************************/
esp_err_t get_rssinum()
{
	return rssi;
}
