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
 *  name:		ota_manage.c
 *	introduce:	该模块负责触发和协调固件更新。
 *  author:		Kavinkun on 2018/07/19
 *  company:	www.hengdawb.com
 */

#include <string.h>
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/opt.h"
#include "lwip/ip.h"
#include "lwip/ip4_addr.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "mdns.h"
#include "sntp_get_time.h"
#include "file_upload.h"
#include "record_sdcard.h"
#include "wifi_sta.h"
#include "common.h"

#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#define TAG "file_upload"


#define PTRNLEN(s)  s,(sizeof(s)-1)
#define min(a,b) ((a) < (b) ? (a) : (b))
#define BUFFSIZE 		1500


/*FTP返回状态*/
 enum ftp_results {
  FTP_RESULT_OK=0,
  FTP_RESULT_INPROGRESS,
  FTP_RESULT_LOGGED,
  FTP_RESULT_ERR_UNKNOWN,   /** Unknown error */
  FTP_RESULT_ERR_ARGUMENT,  /** Wrong argument */
  FTP_RESULT_ERR_MEMORY,    /** Out of memory */
  FTP_RESULT_ERR_CONNECT,   /** Connection to server failed */
  FTP_RESULT_ERR_HOSTNAME,  /** Failed to resolve server hostname */
  FTP_RESULT_ERR_CLOSED,    /** Connection unexpectedly closed by remote server */
  FTP_RESULT_ERR_TIMEOUT,   /** Connection timed out (server didn't respond in time) */
  FTP_RESULT_ERR_SRVR_RESP, /** Server responded with an unknown response code */
  FTP_RESULT_ERR_INTERNAL,  /** Internal network stack error */
  FTP_RESULT_ERR_LOCAL,     /** Local storage error */
  FTP_RESULT_ERR_FILENAME   /** Remote host could not find file */
};

/*FTP控制连接状态*/
 typedef enum  {
  FTP_CLOSED=0,
  FTP_CONNECTED,
  FTP_USER_SENT,
  FTP_PASS_SENT,
  FTP_LOGGED,
  FTP_TYPE_SENT,
  FTP_PASV_SENT,
  FTP_RETR_SENT,
  FTP_STOR_SENT,
  FTP_XFERING,
  FTP_DATAEND,
  FTP_QUIT,
  FTP_QUIT_SENT,
  FTP_CWD_SENT,
  FTP_MKD_SENT,
  FTP_CLOSING,
  FTP_IDLE,
} ftp_state_t;

/*FTP会话结构*/
typedef struct {
  //用户层面
  ip_addr_t    server_ip;
  u16_t         server_port;
  u16_t         data_port;
  const char    *remote_path;
  const char    *user;
  const char    *pass;
  void          *handle;
  uint          (*data_sink)(void*, const char*, uint);	//数据接收器
  //网路数据
  ftp_state_t   control_state;
  ftp_state_t   target_state;
  ftp_state_t   data_state;
  struct tcp_pcb  *control_pcb;
  struct tcp_pcb  *data_pcb;
} ftp_session_t;
static char dirname[50];
static const char *fstor="/sdcard/record/ftp.txt";
static FILE* f2stor;
/*ftp的配置参数*/
static ftp_session_t ftp_config;
static u_char filebuf[4096];
static int wfile_num=0;
/*ftp group*/

static char dbgtm[40];

static int dbgcnt = 0;

static void ftp_control_process(ftp_session_t *s, struct tcp_pcb *tpcb, struct pbuf *p);
/**************************************************************
 *定时器相关函数，调试用
 *
 *
***************************************************************/




/*****************************************************************
  * 给连接控制器发送一个消息
  * @param pointer to ftp session data
  * @param pointer to message string
****************************************************************/
static err_t ftp_send_msg(ftp_session_t *s, const char* msg, size_t len)
{
  err_t error;

  ESP_LOGI(TAG, "ftp:sending %s",msg);
  error = tcp_write(s->control_pcb, msg, len, 0);
  if ( error != ERR_OK ) {
	  ESP_LOGI(TAG, "ftp:cannot write (%s)\n",lwip_strerr(error));
  }
  return error;
}

