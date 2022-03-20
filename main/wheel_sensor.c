#include <sys/cdefs.h>
//
// Created by adria on 19.03.2022.
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <hal/gpio_types.h>
#include <driver/gpio.h>
#include <hal/gpio_hal.h>
#include "esp_log.h"

#include "wheel_sensor.h"
#include "powersaving.h"

#define WHEEL_CIRCUMFENCE_MM 2149
#define uSEC_DELAY_x_KMH(x) (1000000 / (x * 1000 * 1000 / 60 / 60 / WHEEL_CIRCUMFENCE_MM))
#define OUTPUT_PIN GPIO_NUM_7
#define INPUT_PIN GPIO_NUM_19

static const char *WHEEL_TAG = "WHEEL";

static uint64_t lastEdgeTime = 0;
static uint32_t missingTicks = 0;
static bool newEdge = false;
static uint64_t newEdgeTime = 0;
static bool overrun = false;
static uint64_t lastDoneEdgeTime = 0;
static uint32_t rotationCounter = 0;
static uint32_t sleepMs = 10;

uint32_t currentSpeed_mph = 0;
bool fastMode = false;

void enterPowerSave() {
  sleepMs = 500;
}

void exitPowerSave() {
  sleepMs = 10;
}

static void negedgeHandler() {
  uint32_t gpio_intr_status = GPIO.status.val;
  GPIO.status_w1tc.val = gpio_intr_status;

  static uint64_t lastInterruptTime = 0;
  
  uint64_t now = esp_timer_get_time();
  bool tooFast = now - lastInterruptTime < 1000000 / 20; // allow max 20 rising edges per second -> more than 100 km/h
  lastInterruptTime = now;
  if(tooFast) {
    return; // ignore this edge this is probably bounce
  }
  newEdgeTime = now;
  if(newEdge) {
    overrun = true;
  }
  newEdge = true;
  rotationCounter++;
}

_Noreturn static void wheelSensorTask() {
  // Register isr here as this makes sure it runs on the same core so no synchronization should be required
  gpio_isr_register(negedgeHandler, NULL, ESP_INTR_FLAG_EDGE, NULL);
  
  while(1) {
    vTaskDelay(sleepMs / portTICK_PERIOD_MS); // about 100 Hz should be left
    int64_t now = esp_timer_get_time();
    uint64_t newTime = newEdgeTime;
    if(newEdge) {
      newEdge = false;
      missingTicks++;
      resumeFromPowerSaveMode();

      uint64_t delta = newTime - lastEdgeTime;
      // Real calculation is:
//      uint64_t freq = 1000000 / delta;
//      uint64_t speed_mmps = freq * WHEEL_CIRCUMFENCE_MM;
//      uint64_t speed_mmph = speed_mmps * 60 * 60;
//      uint64_t speed_mph = speed_mmph / 1000;
      // But we use this to prevent loss of precission:
      uint64_t speed_mph = 1000000 / 1000 * WHEEL_CIRCUMFENCE_MM * 60 / delta * 60;
      currentSpeed_mph = speed_mph;
      lastEdgeTime = newTime;
      ESP_LOGI(WHEEL_TAG, "Rotations done: %d, Speed: %lld", rotationCounter, speed_mph / 1000);
    } else if(now - lastEdgeTime > 1500 * 1000) {
      currentSpeed_mph = 0;
      if(now - lastEdgeTime > 60 * 1000 * 1000 && missingTicks <= 0) {
        enterPowerSaveMode();
      }
    }

    if(overrun) {
      ESP_LOGW(WHEEL_TAG, "Overrun detected!");
      currentSpeed_mph = 77777;
    }

    if(missingTicks > 0) {
      uint64_t timeSinceLastOutput = now - lastDoneEdgeTime;
      bool recapAtMaxSpeed = (!fastMode || currentSpeed_mph == 0) && timeSinceLastOutput >= uSEC_DELAY_x_KMH(60);
      if(recapAtMaxSpeed || timeSinceLastOutput >= uSEC_DELAY_x_KMH(20)) {
        // Forward the signal to the motor
        gpio_set_level(OUTPUT_PIN, 1);
        missingTicks--;
        lastDoneEdgeTime = now;
        vTaskDelay(1);
        gpio_set_level(OUTPUT_PIN, 0);
      }
    }
  }
}

void wheelSensorInit() {
  
  // Configure output for faking speed signal
  gpio_config_t conf = {};
  conf.intr_type = GPIO_INTR_DISABLE;
  conf.mode = GPIO_MODE_OUTPUT;
  conf.pin_bit_mask = 1ULL << OUTPUT_PIN;
  conf.pull_down_en = 0;
  conf.pull_up_en = 0;
  gpio_config(&conf);
  
  // Configure wheel speed sensor input
  conf.intr_type = GPIO_INTR_NEGEDGE;
  conf.pin_bit_mask = 1ULL << INPUT_PIN;
  conf.mode = GPIO_MODE_INPUT;
  conf.pull_up_en = 1;
  conf.pull_down_en = 0;
  gpio_config(&conf);

  registerPowerSavingActions(enterPowerSave, exitPowerSave);
  
  xTaskCreatePinnedToCore(wheelSensorTask, "WheelSensor", 4096, NULL, 2, NULL, 0);
}
