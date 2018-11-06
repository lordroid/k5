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
 *  name:		uart_manage.h
 *	introduce:	该模块主要功能是负责GD和ESP芯片的数据信息沟通
 *  author:		Kavinkun on 2018/08/11
 *  company:	www.hengdawb.com
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/semphr.h"

#include "uart_manage.h"
#include "wifi_sta.h"
#include "common.h"
#include "serve_response.h"
#include "app_ble.h"

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)



static const char *TAG = "uart_events";
static QueueHandle_t uart0_queue;
static EventGroupHandle_t gd_event_group;

/*****************************************************************
  * 校验和
 ****************************************************************/
static void checksum(uint8_t *pcheck,uint8_t len)
{
	uint8_t i;
	*(pcheck+len) = 0;
	for(i=0;i<len;i++)
	{
		(*(pcheck+len))^=(*(pcheck+i));
	}
}


/*****************************************************************
  * UART接受数据处理函数，处理完成之后发送给GD
 ****************************************************************/
static void uart_data_process(uint8_t *pdata)
{
	static uint8_t senddata[20];
	switch(pdata[2])
	{
		//读取SD卡数据
		case 1:
			senddata[0] = 0xaa;
			senddata[1] = 0x06;
			senddata[2] = 0x01;
			senddata[3] = pdata[3];
			senddata[4] = 0x42+pdata[3];
			checksum(senddata,senddata[1]-1);
			
			uart_write_bytes(EX_UART_NUM, (const char*)senddata, senddata[1]);
			break;

		//播放，续播
		case 2:
			break;

		//暂停播放
		case 4:
			break;

		//接收机操作
		case 41:
			{
				int major = 0;
				int minor = 0;
				int uuid = 0xe1;
				major = pdata[5]*256 + pdata[6];
				minor = pdata[3]*256 + pdata[4];
				set_ibeacon_data(major,minor,uuid);
				set_ble_mode(IBEACON_SENDER);
			}
			break;

		//接收机设置
		case 42:
			{
				int major = 0;
				int minor = 0;
				int uuid = 0xe2;
				major = pdata[5]*256 + pdata[6];
				minor = pdata[3]*256 + pdata[4];
				set_ibeacon_data(major,minor,uuid);
				set_ble_mode(IBEACON_SENDER);
			}
			break;

		//退出广播指令
		case 43:
			set_ble_mode(IBEACON_RECEIVER);
			break;

		//ESP获取发射机参数
		case 61:
			{
				int deviceId = 0;
				int rssinum = 0;
				deviceId = 256 * pdata[4] + pdata[5];
				set_device_id(deviceId);
				rssinum = pdata[9];
				set_rssinum(rssinum);
			}
			break;
		
		//GD获取指派任务id
		case 62:
			{
				int result = 0;
				xEventGroupWaitBits(wifi_sta_get_event_group(), WIFI_STA_EVENT_GROUP_CONNECTED_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
				result = command_guide_nowtask_process("t", "kavin");//获取讲解员被指派任务
				//从服务器获取任务id
				if(result){
					senddata[0] = 0xaa;
					senddata[1] = 0x06;
					senddata[2] = 0x3E;
					senddata[3] = 0x16;	//任务id：get_task_id()/256；get_task_id()mod256
					senddata[4] = 0x15;
					checksum(senddata,senddata[1]-1);
					uart_write_bytes(EX_UART_NUM, (const char*)senddata, senddata[1]);
				}
			}
			break;
		
		//GD请求开始任务,接收到任务请求开始，判断请求位是否为1，并且请求开始id是否是服务器指派任务id，如果是，告知服务器，并反馈给gd
		case 63:
			{
				int result = 0;
				if(get_task_id() == pdata[3]*256+pdata[4] && pdata[5] == 1){
					xEventGroupWaitBits(wifi_sta_get_event_group(), WIFI_STA_EVENT_GROUP_CONNECTED_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
					result = command_guide_begin_process("t", "1234", "kavin");
					if(result){
						senddata[0] = 0xaa;
						senddata[1] = 0x07;
						senddata[2] = 0x3F;
						senddata[3] = 0x16;	//任务id
						senddata[4] = 0x12;
						senddata[5] = 0x01;
						checksum(senddata,senddata[1]-1);
						uart_write_bytes(EX_UART_NUM, (const char*)senddata, senddata[1]);
					}
				}
			}
			break;
		
		//GD请求结束任务,接收到任务请求结束，判断请求位是否为1，并且请求结束id是否是服务器指派任务id，如果是，告知服务器，并反馈给gd
		case 64:
			{
				int result = 0;
				if(get_task_id() == pdata[3]*256+pdata[4] && pdata[5] == 1){
					xEventGroupWaitBits(wifi_sta_get_event_group(), WIFI_STA_EVENT_GROUP_CONNECTED_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
					result = command_guide_finish_process("t", "1234", "kavin");
					if(result){
						senddata[0] = 0xaa;
						senddata[1] = 0x07;
						senddata[2] = 0x40;
						senddata[3] = 0x16;	//任务id
						senddata[4] = 0x12;
						senddata[5] = 0x01;
						checksum(senddata,senddata[1]-1);
						uart_write_bytes(EX_UART_NUM, (const char*)senddata, senddata[1]);
					}
				}
			}
			break;
		default:
			break;
	}
}


/*****************************************************************
  * UART任务处理函数，对UART的队列数据处理
 ****************************************************************/
static void uart_event_task(void *pvParameters)
{
	uart_event_t event;
	size_t buffered_size;
	uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
	uint8_t uarttmp = 0;
	ESP_LOGI(TAG, "k5 uart test");
	
	for(;;) {
		//Waiting for UART event.
		if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)portMAX_DELAY)){
			bzero(dtmp, RD_BUF_SIZE);
			ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
		
			switch(event.type){
				//Event of UART receving data
				case UART_DATA:
					ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
					uart_read_bytes(EX_UART_NUM, dtmp+uarttmp, event.size, portMAX_DELAY);
					if(dtmp[0]==0x55)
					{
						uarttmp+=event.size;
						if(uarttmp>=dtmp[1])
						{
							uarttmp = 0;
							if(dtmp[1]>3)
							{
								uint8_t i = 0,check = 0;
								for(i=0;i<dtmp[1]-1;i++){ check^=dtmp[i]; }
								if(check==dtmp[dtmp[1]-1]){ uart_data_process(dtmp); }
							}
						}
					}else{
						uarttmp = 0;
					}
					break;
				
				//Event of HW FIFO overflow detected
				case UART_FIFO_OVF:
					ESP_LOGI(TAG, "hw fifo overflow");
					// If fifo overflow happened, you should consider adding flow control for your application.
					// The ISR has already reset the rx FIFO,
					// As an example, we directly flush the rx buffer here in order to read more data.
					uart_flush_input(EX_UART_NUM);
					xQueueReset(uart0_queue);
					break;
					
				//Event of UART ring buffer full
				case UART_BUFFER_FULL:
					ESP_LOGI(TAG, "ring buffer full");
					// If buffer full happened, you should consider encreasing your buffer size
					// As an example, we directly flush the rx buffer here in order to read more data.
					uart_flush_input(EX_UART_NUM);
					xQueueReset(uart0_queue);
					break;
					
				//Event of UART RX break detected
				case UART_BREAK:
					ESP_LOGI(TAG, "uart rx break");
					break;
				
				//Event of UART parity check error
				case UART_PARITY_ERR:
					ESP_LOGI(TAG, "uart parity error");
					break;
				
				//Event of UART frame error
				case UART_FRAME_ERR:
					ESP_LOGI(TAG, "uart frame error");
					break;
				
				//UART_PATTERN_DET
				case UART_PATTERN_DET:
					uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
					int pos = uart_pattern_pop_pos(EX_UART_NUM);
					ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
					if (pos == -1) {
						// There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
						// record the position. We should set a larger queue size.
						// As an example, we directly flush the rx buffer here.
						uart_flush_input(EX_UART_NUM);
					} else {
						uart_read_bytes(EX_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
						uint8_t pat[PATTERN_CHR_NUM + 1];
						memset(pat, 0, sizeof(pat));
						uart_read_bytes(EX_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
						ESP_LOGI(TAG, "read data: %s", dtmp);
						ESP_LOGI(TAG, "read pat : %s", pat);
					}
					break;
					
				//Others
				default:
					ESP_LOGI(TAG, "uart event type: %d", event.type);
					break;
			}
		}
	}
	free(dtmp);
	dtmp = NULL;
	vTaskDelete(NULL);
}
	

/*****************************************************************
  * UART初始化
 ****************************************************************/
static void uart_init()
{
	/* Configure parameters of an UART driver,communication pins and install the driver */
	uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};
	uart_param_config(EX_UART_NUM, &uart_config);

	//Set UART log level
	esp_log_level_set(TAG, ESP_LOG_INFO);
	uart_set_pin(EX_UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	//Install UART driver, and get the queue.
	uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);

	//Set uart pattern detect function.
	uart_enable_pattern_det_intr(EX_UART_NUM, '+', PATTERN_CHR_NUM, 10000, 10, 10);
	//Reset the pattern queue length to record at most 20 pattern positions.
	uart_pattern_queue_reset(EX_UART_NUM, 20);
}

/*****************************************************************
  * 创建uart进程之后ESP32向GD发送指令获取设备信息
 ****************************************************************/
static void get_device_info()
{
	static uint8_t senddata[5];
	senddata[0] = 0xaa;
	senddata[1] = 0x04;
	senddata[2] = 0x3D;
	checksum(senddata,senddata[1]-1);
	uart_write_bytes(EX_UART_NUM, (const char*)senddata, senddata[1]);
}


void uart_process()
{
	uart_init();
	ESP_LOGI(TAG, "k5 uart start");
	gd_event_group = xEventGroupCreate();
	//Create a task to handler UART event from ISR
	xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
	get_device_info();
	xEventGroupSetBits(gd_event_group, GD_REC_START);
}
void set_uart_cmdbit(const EventBits_t uxBitsToSet )
{
    xEventGroupSetBits(gd_event_group, uxBitsToSet);
}
void clear_uart_cmdbit(const EventBits_t uxBitsToSet )
{
    xEventGroupClearBits(gd_event_group, uxBitsToSet);
}

EventGroupHandle_t get_uart_cmdbit(void)
{
    return gd_event_group;
}