/*****************************************************************
  * 关闭PCB
  * @param pointer to ftp session data
****************************************************************/
static err_t ftp_pcb_close(struct tcp_pcb *tpcb)
{
  err_t error;

  tcp_err(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  error = tcp_close(tpcb);
  if ( error != ERR_OK ) {
	ESP_LOGI(TAG, "ftp:pcb close failure, not implemented\n");
  }
  return ERR_OK;
}


/*****************************************************************
  * 数据连接通道接受处理
  * @param pointer to ftp session data
  * @param pointer to PCB
  * @param pointer to incoming pbuf
  * @param state of incoming process
****************************************************************/
static err_t ftp_data_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  ftp_session_t *s = (ftp_session_t*)arg;
  if (p) {
	if (s->data_sink) {
	  struct pbuf *q;
	  for (q=p; q; q=q->next) {
		s->data_sink(s->handle, q->payload, q->len);
	  }
	} else {
	  ESP_LOGI(TAG, "ftp: sinking %d bytes\n",p->tot_len);
	}
	tcp_recved(tpcb, p->tot_len);
	pbuf_free(p);
  } else {
	// NULL pbuf shall lead to close the pcb. Close is postponed after
	// the session state machine updates. No need to close right here.
	// Instead we kindly tell data sink we are done
	if (s->data_sink) {
	  s->data_sink(s->handle, NULL, 0);
	}
  }
  return ERR_OK;
}

static err_t ftp_data_sent(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  int readlen=0;
  ftp_session_t *s = (ftp_session_t*)arg;
  
  if (p)
  {
      	readlen=fread((void*)filebuf,1,2000,f2stor);

		if(readlen>0)
		{
		    
 		    readlen = tcp_write(s->data_pcb, filebuf,readlen, 0);
			tcp_output(s->data_pcb);

            if ( readlen != ERR_OK ) {
	            ESP_LOGI(TAG, "ftp:stor file cannot write (%s)\n",lwip_strerr(readlen));
            }           
		}
		else
		{   

            ESP_LOGI(TAG, "file stor done\n");
			fclose(f2stor);
			ftp_pcb_close(s->data_pcb);
			s->data_pcb=NULL;
		}
  }
  
  return ERR_OK;
}


/*****************************************************************
  * 数据通道错误处理
  * @param pointer to ftp session data
  * @param state of connection
****************************************************************/
static void ftp_data_err(void *arg, err_t err)
{
  LWIP_UNUSED_ARG(err);
  if (arg != NULL) {
	ftp_session_t *s = (ftp_session_t*)arg;
    ESP_LOGI(TAG, "ftp:failed/error connecting for data to server (%s)\n",lwip_strerr(err));
	s->data_pcb = NULL; // No need to de-allocate PCB
	if (s->control_state==FTP_XFERING) { // gracefully move control session ahead
	  s->control_state = FTP_DATAEND;
	  ftp_control_process(s, NULL, NULL);
	}
  }
}


/*****************************************************************
  * 数据通道连接
  * @param pointer to ftp session data
  * @param pointer to PCB
  * @param state of connection
****************************************************************/
static err_t ftp_data_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  ftp_session_t *s = (ftp_session_t*)arg;

  if ( err == ERR_OK ) {
	ESP_LOGI(TAG, "ftp:connected for data to server\n");

	s->data_state = FTP_CONNECTED;
    xEventGroupSetBits(ftp_event_group, FTP_DATALINK_OK);
  } else {
	ESP_LOGI(TAG, "ftp:err in data_connected (%s)\n",lwip_strerr(err));
  }
  return err;
}


/*****************************************************************
  * 打开被动传输模式的数据连接通道
  * @param pointer to ftp session data
  * @param pointer to incoming PASV response
****************************************************************/
static err_t ftp_data_getport(ftp_session_t *s, struct pbuf *p)
{
	
	  char *ptr;
	  u16_t data_port;
//	  u32_t ip_temp;
	  // Find server connection parameter
	  ptr = strchr(p->payload, '(');
	  if (!ptr) return ERR_BUF;

	  data_port  = strtoul(ptr+1,&ptr,10) << 8;
	  data_port |= strtoul(ptr+1,&ptr,10) & 255;
	  s->data_port=data_port;
	  if (*ptr!=')') return ERR_BUF;
	  return ERR_OK;
	
}

static err_t ftp_data_open(ftp_session_t *s,struct pbuf *p)
{
	err_t error;
	char *ptr;
	u16_t data_port;
	u32_t ip_temp;
	
	// Find server connection parameter
	ptr = strchr(p->payload, '(');
	if (!ptr) return ERR_BUF;
	do {
	  ip_temp = strtoul(ptr+1,&ptr,10);
	  ip_temp = strtoul(ptr+1,&ptr,10);
	  ip_temp = strtoul(ptr+1,&ptr,10);
	  ip_temp = strtoul(ptr+1,&ptr,10);
//	  ESP_LOGI(TAG, "We don`t need this para,It just get port In the following %d",ip_temp);
	} while(0);
	data_port  = strtoul(ptr+1,&ptr,10) << 8;
	data_port |= strtoul(ptr+1,&ptr,10) & 255;
	ESP_LOGI(TAG, "pasv receive data is %s",(char*)p->payload);
	ESP_LOGI(TAG, "data port is %d",data_port);
	if (*ptr!=')') return ERR_BUF;
	
	// Open data session
	tcp_arg(s->data_pcb, s);

	tcp_err(s->data_pcb, ftp_data_err);
	tcp_recv(s->data_pcb, ftp_data_recv);
	tcp_sent(s->data_pcb, ftp_data_sent);
	error = tcp_connect(s->data_pcb, &s->server_ip, data_port, ftp_data_connected);
	return error;

}

