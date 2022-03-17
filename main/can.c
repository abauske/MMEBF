//
// Created by adria on 17.03.2022.
//

#include "can.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TX_GPIO_NUM                     2
#define RX_GPIO_NUM                     3
static const char *CAN_TAG = "CAN";

static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_25KBITS();
//Set TX queue length to 0 due to listen only mode
static const twai_general_config_t g_config = {.mode = TWAI_MODE_LISTEN_ONLY,
    .tx_io = TX_GPIO_NUM, .rx_io = RX_GPIO_NUM,
    .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED,
    .tx_queue_len = 0, .rx_queue_len = 5,
    .alerts_enabled = TWAI_ALERT_RX_DATA,
    .clkout_divider = 0};

static CAN_MsgHandler msgHandler;

static void canReceiveTask(void *arg) {
  while (1) {
    ESP_LOGI(CAN_TAG, "Trying to receive next CAN message");
    twai_message_t rx_msg;
    esp_err_t err = twai_receive(&rx_msg, portMAX_DELAY);
    if(err == ESP_ERR_TIMEOUT) {
      continue;
    }
    if(err == ESP_OK) {
      msgHandler(&rx_msg);
    }
  }
}

void canInit(CAN_MsgHandler handler) {
  ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
  ESP_LOGI(CAN_TAG, "Driver installed");
  ESP_ERROR_CHECK(twai_start());
  ESP_LOGI(CAN_TAG, "Driver started");

  msgHandler = handler;

  xTaskCreate(canReceiveTask, "CAN_RX", 4096, NULL, 9, NULL);
}
