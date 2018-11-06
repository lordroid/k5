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
 *  name:		serve_response.c
 *	introduce:	该模块负责esp32和网络后台进行http数据应答的处理
 *  author:		Kavinkun on 2018/08/06
 *  company:	www.hengdawb.com
 */

#include <string.h>
#include "esp_log.h"
#include "cJSON.h"

#include "serve_response.h"

#define TAG "serve_response"

#ifndef mem_check
#define mem_check(x) assert(x)
#endif

#define MAX_TRANSLATE_BUFFER (2048)
#define DEFAULT_HTTP_HOST 	"192.168.10.158"
#define DEFAULT_USER_AGENT 	"ESP32 HTTP Client/1.0"
#define DEFAULT_ACCEPT 		"application/json"

/*定义ESP32接受GD的指令类别数组*/
static enum{
	BASE = 0,
	DEVICENO_TEAM_LOCATION,
	DEVICENO_LOCATION_UPLOAD,
	DEVICENO_REC_UPLOAD,
	GUIDE_BEGIN,
	GUIDE_FINISH,
}SERVER_REQ_TYPE_E;

/*设置服务器接口api*/
static char* server_req_type[] = { 
	[BASE] = "http://192.168.10.158:8112/api/",
	[DEVICENO_TEAM_LOCATION] = "http://192.168.10.158:8112/api/task/teampositions",
	[DEVICENO_LOCATION_UPLOAD] = "http://192.168.10.158:8112/api/task/positions", 
	[DEVICENO_REC_UPLOAD] = "http://192.168.10.158:8112/api/task/listen",
	[GUIDE_BEGIN] = "http://192.168.10.158:8112/api/task/begin",
	[GUIDE_FINISH] = "http://192.168.10.158:8112/api/task/finish",
};


/*方法二配套函数
static char *splicing_string(char *first_str, int len_first, char *second_str, int len_second)
{
    int first_str_len = len_first > 0 ? len_first : strlen(first_str);
    int second_str_len = len_second > 0 ? len_second : strlen(second_str);
    char *ret = NULL;
    if (first_str_len + second_str_len > 0) {
        ret = calloc(1, first_str_len + second_str_len + 1);
        mem_check(ret);
        memcpy(ret, first_str, first_str_len);
        memcpy(ret + first_str_len, second_str, second_str_len);
    }
    return ret;
}
*/

/*方法四配套函数
static char *form_url_encode_strcat(char *dst,char *key,char *value,bool is_head)
{
	if(is_head){
		strncat(dst,key,strlen(key));
		strncat(dst,"=",strlen("="));
		strncat(dst,value,strlen(value));
	} else {
		strncat(dst,"&",strlen("&"));
		strncat(dst,key,strlen(key));
		strncat(dst,"=",strlen("="));
		strncat(dst,value,strlen(value));
	}
	return dst;
}
*/


/*****************************************************************
  * 设备定位上传函数接口
 ****************************************************************/
int command_deviceno_positions_process(char *p, char *api_token, char *taskid, char *addtime, char *auto_num)
{
	char data_buf[200] = {};
	int status=0;
	esp_http_client_handle_t client = NULL;

/*方法一
	cJSON *post_position_json = cJSON_CreateObject();
	cJSON_AddItemToObject(post_position_json,"p",cJSON_CreateString(p));
	cJSON_AddItemToObject(post_position_json,"api_token",cJSON_CreateString(api_token));
	cJSON_AddItemToObject(post_position_json,"taskid",cJSON_CreateString(taskid));
	cJSON_AddItemToObject(post_position_json,"addtime",cJSON_CreateString(addtime));
	cJSON_AddItemToObject(post_position_json,"auto_num",cJSON_CreateString(auto_num));

	char* text = cJSON_Print(post_position_json);
	sprintf(data_buf,"%s",text);
	free(text);
	cJSON_Delete(post_position_json);
*/

/*方法二
	data_buf = splicing_string("p=", strlen("p="), p,strlen(p));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&api_token=",strlen("&api_token="));
	data_buf = splicing_string(data_buf, strlen(data_buf),api_token,strlen(api_token));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&taskid=",strlen("&taskid="));
	data_buf = splicing_string(data_buf, strlen(data_buf),taskid,strlen(taskid));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&addtime=",strlen("&addtime="));
	data_buf = splicing_string(data_buf, strlen(data_buf),addtime,strlen(addtime));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&auto_num=",strlen("&auto_num="));
	data_buf = splicing_string(data_buf, strlen(data_buf),auto_num,strlen(auto_num));
*/
/*方法三*/
	strncat(data_buf,"p=",strlen("p="));strncat(data_buf,p,strlen(p));
	strncat(data_buf,"&api_token=",strlen("&api_token="));strncat(data_buf,api_token,strlen(api_token));
	strncat(data_buf,"&taskid=",strlen("&taskid="));strncat(data_buf,taskid,strlen(taskid));
    strncat(data_buf,"&addtime=",strlen("&addtime="));strncat(data_buf,addtime,strlen(addtime));
	strncat(data_buf,"&auto_num=",strlen("&auto_num="));strncat(data_buf,auto_num,strlen(auto_num));
/**/
/*方法四
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"p",p,true));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"api_token",api_token,false));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"taskid",taskid,false));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"addtime",addtime,false));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"auto_num",auto_num,false));
*/
	
	esp_http_client_config_t config = {
		.url = "http://192.168.10.88:8081/test",
	    .timeout_ms = 2000,
	};
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, data_buf, strlen(data_buf));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("{function:http_post}", "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE("{function:http_post}", "HTTP POST request failed");
    }
	status = esp_http_client_get_status_code(client);
	esp_http_client_cleanup(client); 
	ESP_LOGI(TAG, "RAM left: %u", esp_get_free_heap_size());
	return status;
}

