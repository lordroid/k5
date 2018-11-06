/* Record WAV file to SD Card

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "record_sdcard.h"
#include "uart_manage.h"
#include "app_ble.h"
#include "esp_gap_ble_api.h"

static const char *TAG = "REC_SDCARD";

#define RECORD_TIME_SECONDS (30)
static FILE* frec;
static int filenamecnt=1,mincnt=0,rec_start=0;
static char recfilename[30];
static char blebuf[10],sttmbuf[20],fntmbuf[20];
static char filebuf[100];
static audio_pipeline_handle_t pipeline_event;
static audio_element_handle_t fatfs_event, i2s_event;

static void creat_rec_event(void)
{
	audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	pipeline_event = audio_pipeline_init(&pipeline_cfg);
	mem_assert(pipeline);
	
	fatfs_stream_cfg_t fatfs_cfg=FATFS_STREAM_CFG_DEFAULT();
	fatfs_cfg.type = AUDIO_STREAM_WRITER;
	fatfs_cfg.task_prio = 20;
	fatfs_event = fatfs_stream_init(&fatfs_cfg);
	
	i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
	i2s_cfg.i2s_config.sample_rate=8000;
	i2s_cfg.i2s_config.dma_buf_len = 160;
	i2s_cfg.type = AUDIO_STREAM_READER;
	i2s_event = i2s_stream_init(&i2s_cfg);
	
	audio_pipeline_register(pipeline_event, i2s_event, "i2s");
	
	audio_pipeline_register(pipeline_event, fatfs_event, "file"); 

}
static void rec_pipe_start(void)
{
	char recfilename[30];
	sprintf(recfilename,"/sdcard/record/%02d_rec.pcm",filenamecnt);
	audio_pipeline_link(pipeline_event, (const char *[]) {"i2s", "file"}, 2);
	audio_element_set_uri(fatfs_event, recfilename);
	audio_pipeline_run(pipeline_event);

}
static void rec_ready(void)
{

	app_ble_open();
	esp_ble_gap_start_scanning(0);	 
	frec=fopen("/sdcard/record/rec.recconf","w+");
	get_time_sec(sttmbuf);
	rec_pipe_start();

}
void rec_event_task(void)
{
	esp_log_level_set("*", ESP_LOG_WARN);
	esp_log_level_set(TAG, ESP_LOG_INFO);		
	rec_event_group = xEventGroupCreate();
	creat_rec_event();	
	int second_recorded = 0;

    while (1) {
            if(rec_start==0)
                xEventGroupWaitBits(get_uart_cmdbit(),GD_REC_START,pdFALSE, pdFALSE, portMAX_DELAY);
            else 
				xEventGroupWaitBits(get_uart_cmdbit(),GD_REC_END,pdFALSE, pdFALSE, 1000/portTICK_RATE_MS);
			if(xEventGroupGetBits(get_uart_cmdbit())&GD_REC_START)
			{
			    if(rec_start==0)
			    { 
			        clear_uart_cmdbit( GD_REC_START);
			        second_recorded = 0;
			        filenamecnt=1;
				    rec_start=1;
				    rec_ready();
			    }
			}
			else if(xEventGroupGetBits(get_uart_cmdbit())&GD_REC_END)
			{
                fclose(frec);
				app_ble_close();					
				get_time_sec(fntmbuf);//上传时间					
				ftp_ul_process(6,fntmbuf);
				xEventGroupWaitBits(get_ftp_statusbit(), FTP_ALL_DONE, pdFALSE, pdFALSE, portMAX_DELAY);
				rec_start=0;
				clear_uart_cmdbit( GD_REC_END);
				set_uart_cmdbit(GD_REC_START);
				ESP_LOGI(TAG, "return from ftp_up_process!");
				ESP_LOGI(TAG, "RAM left: %u", esp_get_free_heap_size());                
			}
			else
            {
            second_recorded ++;
			if((second_recorded%60)==0)
				mincnt++;
            if (second_recorded >= RECORD_TIME_SECONDS) {
                second_recorded=0;	
				
			    get_ble_num(blebuf);
			    get_time_sec(fntmbuf);				
				sprintf(filebuf,"%02d-%s-%s-%s\r\n",filenamecnt,blebuf,sttmbuf,fntmbuf);
				fwrite(filebuf,1,strlen(filebuf),frec);
				filenamecnt++;	
			    audio_pipeline_stop(pipeline_event);
			    audio_pipeline_wait_for_stop(pipeline_event);
				ESP_LOGE(TAG, "one file has been written!");				
				if(filenamecnt>6)
				{
				    set_uart_cmdbit(GD_REC_END);
				}
                else
				{
				    ESP_LOGI(TAG, "RAM left: %u", esp_get_free_heap_size());
				    get_time_sec(sttmbuf);
                    rec_pipe_start();
                }
			}
			}


    }
	

	ESP_LOGI(TAG, "RAM left: %u", esp_get_free_heap_size());
    audio_pipeline_terminate(pipeline_event);

    audio_pipeline_deinit(pipeline_event);
    audio_element_deinit(fatfs_event);
    audio_element_deinit(i2s_event);    
    vEventGroupDelete(rec_event_group);
//	ESP_LOGI(TAG, "RAM left: %u", esp_get_free_heap_size());	
    xTaskCreate((void*)restart_rec, "restart_rec_task", 2048, NULL, 9, NULL);
	ESP_LOGI(TAG, "record task will be deleted!");
	vTaskDelete(NULL);
}

