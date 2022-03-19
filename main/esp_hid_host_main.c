/* This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this software is
   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <memory.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"

#include "led.h"
#include "can.h"
#include "ble_beacon.h"
#include "wheel_sensor.h"

static const char *TAG = "MAIN";

extern uint32_t currentSpeed_mph;
extern bool fastMode;

static void canHandler(twai_message_t* msg) {
  uint8_t data[12];
  memcpy(data, &msg->identifier, sizeof(msg->identifier));
  memcpy(data + sizeof(msg->identifier), msg->data, msg->data_length_code);
  switch(msg->identifier) {
//    case 0x201: // Laenge 5 Bytes nur!   Byte 0-1: geschwindigkeit: wert / 100 = km/h; Byte 4: 00 = keine unterstützung, 20=Stufe 1, 40=Stufe 2, 60=Stufe 3, 80=Stufe 4, 02=Licht an;rest konstant 0er
    case 0x203:
    case 0x300: // Laenge 4;   Byte 3: Fahrrad an: 5A, aus: AA; byte 0: 00=keine Unterstützung; 04=Vollgas; Byte 2: 00=Licht aus, 64=licht an
      return;
    // nach anschalten:
    case 0x200: // ändert sich fast gar nicht nur Byte 6 (das 7.) zyklisch von 0 bis 2; erstes Byte immer 100, andere immer 0
    case 0x665: // konstant: 00 00 00 53 4C 18 00 00
    case 0x202: // Byte 4-7: Laufleistung in metern! Little Endian ; Byte 0 immer 9A, Byte 2+3 immer 0; Byte 1 wechselt wild zwischen 12 bis 15
    case 0x666: // konstant: 52 06 01 00 52 01 38 52
      return;
  }
//      printf("CAN: Received message: %08X     %d    %02X %02X %02X %02X %02X %02X %02X %02X\n", msg->identifier, msg->data_length_code, data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);
//  bleSetData(data, msg->data_length_code + sizeof(msg->identifier));
  if(fastMode && currentSpeed_mph > 20000) {
    static twai_message_t out = {};
    out.identifier = 0x201;
    out.data_length_code = 5;
    uint16_t *speedLocation = (uint16_t *) out.data;
    *speedLocation = (uint16_t) (currentSpeed_mph / 10);
    canSend(&out);
  }
}

void app_main(void)
{
  ledInit(2);
  canInit(canHandler);
  wheelSensorInit();
//  bleInit();

//  while(1) {
//    vTaskDelay(100);
//    ESP_LOGI(TAG, "time since start: %lld us", esp_timer_get_time());
//  }
}
