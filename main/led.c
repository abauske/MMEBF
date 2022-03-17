//
// Created by adria on 17.03.2022.
//

#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "led.h"
#include "led_strip.h"
#include "esp_log.h"

static const char *LED_TAG = "LED";

#define BLINK_GPIO 8

static uint8_t s_led_state = 0;
static uint32_t frequency;

static led_strip_t *pStrip_a;

static void blink_led(void)
{
  /* If the addressable LED is enabled */
  if (s_led_state) {
    /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    pStrip_a->set_pixel(pStrip_a, 0, 2, 2, 2);
    /* Refresh the strip to send data */
    pStrip_a->refresh(pStrip_a, 100);
  } else {
    /* Set all LED off to clear all pixels */
    pStrip_a->clear(pStrip_a, 50);
  }
}

static void configure_led(void)
{
  ESP_LOGI(LED_TAG, "Example configured to blink addressable LED!");
  /* LED strip initialization with the GPIO and pixels number*/
  pStrip_a = led_strip_init(CONFIG_BLINK_LED_RMT_CHANNEL, BLINK_GPIO, 1);
  /* Set all LED off to clear all pixels */
  pStrip_a->clear(pStrip_a, 50);
}

static void ledTask() {
  while (1) {
//    ESP_LOGI(LED_TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
    blink_led();
    /* Toggle the LED state */
    s_led_state = !s_led_state;
    vTaskDelay(1000 / portTICK_PERIOD_MS / 2 / frequency);
  }
}

void ledInit(uint32_t freq) {
  frequency = freq;
  /* Configure the peripheral according to the LED type */
  configure_led();

  for(int i = 0; i < 10; i++) {
    ESP_LOGI(LED_TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
    blink_led();
    /* Toggle the LED state */
    s_led_state = !s_led_state;
    vTaskDelay(1000 / portTICK_PERIOD_MS / 2 / 5); // 5 Hz
  }

  xTaskCreate(ledTask, "LED", 4096, NULL, 1, NULL);
}