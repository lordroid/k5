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
 *  name:		file_upload.h
 *	introduce:	该模块负责上传。
 *  author:		Kavinkun on 2018/07/19
 *  company:	www.hengdawb.com
 */

#ifndef __FILE_UPLOAD_H__
#define __FILE_UPLOAD_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "freertos/event_groups.h"
#define FTP_STOR_DONE   (1 << 0)
#define FTP_LOGEING_OK 	(1 << 1)
#define FTP_DATALINK_OK	(1 << 2)
#define FTP_CWD_DONE    (1 << 3)
#define FTP_MKD_DONE    (1 << 4)
#define FTP_STOR_CMD    (1 << 5)
#define FTP_ALL_DONE    (1 << 6)

EventGroupHandle_t ftp_event_group;

esp_err_t ftp_ul_init(void);
void ftp_ul_process(int filenum,char* uptm);
EventGroupHandle_t get_ftp_statusbit(void);


#ifdef __cplusplus
}
#endif

#endif

