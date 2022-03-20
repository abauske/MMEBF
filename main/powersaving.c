//
// Created by adria on 19.03.2022.
//

#include <stdint-gcc.h>
#include <assert.h>
#include <stdbool.h>
#include <esp_pm.h>
#include "powersaving.h"

#define ACTION_COUNT 10

static powerSaveAction enterActions[ACTION_COUNT];
static powerSaveAction exitActions[ACTION_COUNT];
static uint32_t actionCount = 0;
static bool powerSaveEnabled = false;
static esp_pm_lock_handle_t lockHandle;

void powerSavingInit() {
  esp_pm_config_esp32c3_t config = { .max_freq_mhz = 160, .min_freq_mhz = 10, .light_sleep_enable = false};
  esp_pm_configure(&config);
  esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "main", &lockHandle);
  esp_pm_lock_acquire(lockHandle);
}

void enterPowerSaveMode() {
  if(powerSaveEnabled) {
    return; // already in powerSaveMode
  }
  for(uint32_t i = 0; i < actionCount; i++) {
    enterActions[i]();
  }
  powerSaveEnabled = true;
  esp_pm_lock_release(lockHandle);
}

void resumeFromPowerSaveMode() {
  if(!powerSaveEnabled) {
    return; // power save mode already disabled
  }
  esp_pm_lock_acquire(lockHandle);
  for(uint32_t i = 0; i < actionCount; i++) {
    exitActions[i]();
  }
  powerSaveEnabled = false;
}

void registerPowerSavingActions(powerSaveAction enterPowerSaveAction, powerSaveAction exitPowerSaveAction) {
  assert(actionCount < ACTION_COUNT);
  enterActions[actionCount] = enterPowerSaveAction;
  exitActions[actionCount] = exitPowerSaveAction;
  actionCount++;
}
