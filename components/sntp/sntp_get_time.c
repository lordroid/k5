/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sntp_get_time.h"


static const char *TAG = "sntp_server";

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */


static void obtain_time(void);
static void initialize_sntp(void);
void get_time_min(char* tminfo)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
	sprintf(tminfo,"%d%02d%02d%02d%02d",1900+timeinfo.tm_year,1+timeinfo.tm_mon,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min);
}
void get_time_sec(char* tminfo)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
	sprintf(tminfo,"%d%02d%02d%02d%02d%02d",1900+timeinfo.tm_year,1+timeinfo.tm_mon,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
}

void sntp_get_time()
{
//	esp_log_level_set("*", ESP_LOG_WARN);
//	esp_log_level_set(TAG, ESP_LOG_INFO);
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year < (2016 - 1900)) {
        obtain_time();
        time(&now);
    }
	sntp_stop();
    char strftime_buf[64];
	// Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
	get_time_min(strftime_buf);
	ESP_LOGI(TAG,"%s\r\n",strftime_buf);
	get_time_sec(strftime_buf);
	ESP_LOGI(TAG,"%s\r\n",strftime_buf);	
//    localtime_r(&now, &timeinfo);
//	ESP_LOGI(TAG, "The current date/time in Shanghai:year %d\r\nmonth %d\r\nday %d\r\nhour %d\r\nmin %d\r\nsec %d\r\n",1900+timeinfo.tm_year,1+timeinfo.tm_mon,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec );
   
}

static void obtain_time(void)
{
	xEventGroupWaitBits(wifi_sta_get_event_group(), WIFI_STA_EVENT_GROUP_CONNECTED_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
    initialize_sntp();
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
//        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

static void initialize_sntp(void)
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}


