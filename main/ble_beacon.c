//
// Created by adria on 17.03.2022.
//

#include <memory.h>
#include "ble_beacon.h"

#include "esp_gap_ble_api.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"

static const char *BLE_TAG = "BLE Beacon";

static uint8_t uuid[16] = {0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17};

esp_ble_adv_data_t advData = {
    .set_scan_rsp = false,           /*!< Set this advertising data as scan response or not*/
    .include_name = false,           /*!< Advertising data include device name or not */
    .include_txpower = false,        /*!< Advertising data include TX power */
    .min_interval = 0x0006,           /*!< Advertising data show slave preferred connection min interval.
                             The connection interval in the following manner:
                             connIntervalmin = Conn_Interval_Min * 1.25 ms
                             Conn_Interval_Min range: 0x0006 to 0x0C80
                             Value of 0xFFFF indicates no specific minimum.
                             Values not defined above are reserved for future use.*/

    .max_interval = 0x0006,           /*!< Advertising data show slave preferred connection max interval.
                             The connection interval in the following manner:
                             connIntervalmax = Conn_Interval_Max * 1.25 ms
                             Conn_Interval_Max range: 0x0006 to 0x0C80
                             Conn_Interval_Max shall be equal to or greater than the Conn_Interval_Min.
                             Value of 0xFFFF indicates no specific maximum.
                             Values not defined above are reserved for future use.*/

    .appearance = 0,             /*!< External appearance of device */
    .manufacturer_len = 0,       /*!< Manufacturer data length */
    .service_data_len = 0,       /*!< Service data length */
    .service_uuid_len = sizeof(uuid),       /*!< Service uuid length */
    .p_service_uuid = (uint8_t *) &uuid,        /*!< Service uuid array point */
    .flag = ESP_BLE_ADV_FLAG_NON_LIMIT_DISC,                   /*!< Advertising flag of discovery mode, see BLE_ADV_DATA_FLAG detail */
};


esp_ble_adv_params_t advParams = {
    .adv_int_min = 0x0020,        /*!< Minimum advertising interval for
                           undirected and low duty cycle directed advertising.
                           Range: 0x0020 to 0x4000 Default: N = 0x0800 (1.28 second)
                           Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec */
    .adv_int_max = 0x0020,        /*!< Maximum advertising interval for
                           undirected and low duty cycle directed advertising.
                           Range: 0x0020 to 0x4000 Default: N = 0x0800 (1.28 second)
                           Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec Advertising max interval */
    .adv_type = ADV_TYPE_NONCONN_IND,           /*!< Advertising type */
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,      /*!< Owner bluetooth device address type */
    .channel_map = ADV_CHNL_ALL,        /*!< Advertising channel map */
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY  /*!< Advertising filter policy */
};

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  esp_err_t err;

  switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:{
      static bool started = false;
      if(!started) {
        esp_ble_gap_start_advertising(&advParams);
        started = true;
      }
      break;
    }
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
      break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
      //scan start complete event to indicate scan start successfully or failed
      if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(BLE_TAG, "Scan start failed: %s", esp_err_to_name(err));
      }
      break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      //adv start complete event to indicate adv start successfully or failed
      if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(BLE_TAG, "Adv start failed: %s", esp_err_to_name(err));
      } else {
        ESP_LOGE(BLE_TAG, "Adv started");
      }
      break;

    default:
      ESP_LOGI(BLE_TAG, "Event unhandled");
      break;
  }
}

void bleInit() {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);

  esp_bluedroid_init();
  esp_bluedroid_enable();

  esp_err_t status;

  ESP_LOGI(BLE_TAG, "register callback");

  //register the scan callback function to the gap module
  if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
    ESP_LOGE(BLE_TAG, "gap register error: %s", esp_err_to_name(status));
    return;
  }

  esp_ble_gap_set_device_name("MMEBF");
  esp_ble_gap_config_adv_data(&advData);
}

void bleSetData(uint8_t *data, uint8_t len) {
  memcpy(uuid, data, len < sizeof(uuid) ? len : sizeof(uuid));
  esp_ble_gap_config_adv_data(&advData);
}