static err_t ftp_stor_ready(ftp_session_t *s,char *dstfilename)
{
  err_t error;
  
  error=ftp_send_msg(s, PTRNLEN("STOR "));
  error=ftp_send_msg(s, dstfilename, strlen(dstfilename));
  error=ftp_send_msg(s, PTRNLEN("\r\n"));
  return error;
}


static err_t ftp_stor_begin(ftp_session_t *s,char *sourcefilename)
{
  err_t error;
  char namebuf[60];
  int readlen=0;
  
  f2stor=fopen(sourcefilename,"rb");
  if(f2stor==NULL)
  {
	    ESP_LOGI(TAG, "open file %s failed!\n",fstor);
		ftp_pcb_close(s->data_pcb);
		s->data_pcb=NULL;
	    return;
  } 
  readlen=fread((void*)filebuf,1,2000,f2stor);
  if(readlen==0)
  {
  		fclose(f2stor);
		ftp_pcb_close(s->data_pcb);
		s->data_pcb=NULL;
	    return;       
  }
  error = tcp_write(s->data_pcb, filebuf, readlen, 0);
  tcp_output(s->data_pcb);
  if ( error != ERR_OK ) {
      ESP_LOGI(TAG, "ftp:stor file cannot write (%s)\n",lwip_strerr(readlen));
  }  
  return error;

}

/*****************************************************************
  * 关闭数据连接
  * @param pointer to ftp session data
  * @param result to pass to callback fn (if called)
****************************************************************/
static void ftp_data_close(ftp_session_t *s, int result)
{
  if (s->data_pcb) {
	ftp_pcb_close(s->data_pcb);
	s->data_pcb = NULL;
  }
}


/*****************************************************************
 *****************************************************************
 *****************************************************************
  *	以上是ftp_data层
*****************************************************************
*****************************************************************
*****************************************************************/


/*****************************************************************
  * 开始下载数据
  * @param pointer to ftp session
****************************************************************/
static void ftp_start_RETR(void *arg)
{
  ftp_session_t *s = (ftp_session_t*)arg;
  
  if ( s->control_state == FTP_LOGGED ) {
	ftp_send_msg(s, PTRNLEN("TYPE I\n"));
	s->control_state = FTP_TYPE_SENT;
	s->target_state = FTP_RETR_SENT;
  } else {
	ESP_LOGI(TAG, "Unexpected condition");
  }
}

static void ftp_start_STOR(void *arg)
{
  ftp_session_t *s = (ftp_session_t*)arg;
//  tg0_timer_init(TIMER_0, 0);
    s->control_state = FTP_TYPE_SENT;
	s->target_state = FTP_STOR_SENT;
	ftp_send_msg(s, PTRNLEN("TYPE I\r\n"));
}

static void ftp_init_STOR(void *arg)
{
  ftp_session_t *s = (ftp_session_t*)arg;
//  xEventGroupSetBits(ftp_event_group, FTP_STOR_DONE);
}

/*****************************************************************
  * 给控制器发送QUIT指令
  * @param pointer to ftp session
****************************************************************/
static void ftp_send_QUIT(void *arg)
{
  ftp_session_t *s = (ftp_session_t*)arg;

  if (s->control_pcb) {
	ftp_send_msg(s, PTRNLEN("QUIT\r\n"));
	tcp_output(s->control_pcb);
	s->control_state = FTP_QUIT_SENT;
  }
}

static void ftp_CWD(void *arg,char *dir )
{
    ftp_session_t *s = (ftp_session_t*)arg;
    s->control_state = FTP_CWD_SENT;
	ftp_send_msg(s, PTRNLEN("CWD "));
	ftp_send_msg(s, dir, strlen(dir));
    ftp_send_msg(s, PTRNLEN("\r\n"));
	
}
static void ftp_MKD(void *arg,char *dir)
{
  ftp_session_t *s = (ftp_session_t*)arg;
  s->control_state = FTP_MKD_SENT;
  ftp_send_msg(s, PTRNLEN("MKD "));
  ftp_send_msg(s, dir, strlen(dir));
  ftp_send_msg(s, PTRNLEN("\r\n"));
 

}