/*****************************************************************
  * 设备录音上传函数接口
 ****************************************************************/
int command_deviceno_listens_process(char *p, char *taskid, char *file_name, char *creattime, char *path)
{
	char data_buf[200] = {};
	int result = 0;
	esp_http_client_handle_t client = NULL;

/*方法一
	cJSON * post_listens_json = cJSON_CreateObject();
	cJSON_AddItemToObject(post_listens_json,"p",cJSON_CreateString(p));
	cJSON_AddItemToObject(post_listens_json,"taskid",cJSON_CreateString(taskid));
	cJSON_AddItemToObject(post_listens_json,"file_name",cJSON_CreateString(file_name));
	cJSON_AddItemToObject(post_listens_json,"creattime",cJSON_CreateString(creattime));
	cJSON_AddItemToObject(post_listens_json,"path",cJSON_CreateString(path));

	char* text = cJSON_Print(post_listens_json);
	sprintf(data_buf,"%s",text);
	free(text);
	cJSON_Delete(post_listens_json);
*/

/*方法二
	data_buf = splicing_string("p=", strlen("p="), p,strlen(p));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&taskid=",strlen("&taskid="));
	data_buf = splicing_string(data_buf, strlen(data_buf),taskid,strlen(taskid));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&file_name=",strlen("&file_name="));
	data_buf = splicing_string(data_buf, strlen(data_buf),file_name,strlen(file_name));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&creattime=",strlen("&creattime="));
	data_buf = splicing_string(data_buf, strlen(data_buf),creattime,strlen(creattime));

	data_buf = splicing_string(data_buf, strlen(data_buf),"&path=",strlen("&path="));
	data_buf = splicing_string(data_buf, strlen(data_buf),path,strlen(path));
*/
/*方法三*/
	strncat(data_buf,"p=",strlen("p="));strncat(data_buf,p,strlen(p));
	strncat(data_buf,"&taskid=",strlen("&taskid="));strncat(data_buf,taskid,strlen(taskid));
	strncat(data_buf,"&file_name=",strlen("&file_name="));strncat(data_buf,file_name,strlen(file_name));
    strncat(data_buf,"&creattime=",strlen("&creattime="));strncat(data_buf,creattime,strlen(creattime));
	strncat(data_buf,"&path=",strlen("&path="));strncat(data_buf,path,strlen(path));
/**/
/*方法四
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"p",p,true));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"taskid",taskid,false));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"file_name",file_name,false));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"creattime",creattime,false));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"path",path,false));
*/

	esp_http_client_config_t config = {
		.url = "http://192.168.10.88:8081/test",
	    .timeout_ms = 2000,
	};
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, data_buf, strlen(data_buf));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("{function:http_post}", "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE("{function:http_post}", "HTTP POST request failed");
    }
	result = esp_http_client_get_status_code(client);
	esp_http_client_cleanup(client); 
	
	return result;
}

/*****************************************************************
  * 设备开始讲解函数接口
 ****************************************************************/
