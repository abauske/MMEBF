#include <sys/cdefs.h>
//
// Created by adria on 17.03.2022.
//

#include "can.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "powersaving.h"

#define TX_GPIO_NUM                     10
#define RX_GPIO_NUM                     1
static const char *CAN_TAG = "CAN";

static const twai_filter_config_t f_config = {.acceptance_code = 0, .acceptance_mask = 0xFFFFFFFF, .single_filter = true};
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
//Set TX queue length to 0 due to listen only mode
static const twai_general_config_t g_config = {.mode = TWAI_MODE_NORMAL,
    .tx_io = TX_GPIO_NUM, .rx_io = RX_GPIO_NUM,
    .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED,
    .tx_queue_len = 1, .rx_queue_len = 5,
    .alerts_enabled = TWAI_ALERT_RX_DATA,
    .clkout_divider = 0};

static CAN_MsgHandler msgHandler;

static void disable() {
  ESP_ERROR_CHECK(twai_stop());
  ESP_ERROR_CHECK(twai_driver_uninstall());
}

static void enable() {
  ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
  ESP_ERROR_CHECK(twai_start());
}

_Noreturn static void canReceiveTask(void *arg) {
  while (1) {
//    ESP_LOGI(CAN_TAG, "Trying to receive next CAN message");
    twai_message_t rx_msg;
    esp_err_t err = twai_receive(&rx_msg, portMAX_DELAY);
    if(err == ESP_ERR_TIMEOUT) {
      continue;
    } else if(err == ESP_ERR_INVALID_STATE) {
      vTaskDelay(1000 / portTICK_PERIOD_MS); // sleep more in power save mode
    }
    if(err == ESP_OK) {
      msgHandler(&rx_msg);
    }
  }
}

void canInit(CAN_MsgHandler handler) {
  enable();
  registerPowerSavingActions(disable, enable);

  msgHandler = handler;

  xTaskCreate(canReceiveTask, "CAN_RX", 4096, NULL, 9, NULL);
}

void canSend(const twai_message_t *msg) {
  twai_transmit(msg, 1);
}