/*****************************************************************
  * FTP控制连接关闭
  * @param pointer to ftp session data
  * @param result to pass to callback fn (if called)
****************************************************************/
static void ftp_control_close(ftp_session_t *s, int result)
{
  if (s->data_pcb) {
	ftp_pcb_close(s->data_pcb);
	s->data_pcb = NULL;
  }
  if (s->control_pcb) {
	ftp_pcb_close(s->control_pcb);
	s->control_pcb = NULL;
  }
  s->control_state = FTP_CLOSED;
  xEventGroupSetBits(ftp_event_group, FTP_ALL_DONE);
}


/*****************************************************************
  * 处理控制器连接收到的数据
  * @param pointer to ftp session data
  * @param pointer to PCB
  * @param pointer to incoming pbuf
  * @param state of incoming process
****************************************************************/
static err_t ftp_control_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  ftp_session_t *s = (ftp_session_t*)arg;

  if ( err == ERR_OK ) {
	if (p) {
	  tcp_recved(tpcb, p->tot_len);
	  ftp_control_process(s, tpcb, p);
	} else {
	  ESP_LOGI(TAG, "ftp:connection closed by remote host\n");
	  ftp_control_close(s, FTP_RESULT_ERR_CLOSED);
	}
  } else {
	ESP_LOGI(TAG, "ftp:failed to receive (%s)\n",lwip_strerr(err));
	ftp_control_close(s, FTP_RESULT_ERR_UNKNOWN);
  }
  return err;
}


/*****************************************************************
  * 处理发送数据的控制连接确认
  * @param pointer to ftp session data
  * @param pointer to PCB
  * @param number of bytes sent
****************************************************************/
static err_t ftp_control_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  ESP_LOGI(TAG, "ftp:successfully sent %d bytes\n",len);
  return ERR_OK;
}


/*****************************************************************
  * 协议控制块连接错误处理
  * @param pointer to ftp session data
  * @param state of connection
****************************************************************/
static void ftp_control_err(void *arg, err_t err)
{
  LWIP_UNUSED_ARG(err);
  if (arg != NULL) {
	ftp_session_t *s = (ftp_session_t*)arg;
	int result;
	if( s->control_state == FTP_CLOSED ) {
	  ESP_LOGI(TAG, "ftp:failed to connect to server (%s)\n",lwip_strerr(err));
	  result = FTP_RESULT_ERR_CONNECT;
	} else {
	  ESP_LOGI(TAG, "ftp:connection closed by remote host\n");
	  result = FTP_RESULT_ERR_CLOSED;
	}
	s->control_pcb = NULL; // No need to de-allocate PCB
	ftp_control_close(s, result);
  }
}


/*****************************************************************
  * 协议控制块连接
  * @param pointer to ftp session data
  * @param pointer to PCB
  * @param state of connection
****************************************************************/
static err_t ftp_control_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  ftp_session_t *s = (ftp_session_t*)arg;

  if ( err == ERR_OK ) {
  	  ESP_LOGI(TAG, "ftp:connected to server\n");
	  s->control_state = FTP_CONNECTED;
  } else {
  	ESP_LOGI(TAG, "ftp:err in control_connected (%s)\n",lwip_strerr(err));
  }
  return err;
}


/*****************************************************************
  * 协议控制块连接
  * @param pointer to ftp session data
  * @param pointer to ptr
  * @param len
****************************************************************/
static uint data_sink(void *arg, const char* ptr, uint len)
{
//    memset(ota_write_data, 0, BUFFSIZE);
//	memcpy(ota_write_data, ptr, len);
//    esp_ota_write( update_handle, (const void *)ota_write_data, len);
//    binary_file_length += len;
//    ESP_LOGI(TAG, "Have written image length %d", binary_file_length);
	return len;
}