int command_guide_begin_process(char *p, char *taskid, char *api_token)
{
	char data_buf[200] = {};
	int result = 0;
	esp_http_client_handle_t client = NULL;

/*方法一
	cJSON * get_begin_json = cJSON_CreateObject();
	cJSON_AddItemToObject(get_begin_json,"p",cJSON_CreateString(p));
	cJSON_AddItemToObject(get_begin_json,"taskid",cJSON_CreateString(taskid));
	cJSON_AddItemToObject(get_begin_json,"api_token",cJSON_CreateString(api_token));

	char* text = cJSON_Print(get_begin_json);
	sprintf(data_buf,"%s",text);
	free(text);
	cJSON_Delete(get_begin_json);
*/

/*方法二
	data_buf = splicing_string("p=", strlen("p="), p,strlen(p));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&taskid=",strlen("&taskid="));
	data_buf = splicing_string(data_buf, strlen(data_buf),taskid,strlen(taskid));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&api_token=",strlen("&api_token="));
	data_buf = splicing_string(data_buf, strlen(data_buf),api_token,strlen(api_token));
*/
/*方法三*/
	strncat(data_buf,"p=",strlen("p="));strncat(data_buf,p,strlen(p));
	strncat(data_buf,"&taskid=",strlen("&taskid="));strncat(data_buf,taskid,strlen(taskid));
	strncat(data_buf,"&api_token=",strlen("&api_token="));strncat(data_buf,api_token,strlen(api_token));
/**/
/*方法四
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"p",p,true));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"taskid",taskid,false));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"api_token",api_token,false));
*/

	esp_http_client_config_t config = {
		.url = "http://192.168.10.88:8081/test",
	    .timeout_ms = 2000,
	};
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_post_field(client, data_buf, strlen(data_buf));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("{function:http_post}", "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE("{function:http_post}", "HTTP POST request failed");
    }
	result = esp_http_client_get_status_code(client);
	esp_http_client_cleanup(client); 
	
	return result;
}

/*****************************************************************
  * 设备结束讲解函数接口
 ****************************************************************/
int command_guide_finish_process(char *p, char *taskid, char *api_token)
{
	char data_buf[200] = {};
	int result = 0;
	esp_http_client_handle_t client = NULL;

/*方法一
	cJSON * get_finish_json = cJSON_CreateObject();
	cJSON_AddItemToObject(get_finish_json,"p",cJSON_CreateString(p));
	cJSON_AddItemToObject(get_finish_json,"taskid",cJSON_CreateString(taskid));
	cJSON_AddItemToObject(get_finish_json,"api_token",cJSON_CreateString(api_token));

	char* text = cJSON_Print(get_finish_json);
	sprintf(data_buf,"%s",text);
	free(text);
	cJSON_Delete(get_finish_json);
*/

/*方法二
	data_buf = splicing_string("p=", strlen("p="), p,strlen(p));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&taskid=",strlen("&taskid="));
	data_buf = splicing_string(data_buf, strlen(data_buf),taskid,strlen(taskid));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&api_token=",strlen("&api_token="));
	data_buf = splicing_string(data_buf, strlen(data_buf),api_token,strlen(api_token));
*/
/*方法三*/
	strncat(data_buf,"p=",strlen("p="));strncat(data_buf,p,strlen(p));
	strncat(data_buf,"&taskid=",strlen("&taskid="));strncat(data_buf,taskid,strlen(taskid));
	strncat(data_buf,"&api_token=",strlen("&api_token="));strncat(data_buf,api_token,strlen(api_token));
/**/
/*方法四
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"p",p,true));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"taskid",taskid,false));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"api_token",api_token,false));
*/

	esp_http_client_config_t config = {
		.url = "http://192.168.10.88:8081/test",
	    .timeout_ms = 2000,
	};
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_post_field(client, data_buf, strlen(data_buf));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("{function:http_post}", "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE("{function:http_post}", "HTTP POST request failed");
    }
	result = esp_http_client_get_status_code(client);
	esp_http_client_cleanup(client); 
	
	return result;
}


/*****************************************************************
  * 判断讲解员当前是否有指派任务
 ****************************************************************/
int command_guide_nowtask_process(char *p, char *api_token)
{
	char data_buf[200] = {};
	int result = 0;
	esp_http_client_handle_t client = NULL;

/*方法一
	cJSON * get_begin_json = cJSON_CreateObject();
	cJSON_AddItemToObject(get_begin_json,"p",cJSON_CreateString(p));
	cJSON_AddItemToObject(get_begin_json,"api_token",cJSON_CreateString(api_token));

	char* text = cJSON_Print(get_begin_json);
	sprintf(data_buf,"%s",text);
	free(text);
	cJSON_Delete(get_begin_json);
*/

/*方法二
	data_buf = splicing_string("p=", strlen("p="), p,strlen(p));
	
	data_buf = splicing_string(data_buf, strlen(data_buf),"&api_token=",strlen("&api_token="));
	data_buf = splicing_string(data_buf, strlen(data_buf),api_token,strlen(api_token));
*/
/*方法三*/
	strncat(data_buf,"p=",strlen("p="));strncat(data_buf,p,strlen(p));
	strncat(data_buf,"&api_token=",strlen("&api_token="));strncat(data_buf,api_token,strlen(api_token));
/**/
/*方法四
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"p",p,true));
	sprintf(data_buf,"%s",form_url_encode_strcat(data_buf,"api_token",api_token,false));
*/

	esp_http_client_config_t config = {
		.url = "http://192.168.10.88:8081/test",
	    .timeout_ms = 2000,
	};
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_post_field(client, data_buf, strlen(data_buf));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("{function:http_post}", "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE("{function:http_post}", "HTTP POST request failed");
    }
	result = esp_http_client_get_status_code(client);
	esp_http_client_cleanup(client); 
	
	return result;
}


