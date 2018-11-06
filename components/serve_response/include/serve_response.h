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
 *  name:		serve_response.h
 *	introduce:	该模块负责esp32和网络后台进行http数据应答的处理
 *  author:		Kavinkun on 2018/08/06
 *  company:	www.hengdawb.com
 */

#ifndef __SERVE_RESPONSE_H__
#define __SERVE_RESPONSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_http_client.h"


int command_deviceno_positions_process(char *p, char *api_token, char *taskid, char *addtime, char *auto_num);
int command_deviceno_listens_process(char *p, char *taskid, char *file_name, char *creattime, char *path);
int command_guide_begin_process(char *p, char *taskid, char *api_token);
int command_guide_finish_process(char *p, char *taskid, char *api_token);
int command_guide_nowtask_process(char *p, char *api_token);


#ifdef __cplusplus
}
#endif

#endif