/*****************************************************************
  * FTP client 状态处理器
  * @param pointer to ftp session data
  * @param pointer to PCB
  * @param pointer to incoming data
****************************************************************/
static void ftp_control_process(ftp_session_t *s, struct tcp_pcb *tpcb, struct pbuf *p)
{
  uint response = 0;
  int result = FTP_RESULT_ERR_SRVR_RESP; 
//  int readlen=0;
  // Try to get response number
  if (p) {
	response = strtoul(p->payload, NULL, 10);
    ESP_LOGI(TAG, "ftp:got response %d\n",response);
  }

  switch (s->control_state) {
	case FTP_CONNECTED:
	  if (response>0) {
		if (response==220) {
		  ftp_send_msg(s, PTRNLEN("USER "));
		  ftp_send_msg(s, s->user, strlen(s->user));
		  ftp_send_msg(s, PTRNLEN("\r\n"));
		  s->control_state = FTP_USER_SENT;
		} else {
		  s->control_state = FTP_QUIT;
		}
	  }
	  break;
	case FTP_USER_SENT:
	  if (response>0) {
		if (response==331) {
		  ftp_send_msg(s, PTRNLEN("PASS "));
		  ftp_send_msg(s, s->pass, strlen(s->pass));
		  ftp_send_msg(s, PTRNLEN("\r\n"));
		  s->control_state = FTP_PASS_SENT;
		} else {
		  s->control_state = FTP_QUIT;
		}
	  }
	  break;
	case FTP_PASS_SENT:
	  if (response>0) {
		if (response==230) {
		  s->control_state = FTP_LOGGED;
		  ESP_LOGI(TAG, "ftp: now logged in\n");
//		  xEventGroupSetBits(ftp_event_group, FTP_STOR_DONE);
		  xEventGroupSetBits(ftp_event_group, FTP_LOGEING_OK);
		} else {
		  s->control_state = FTP_QUIT;
		}
	  }
	  break;
	case FTP_TYPE_SENT:
	  if (response>0) {
		if (response==200) {
		  ftp_send_msg(s, PTRNLEN("PASV\r\n"));
		  s->control_state = FTP_PASV_SENT;
		} else {
		  s->control_state = FTP_QUIT;
		}
	  }
	  break;
	case FTP_PASV_SENT:
	  if (response>0) {
		if (response==227) {
		  ftp_data_open(s,p);
		  s->control_state = s->target_state;		  
		} else {
		  s->control_state = FTP_QUIT;
		}
	  }
	  break;
	case FTP_RETR_SENT:
	  if (response>0) {
		if (response==150) {
		  	s->control_state = FTP_XFERING;
		} else if (response==550) {
			s->control_state = FTP_DATAEND;
			result = FTP_RESULT_ERR_FILENAME;
			ESP_LOGI(TAG, "ftp: Failed to open file '%s'\n", s->remote_path);
		}else {
		  s->control_state = FTP_DATAEND;
		  ESP_LOGI(TAG, "ftp:expected 150, received %d\n",response);
		}
	  }
	  break;
	  case FTP_STOR_SENT:
		if (response>0) {
		  if ((response==150)||(response==125)) {
		  { 
		      s->control_state = FTP_XFERING;
			  if(s->data_state == FTP_CONNECTED)
			  {
                  s->data_state = FTP_XFERING;
			  }
			  xEventGroupSetBits(ftp_event_group, FTP_STOR_CMD);
		  }
		  } else if (response==550) {
			  s->control_state = FTP_DATAEND;
			  result = FTP_RESULT_ERR_FILENAME;
		  }else {
			s->control_state = FTP_DATAEND;
			ESP_LOGI(TAG, "ftp:expected 150, received %d\n",response);
		  }
		}
		break;

	case FTP_XFERING:
	  if (response>0) {
		if (response==226) {
		  result = FTP_RESULT_OK;
		  xEventGroupSetBits(ftp_event_group, FTP_STOR_DONE);
		  
		} else {
		  result = FTP_RESULT_ERR_CLOSED;
		  ESP_LOGI(TAG, "ftp:expected 226, received %d\n",response);
		}
//		s->control_state = FTP_DATAEND;
	  }
	  break;
	case FTP_DATAEND:
	  ESP_LOGI(TAG,"forced end of data session");
	  break;
	case FTP_CWD_SENT:
	  if (response>0) {
		if (response==250) {
		  xEventGroupSetBits(ftp_event_group, FTP_CWD_DONE);
		} else {
		  result = FTP_RESULT_ERR_UNKNOWN;
		  ESP_LOGI(TAG, "ftp:expected 221, received %d\n",response);
		}
	  }
	  break;	
	 case FTP_MKD_SENT:
		if (response>0) {
		  if (response==257) {
			xEventGroupSetBits(ftp_event_group, FTP_MKD_DONE);
		  } else {
			result = FTP_RESULT_ERR_UNKNOWN;
			ESP_LOGI(TAG, "ftp:expected 221, received %d\n",response);
		  }
		}
		break;
	case FTP_QUIT_SENT:
	  if (response>0) {
		if (response==221) {
		  result = FTP_RESULT_OK;
		  ftp_control_close(s, result);
		} else {
		  result = FTP_RESULT_ERR_UNKNOWN;
		  ESP_LOGI(TAG, "ftp:expected 221, received %d\n",response);
		}
		s->control_state = FTP_CLOSING;
	  }
	  break;
	default:
	  ESP_LOGI(TAG, "ftp:unhandled state (%d)\n",s->control_state);
  }

  // Free receiving pbuf if any
  if (p) {
	pbuf_free(p);
  }

}


