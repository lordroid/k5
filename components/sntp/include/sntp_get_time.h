#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "wifi_sta.h"

#include "lwip/err.h"
#include "apps/sntp/sntp.h"

void sntp_get_time(void);
void get_time_min(char* tminfo);
void get_time_sec(char* tminfo);



