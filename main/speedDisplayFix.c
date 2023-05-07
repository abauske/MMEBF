//
// Created by adria on 31.08.2022.
//

#include <stdint-gcc.h>
#include <hal/twai_types.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "displayfake.h"
#include "can.h"
#include "powersaving.h"

static char* SPEED_FIX_TAG = "SPEED_FIX";

static uint64_t lastSpeedMessageTime = 0;

extern uint32_t currentSpeed_mph;
extern bool motorHasRealSpeed;
extern uint64_t lastDoneEdgeTime;

static twai_message_t out = {};

void fillSpeedMsg() {
  out.identifier = 0x201;
  out.data_length_code = 5;
  uint16_t *speedLocation = (uint16_t *) out.data;
  *speedLocation = (uint16_t) (currentSpeed_mph / 10);
}

_Noreturn static void speedDisplayFixTask() {
  while (1) {
//    if(isPowerSafeEnabled()) {
//      vTaskDelay(4000 / portTICK_PERIOD_MS);
//      continue;
//    }

    int64_t now = esp_timer_get_time();
    if(!motorHasRealSpeed) {
      fillSpeedMsg();
      for(uint8_t i = 0; i < 60; i++) {
        canSend(&out); // send multiple times to make sure we hit the original message
//        ESP_LOGI(SPEED_FIX_TAG, "Time: %lld", now);
      }
    }
//    ESP_LOGI(SPEED_FIX_TAG, "Time: %lld", now);
//    vTaskDelay(1000 / portTICK_PERIOD_MS);

    int64_t delayMs = (((int64_t) lastSpeedMessageTime) + 98000 - now) / 1000;
    if(delayMs < 10) {
      delayMs += 100;
    }
    if(delayMs < 50) {
      delayMs = 50;
    }
    ESP_LOGI(SPEED_FIX_TAG, "Time: %lld, %lld", now, delayMs);
    vTaskDelay(delayMs / portTICK_PERIOD_MS);
  }
}

void speedDisplayFixInit() {
//  xTaskCreate(speedDisplayFixTask, "speedDisplayFix", 4096, NULL, 1, NULL);
}

void on201SpeedMsgReceived() {
  lastSpeedMessageTime = esp_timer_get_time();

//  if(!motorHasRealSpeed) {
//    fillSpeedMsg();
//    canSend(&out);
//  }
}