/*****************************************************************
  * 创建一个FTP控制连接，下载或者上传需要建立数据连接
  * @param Session structure
****************************************************************/
static err_t ftp_connect(ftp_session_t *s)
{
  err_t error;
  enum ftp_results retval = FTP_RESULT_ERR_UNKNOWN;

  // Check user supplied data
  if ( (s->control_state!=FTP_CLOSED) ||
	   s->control_pcb ||
	   s->data_pcb ||
	   !s->user ||
	   !s->pass )
  {
	ESP_LOGI(TAG, "ftp:invalid control session\n");
	retval = FTP_RESULT_ERR_ARGUMENT;
	goto exit;
  }
  // Get sessions pcb
  s->control_pcb = tcp_new();
  if (!s->control_pcb) {
	ESP_LOGI(TAG, "ftp:cannot alloc control_pcb (low memory?)\n");
	retval = FTP_RESULT_ERR_MEMORY;
	goto exit;
  }
  // Open control session
  tcp_arg(s->control_pcb, s);
  tcp_err(s->control_pcb, ftp_control_err);
  tcp_recv(s->control_pcb, ftp_control_recv);
  tcp_sent(s->control_pcb, ftp_control_sent);
  error = tcp_connect(s->control_pcb, &s->server_ip, s->server_port, ftp_control_connected);
  if ( error == ERR_OK ) {
	retval = FTP_RESULT_INPROGRESS;
	return retval;
  }

  // Release pcbs in case of failure
  ESP_LOGI(TAG, "ftp:cannot connect control_pcb (%s)\n", lwip_strerr(error));
  ftp_control_close(s, -1);

exit:
  return retval;
}


/*****************************************************************
  * 从远程文件下载数据
  * @@param Session structure
****************************************************************/
static err_t ftp_retrieve(ftp_session_t *s)
{
  err_t error;
  enum ftp_results retval = FTP_RESULT_ERR_UNKNOWN;

  // Check user supplied data
  if ( (s->control_state!=FTP_LOGGED) ||
	   !s->remote_path ||
	   s->data_pcb )
  {
	ESP_LOGI(TAG, "ftp:invalid session data\n");
	retval = FTP_RESULT_ERR_ARGUMENT;
	goto exit;
  }
  // Get data pcb
  s->data_pcb = tcp_new();
  if (!s->data_pcb) {
	ESP_LOGI(TAG, "ftp:cannot alloc data_pcb (low memory?)\n");
	retval = FTP_RESULT_ERR_MEMORY;
	goto exit;
  }
  // Initiate transfer
  error = tcpip_callback(ftp_start_RETR, s);
  if ( error == ERR_OK ) {
	retval = FTP_RESULT_INPROGRESS;
	return retval;
  } else {
	ESP_LOGI(TAG, "cannot start RETR (%s)",lwip_strerr(error));
	retval = FTP_RESULT_ERR_INTERNAL;
  }

exit:
  return retval;
}

static err_t ftp_stor(ftp_session_t *s)
{
  err_t error;
  enum ftp_results retval = FTP_RESULT_ERR_UNKNOWN;

  // Check user supplied data
  if (!s->remote_path ||
	   s->data_pcb )
  {
	ESP_LOGI(TAG, "ftp:invalid session data\n");
	retval = FTP_RESULT_ERR_ARGUMENT;
	goto exit;
  }
  // Get data pcb
  s->data_pcb = tcp_new();
  if (!s->data_pcb) {
	ESP_LOGI(TAG, "ftp:cannot alloc data_pcb (low memory?)\n");
	retval = FTP_RESULT_ERR_MEMORY;
	goto exit;
  }
  error = tcpip_callback(ftp_init_STOR, s);
  if ( error == ERR_OK ) {
	retval = FTP_RESULT_INPROGRESS;
	return retval;
  } else {
	ESP_LOGI(TAG, "cannot start STOR (%s)",lwip_strerr(error));
	retval = FTP_RESULT_ERR_INTERNAL;
  }

exit:
  return retval;
}


/*****************************************************************
  * ftp_client模块的初始化配置
 ****************************************************************/
static void ftp_init()
{
	//ftp进行初始化并连接,使用匿名FTP
	memset(&ftp_config, 0, sizeof(ftp_config));
	IP_ADDR4(&ftp_config.server_ip, 192,168,10,50);
	ftp_config.server_port = FTP_SERVER_PORT;
	ftp_config.remote_path = FTP_REMOTE_FILE_PATH;
	ftp_config.user ="yf4z";
	ftp_config.pass = "123123";
	ftp_config.handle = &ftp_config;
	ftp_config.data_sink = data_sink;
}

