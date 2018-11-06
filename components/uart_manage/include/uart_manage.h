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

#ifndef __UART_MANAGE_H__
#define __UART_MANAGE_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/event_groups.h"

#define EX_UART_NUM UART_NUM_2
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define GD_REC_START    (1 << 0)
#define GD_REC_END  	(1 << 1)

void uart_process();
EventGroupHandle_t get_uart_cmdbit(void);
void clear_uart_cmdbit(const EventBits_t uxBitsToSet );
void set_uart_cmdbit(const EventBits_t uxBitsToSet );


#ifdef __cplusplus
}
#endif

#endif





