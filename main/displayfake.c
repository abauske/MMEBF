//
// Created by adria on 19.03.2022.
//

#include <stdint-gcc.h>
#include <hal/twai_types.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "displayfake.h"
#include "can.h"
#include "powersaving.h"

static const char *WHEEL_TAG = "DISPLAYFAKE";

static uint64_t last300MessageTime = 0;
static uint64_t last300FakedTime = 0;
extern bool btConnected;

_Noreturn static void displayFakeTask() {
  static twai_message_t enableMsg = {0};
  while (1) {
    vTaskDelay(30 / portTICK_PERIOD_MS);

    int64_t now = esp_timer_get_time();
    if(now - last300MessageTime < 5000000) {
      // display seems to be alive -> don't do anything
      continue;
    }
    // display seems to be dead. Let's do what the display is supposed to do.

    enableMsg.identifier = 0x300;
    enableMsg.data_length_code = 4;
    uint8_t enabled = isPowerSafeEnabled() ? 0xAA : 0x5A;
    enableMsg.data[0] = btConnected ? 0x02 : 0x04; // set support
    enableMsg.data[1] = enabled;
    enableMsg.data[2] = isPowerSafeEnabled() ? 0x00 : 0x64; // turn light on
    enableMsg.data[3] = enabled;
    canSend(&enableMsg);
  }
}

void displayFakeInit() {
  xTaskCreate(displayFakeTask, "DisplayFake", 4096, NULL, 0, NULL);
}

void on300MessageReceived() {
  last300MessageTime = esp_timer_get_time();
}
