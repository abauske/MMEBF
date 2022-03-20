#ifndef _ESP_HID_GAP_H_
#define _ESP_HID_GAP_H_

#define HIDD_IDLE_MODE 0x00
#define HIDD_BLE_MODE 0x01

#define HID_DEV_MODE HIDD_BLE_MODE

#include "esp_err.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_hid_common.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_gap_ble_api.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t esp_hid_gap_init(uint8_t mode);

esp_err_t esp_hid_ble_gap_adv_init(uint16_t appearance, const char *device_name);
esp_err_t esp_hid_ble_gap_adv_start(void);

void print_uuid(esp_bt_uuid_t *uuid);
const char *ble_addr_type_str(esp_ble_addr_type_t ble_addr_type);

#ifdef __cplusplus
}
#endif

#endif /* _ESP_HIDH_GAP_H_ */
