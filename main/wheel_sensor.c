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
#include "can.h"

#include "wheel_sensor.h"
#include "powersaving.h"

#define WHEEL_CIRCUMFENCE_MM 2149
#define uSEC_DELAY_x_KMH(x) (1000000 / (x * 1000 * 1000 / 60 / 60 / WHEEL_CIRCUMFENCE_MM))
#define OUTPUT_PIN GPIO_NUM_7
#define INPUT_PIN GPIO_NUM_19

static const char *WHEEL_TAG = "WHEEL";

extern bool btConnected;

static uint64_t lastEdgeTime = 0;
static uint32_t missingTicks = 0;
static uint32_t recapStartTicks = 0;
static bool newEdge = false;
static uint64_t newEdgeTime = 0;
static bool overrun = false;
uint64_t lastDoneEdgeTime = 0;
static uint32_t rotationCounter = 0;
static uint32_t sleepMs = 10;

uint32_t currentSpeed_mph = 0;
bool lastTickFastMode = false;
bool motorHasRealSpeed = true;

#define TICK_MODE_NORMAL 0
#define TICK_MODE_SLOW_RECAP 1
#define TICK_MODE_FAST 2
uint8_t tickMode = TICK_MODE_NORMAL;

uint32_t bootupMileage = 0;
uint32_t totalTickCount = 0;
uint64_t lastMilageUpdateTime = 0;
uint32_t lastTickMileage = 0;
extern uint32_t mileage;

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

static void setSpeedMph(uint32_t speedMph) {
  static twai_message_t out = {};

  currentSpeed_mph = speedMph;

  out.identifier = 0x737;
  out.data_length_code = 5;
  uint16_t *speedLocation = (uint16_t *) out.data;
  *speedLocation = (uint16_t) (currentSpeed_mph / 10);
  canSend(&out);
}

_Noreturn static void wheelSensorTask() {
  // Register isr here as this makes sure it runs on the same core so no synchronization should be required
  gpio_isr_register(negedgeHandler, NULL, ESP_INTR_FLAG_EDGE, NULL);
  
  while(1) {
    vTaskDelay(sleepMs / portTICK_PERIOD_MS); // about 100 Hz should be left
    int64_t now = esp_timer_get_time();
    uint64_t newTime = newEdgeTime;
    uint64_t timeSinceLastOutput = now - lastDoneEdgeTime;
    if(newEdge) {
      newEdge = false;
      missingTicks++;
      totalTickCount++;
      resumeFromPowerSaveMode();

      uint64_t delta = newTime - lastEdgeTime;
      // Real calculation is:
//      uint64_t freq = 1000000 / delta;
//      uint64_t speed_mmps = freq * WHEEL_CIRCUMFENCE_MM;
//      uint64_t speed_mmph = speed_mmps * 60 * 60;
//      uint64_t speed_mph = speed_mmph / 1000;
      // But we use this to prevent loss of precission:
      uint64_t speed_mph = 1000000 / 1000 * WHEEL_CIRCUMFENCE_MM * 60 / delta * 60;
      setSpeedMph(speed_mph);
      lastEdgeTime = newTime;
      ESP_LOGI(WHEEL_TAG, "Rotations done: %d, Speed: %lld", rotationCounter, speed_mph / 1000);
    } else if(now - lastEdgeTime > 2500 * 1000) {
      setSpeedMph(0);
      if(now - lastEdgeTime > 60 * 1000 * 1000 && missingTicks <= 0 && timeSinceLastOutput > 10000000) {
        enterPowerSaveMode();
      }
    }

    if(overrun) {
      ESP_LOGW(WHEEL_TAG, "Overrun detected!");
      setSpeedMph(66666);
    }

    if(mileage > 0 && bootupMileage == 0) {
      bootupMileage = mileage;
    } else if(lastTickMileage != mileage) {
      lastMilageUpdateTime = now;
      lastTickMileage = mileage;
    }
    bool fastMode = btConnected;

    if(missingTicks > 0) {

      if(!fastMode && lastTickFastMode) {
        // we have just exited fast mode -> recap ticks but keep real speed
        tickMode = TICK_MODE_SLOW_RECAP;
        recapStartTicks = missingTicks;
      } else if(fastMode) {
        tickMode = TICK_MODE_FAST;
      }

      bool standingRecap = currentSpeed_mph == 0 && timeSinceLastOutput >= uSEC_DELAY_x_KMH(120) && missingTicks > 10;
      bool doTick = true;
      switch (tickMode) {
        case TICK_MODE_NORMAL:
          doTick = true;
          break;
        case TICK_MODE_SLOW_RECAP:
          if(missingTicks > recapStartTicks) {
            // we are driving faster than recapping -> forward tick and set new upper limit
            doTick = true;
            recapStartTicks = missingTicks - 1;
          } else if(timeSinceLastOutput >= uSEC_DELAY_x_KMH(25) || standingRecap) {
            doTick = true;
            // missing ticks will be decreased by 1 so recapStartTicks is one more to accomodate for a potential later
            // coming next tick, that would cause double ticks -> would cancel motor support from 10 km/h onwards
            recapStartTicks = missingTicks;
          } else {
            doTick = false; // no new tick came in and time for recap is not over
          }
          break;
        case TICK_MODE_FAST:
          doTick = timeSinceLastOutput >= uSEC_DELAY_x_KMH(20)    // we are driving -> tick slowly
              || standingRecap;                                   // we are standing -> tick fast
          break;

        default: // should never be used
          doTick = true;
          break;
      }

      if(doTick) {
        // Forward the signal to the motor
        gpio_set_level(OUTPUT_PIN, 1);
        missingTicks--;
        lastDoneEdgeTime = now;
        vTaskDelay(1);
        gpio_set_level(OUTPUT_PIN, 0);
      }
      motorHasRealSpeed = missingTicks == 0;
    } else if(mileage < bootupMileage + totalTickCount * WHEEL_CIRCUMFENCE_MM / 1000 && now - lastMilageUpdateTime < 10000000) {
      // get to the end result slowly -> only add 4/5 of what we expect we need to add
      missingTicks += (totalTickCount - (mileage - bootupMileage) * 1000 / WHEEL_CIRCUMFENCE_MM) * 4 / 5 + 1;
    } else {
      tickMode = TICK_MODE_NORMAL; // everything recapped -> go to normal
    }
    lastTickFastMode = fastMode;
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
