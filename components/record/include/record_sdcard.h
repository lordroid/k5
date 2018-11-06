/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/



/****************************************************************************
*
* This file is for iBeacon definitions. It supports both iBeacon sender and receiver
* which is distinguished by macros IBEACON_SENDER and IBEACON_RECEIVER,
*
* iBeacon is a trademark of Apple Inc. Before building devices which use iBeacon technology,
* visit https://developer.apple.com/ibeacon/ to obtain a license.
*
****************************************************************************/
#ifndef __RECORD_SDCARD_H__
#define __RECORD_SDCARD_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <sys/unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "ff.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "audio_hal.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "wav_encoder.h"
#include "esp_peripherals.h"
#include "periph_sdcard.h"
#include "wm8731drv.h"
#include "amrFileCodec.h"
#include "sntp_get_time.h"
#include "file_upload.h"
#define  REC_UP_DONE  1<<0

EventGroupHandle_t rec_event_group;

void  rec_event_task(void);
extern 
void restart_rec(void);

#ifdef __cplusplus
}
#endif

#endif