/*****************************************************************
  * 终止FTP对话
  * @@param Session structure
****************************************************************/
static void ftp_close(ftp_session_t *s)
{
  err_t error;

  // Nothing to do when already closed
  if ( s->control_state == FTP_CLOSED ) 

  {
      xEventGroupSetBits(ftp_event_group, FTP_ALL_DONE);
      return;
  }

  // Initiate transfer
  error = tcpip_callback(ftp_send_QUIT, s);
  if ( error != ERR_OK ) {
	// This is a critical error, try to close anyway
	// polling process may save us
	ESP_LOGI(TAG, "cannot request for close");
	s->control_state = FTP_QUIT;
  }
}
/**************/
static void ftp_ul_task(void *pvParameter)
{
	int filecnt=0;
	char srcname[100],dstname[100];		
        filecnt=1;	
	    while (1) 
	    {
	        xEventGroupWaitBits(ftp_event_group, FTP_STOR_DONE|FTP_ALL_DONE, pdFALSE, pdFALSE, portMAX_DELAY);	//文件传输完成	  
	        if(xEventGroupGetBits(ftp_event_group)&FTP_ALL_DONE)
	        {
	            esp_wifi_stop();	
				xEventGroupClearBits(ftp_event_group, FTP_ALL_DONE);
				filecnt=1;
			}
            else if(xEventGroupGetBits(ftp_event_group)&FTP_STOR_DONE)
		    {
		        if(filecnt==1)
			        ESP_LOGI(TAG, "File upload task started.");	
		        if(filecnt<wfile_num)
	            {
	                sprintf(srcname,"/sdcard/record/%02d_rec.pcm",filecnt);
			        sprintf(dstname,"%02d_rec.pcm",filecnt);
			        xEventGroupClearBits(ftp_event_group, FTP_DATALINK_OK);	
			        ftp_stor(&ftp_config);
			        ftp_start_STOR(&ftp_config);
			        xEventGroupWaitBits(ftp_event_group, FTP_DATALINK_OK, pdFALSE, pdFALSE, portMAX_DELAY);	
			        xEventGroupClearBits(ftp_event_group, FTP_STOR_CMD);
			        ftp_stor_ready(&ftp_config,dstname);
			        xEventGroupWaitBits(ftp_event_group, FTP_STOR_CMD, pdFALSE, pdFALSE, portMAX_DELAY);	
	                ftp_stor_begin(&ftp_config, srcname);
		            xEventGroupClearBits(ftp_event_group, FTP_STOR_DONE);			
                    filecnt++;
		       }
		       else  if(filecnt==wfile_num)
		       {

                   xEventGroupClearBits(ftp_event_group, FTP_CWD_DONE);
                   ftp_CWD(&ftp_config,dirname);
                   xEventGroupWaitBits(ftp_event_group, FTP_CWD_DONE, pdFALSE, pdFALSE, portMAX_DELAY);//目录转换成功  		    
	               sprintf(srcname,"/sdcard/record/rec.recconf");
			       sprintf(dstname,"rec.recconf");
			       xEventGroupClearBits(ftp_event_group, FTP_DATALINK_OK);	
			       ftp_stor(&ftp_config);
			       ftp_start_STOR(&ftp_config);
			       xEventGroupWaitBits(ftp_event_group, FTP_DATALINK_OK, pdFALSE, pdFALSE, portMAX_DELAY);	
			       xEventGroupClearBits(ftp_event_group, FTP_STOR_CMD);
			       ftp_stor_ready(&ftp_config,dstname);
			       xEventGroupWaitBits(ftp_event_group, FTP_STOR_CMD, pdFALSE, pdFALSE, portMAX_DELAY);	
	               ftp_stor_begin(&ftp_config, srcname);
		           xEventGroupClearBits(ftp_event_group, FTP_STOR_DONE);	


			       filecnt++;

		       }
		       else if(filecnt==(wfile_num+1))
		       {
	               sprintf(srcname,"/sdcard/record/ble.bleconf");
			       sprintf(dstname,"ble.bleconf");
			       xEventGroupClearBits(ftp_event_group, FTP_DATALINK_OK);	
			       ftp_stor(&ftp_config);
			       ftp_start_STOR(&ftp_config);
			       xEventGroupWaitBits(ftp_event_group, FTP_DATALINK_OK, pdFALSE, pdFALSE, portMAX_DELAY);	
			       xEventGroupClearBits(ftp_event_group, FTP_STOR_CMD);
			       ftp_stor_ready(&ftp_config,dstname);
			       xEventGroupWaitBits(ftp_event_group, FTP_STOR_CMD, pdFALSE, pdFALSE, portMAX_DELAY);	
	               ftp_stor_begin(&ftp_config, srcname);
		           xEventGroupClearBits(ftp_event_group, FTP_STOR_DONE);	

			       filecnt++;

		       }
		       else 
		       {
		    
			       ESP_LOGI(TAG, "all files upload .");	
	               xEventGroupClearBits(ftp_event_group, FTP_STOR_DONE);
	               xEventGroupClearBits(ftp_event_group, FTP_DATALINK_OK);
	               ftp_close(&ftp_config);				                
		       }
           }		  
//	    }
    }
	xEventGroupClearBits(ftp_event_group, FTP_STOR_DONE);
	xEventGroupClearBits(ftp_event_group, FTP_DATALINK_OK);
	ftp_close(&ftp_config);		
	xEventGroupWaitBits(ftp_event_group, FTP_ALL_DONE, pdFALSE, pdFALSE, portMAX_DELAY);
	ESP_LOGI(TAG, "esp_wifi will stop!");	
	esp_wifi_stop();	
	ESP_LOGI(TAG, "RAM left: %u", esp_get_free_heap_size());
	vEventGroupDelete(ftp_event_group);
	ESP_LOGI(TAG, "ftp_ul_task will be deleted!");
	vTaskDelete(NULL);
}

