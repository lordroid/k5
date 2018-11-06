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
#ifndef __APP_BLE_H__
#define __APP_BLE_H__

#ifdef __cplusplus
	extern "C" {
#endif

/* Because current ESP IDF version doesn't support scan and adv simultaneously,
 * so iBeacon sender and receiver should not run simultaneously */
#define IBEACON_SENDER      0
#define IBEACON_RECEIVER    1


/* Major and Minor part are stored in big endian mode in iBeacon packet,
 * need to use this macro to transfer while creating or processing
 * iBeacon data */
#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00)>>8) + (((x)&0xFF)<<8))
#define max_recv_ble_num  100
#define max_recv_rssi_num 100
#define DEVICE_NAME "SmartK5"

#define ble_rssi_upload_interval 10//sec

typedef struct {
    uint8_t flags[3];
    uint8_t length;
    uint8_t type;
    uint16_t company_id;
    uint16_t beacon_type;
}__attribute__((packed)) esp_ble_ibeacon_head_t;

typedef struct {
    uint8_t proximity_uuid[16];
    uint16_t major;
    uint16_t minor;
	int8_t measured_power;
}__attribute__((packed)) esp_ble_ibeacon_vendor_t;


typedef struct {
    esp_ble_ibeacon_head_t ibeacon_head;
    esp_ble_ibeacon_vendor_t ibeacon_vendor;
}__attribute__((packed)) esp_ble_ibeacon_t;


/* For iBeacon packet format, please refer to Apple "Proximity Beacon Specification" doc */
/* Constant part of iBeacon data */
extern esp_ble_ibeacon_head_t ibeacon_common_head;

void app_ble_init(void);
void app_ble_close(void);
void app_ble_open(void);
void get_ble_num(char *tmpbuf);
/*
set_major:	IBEACON_MAJOR 
set_minor:	IBEACON_MINOR
uuid_cmd:   uuid[15]
*/
void set_ibeacon_data(uint16_t set_major,uint16_t set_minor,uint16_t uuid_cmd);

/*
set_ibeacon_mode:	IBEACON_SENDER 
				    IBEACON_RECEIVER
*/
void set_ble_mode(uint8_t set_ibeacon_mode);

uint32_t get_current_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif


