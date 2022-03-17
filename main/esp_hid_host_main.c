/* This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this software is
   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#if CONFIG_BT_BLE_ENABLED
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#endif
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "led_strip.h"

#define DEMO_TAG TAG

static const char *TAG = "HID_DEV_DEMO";


#define BLINK_GPIO 8

static uint8_t s_led_state = 0;

static led_strip_t *pStrip_a;

static void blink_led(void)
{
  /* If the addressable LED is enabled */
  if (s_led_state) {
    /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    pStrip_a->set_pixel(pStrip_a, 0, 16, 16, 16);
    /* Refresh the strip to send data */
    pStrip_a->refresh(pStrip_a, 100);
  } else {
    /* Set all LED off to clear all pixels */
    pStrip_a->clear(pStrip_a, 50);
  }
}

static void configure_led(void)
{
  ESP_LOGI(TAG, "Example configured to blink addressable LED!");
  /* LED strip initialization with the GPIO and pixels number*/
  pStrip_a = led_strip_init(CONFIG_BLINK_LED_RMT_CHANNEL, BLINK_GPIO, 1);
  /* Set all LED off to clear all pixels */
  pStrip_a->clear(pStrip_a, 50);
}

static uint8_t manu = 0x15;
static uint8_t data = 0x16;
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
        ESP_LOGE(DEMO_TAG, "Scan start failed: %s", esp_err_to_name(err));
      }
      break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      //adv start complete event to indicate adv start successfully or failed
      if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(DEMO_TAG, "Adv start failed: %s", esp_err_to_name(err));
      } else {
        ESP_LOGE(DEMO_TAG, "Adv started");
      }
      break;

    default:
      ESP_LOGI(DEMO_TAG, "Event unhandled");
      break;
  }
}

void app_main(void)
{

  /* Configure the peripheral according to the LED type */
  configure_led();

  for(int i = 0; i < 10; i++) {
    ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
    blink_led();
    /* Toggle the LED state */
    s_led_state = !s_led_state;
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }

  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);

  esp_bluedroid_init();
  esp_bluedroid_enable();

  esp_err_t status;

  ESP_LOGI(TAG, "register callback");

  //register the scan callback function to the gap module
  if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
    ESP_LOGE(TAG, "gap register error: %s", esp_err_to_name(status));
    return;
  }

  esp_ble_gap_set_device_name("MMEBF");
  esp_ble_gap_config_adv_data(&advData);


  static uint8_t iteration = 0;
  while (1) {
    ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
    blink_led();
    /* Toggle the LED state */
    s_led_state = !s_led_state;
    vTaskDelay(100 / portTICK_PERIOD_MS);
    for(int i = 0; i < sizeof(uuid); i++) {
      uuid[i] = iteration;
    }
    esp_ble_gap_config_adv_data(&advData);
    iteration++;
  }
}