/*****************************************************************
  * OTA && FTP初始化处理,在这个函数中，对ftp和ota进行初始化操作，进行配置连接
 ****************************************************************/
esp_err_t ftp_ul_init()
{
    ESP_LOGI(TAG, "ftp_upload_init");
	ftp_init();
    ftp_event_group = xEventGroupCreate();
    xTaskCreate(ftp_ul_task, "ftp_upload_task", 4096, NULL, 11, NULL);
    return 0;
}


/*****************************************************************
  * OTA进程的处理，在处理的时候，处于最高优先级，不进行其他的操作
 ****************************************************************/
void ftp_ul_process(int filenum,char* uptm)
{

    char concnt=0;
//	esp_log_level_set("*", ESP_LOG_INFO);
	esp_log_level_set(TAG, ESP_LOG_INFO);
	
    ESP_LOGI(TAG, "Initialising ftpupload");
	sprintf(dirname,"/%013d#%s",get_device_id(),uptm);
	wfile_num=filenum+1;
	esp_wifi_start(); 
	while(wifi_sta_is_connected()==0)
	{
	    concnt++;
	    if(concnt>5)
	   	    break;
	    xEventGroupWaitBits(wifi_sta_get_event_group(), WIFI_STA_EVENT_GROUP_CONNECTED_FLAG, pdFALSE, pdFALSE, 5000/portTICK_PERIOD_MS);
		
	}
	if(wifi_sta_is_connected())
	{
	    ftp_config.control_state=FTP_CLOSED;
		ftp_connect(&ftp_config);        
		xEventGroupWaitBits(ftp_event_group, FTP_LOGEING_OK|FTP_ALL_DONE, pdFALSE, pdFALSE, portMAX_DELAY);//用户名密码发送成功，登录FTP服务器成功
        if(xEventGroupGetBits(ftp_event_group)&FTP_LOGEING_OK)
        {   
            xEventGroupClearBits(ftp_event_group,FTP_LOGEING_OK);
            ftp_MKD(&ftp_config, dirname);
	        xEventGroupWaitBits(ftp_event_group, FTP_MKD_DONE, pdFALSE, pdFALSE, portMAX_DELAY);//目录创建成功 
	        xEventGroupClearBits(ftp_event_group, FTP_CWD_DONE);
            ftp_CWD(&ftp_config,dirname);
            xEventGroupWaitBits(ftp_event_group, FTP_CWD_DONE, pdFALSE, pdFALSE, portMAX_DELAY);//目录转换成功 
            ftp_MKD(&ftp_config, "rec-file");
	        xEventGroupWaitBits(ftp_event_group, FTP_MKD_DONE, pdFALSE, pdFALSE, portMAX_DELAY);//目录创建成功 
	        xEventGroupClearBits(ftp_event_group, FTP_CWD_DONE);
            ftp_CWD(&ftp_config,"rec-file");
            xEventGroupWaitBits(ftp_event_group, FTP_CWD_DONE, pdFALSE, pdFALSE, portMAX_DELAY);//目录转换成功 
            
            xEventGroupSetBits(ftp_event_group, FTP_STOR_DONE);
    	}
	}
	else
	{
	    ESP_LOGI(TAG, "esp_wifi will stop!");	
	    xEventGroupSetBits(ftp_event_group, FTP_ALL_DONE);
	}
}
EventGroupHandle_t get_ftp_statusbit(void)
{
    return ftp_event_group;
}


