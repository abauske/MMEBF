/* This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this software is
   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <memory.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include <inttypes.h>

#include "led.h"
#include "can.h"
#include "ble_beacon.h"
#include "wheel_sensor.h"
#include "bt_con.h"
#include "powersaving.h"
#include "displayfake.h"
#include "speedDisplayFix.h"

static const char *TAG = "MAIN";

extern uint32_t currentSpeed_mph;
extern bool fastMode;
extern bool motorHasRealSpeed;
extern uint64_t lastDoneEdgeTime;
uint32_t mileage;

static void canHandler(twai_message_t* msg) {
  uint8_t data[8];
  uint32_t id = msg->identifier;
  memcpy(data, msg->data, msg->data_length_code);
  switch(msg->identifier) {
    case 0x201: // Laenge 5 Bytes nur!   Byte 0-1: geschwindigkeit: wert / 100 = km/h; Byte 4: 00 = keine unterstützung, 20=Stufe 1, 40=Stufe 2, 60=Stufe 3, 80=Stufe 4, 02=Licht an;rest konstant 0er
//      on201SpeedMsgReceived();
      return;
    case 0x203: // kommt von Motor! Auswertungstag 1: konstant: 00 00 00 00 00 00 72 00; Auswertungstag 2: konstant 00 00 00 00 00 00 FF 0F
      return;
    case 0x300: // kommt von Display! Laenge 4;   Byte 3: Fahrrad an: 5A, aus: AA; byte 0: 00=keine Unterstützung; 04=Vollgas; Byte 2: 00=Licht aus, 64=licht an
                // Beispiel: 00 5A 00 5A
//      printf("CAN: Received message: %08X     %d    %02X %02X %02X %02X %02X %02X %02X %02X\n", msg->identifier, msg->data_length_code, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
      on300MessageReceived();
      return;
    // nach anschalten:
    case 0x200: // kommt von Motor! Analysetag 1: ändert sich fast gar nicht nur Byte 6 (das 7.) zyklisch von 0 bis 2; erstes Byte immer 100, andere immer 0
                // Analysetag 2: 45 00 1C B0 06 00 01 80 bei 4 von 5 Balken Akkuladung; Byte 6 wieder zyklisch von 0 bis 2; letztes Byte war einmal 0x80 sonst immer 0
    case 0x2FA: // erst mit neuem Display 03 80 7C E1 0B 00 E7 F8 ersten 4 konstant . Byte 4 (das 5.): 0B -> B konst, 0 zählt in 2er schritten hoch; Byte 5-7: verändern sich, evtl Zeit?
    case 0x665: // kommt von Motor! konstant: 00 00 00 53 4C 18 00 00
    case 0x666: // kommt von Motor! konstant: 52 06 01 00 52 01 38 52
//      printf("CAN: Received message: %08X     %d    %02X %02X %02X %02X %02X %02X %02X %02X\n", msg->identifier, msg->data_length_code, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
      return;
    case 0x3F0: // Received message: 000003F0     8    F1 BE C5 2E F8 11 AC AC // Received message: 000003F0     8    F1 BE C5 2E 18 18 9D 0C // Received message: 000003F0     8    F1 BE C5 2E B8 1D 8F 6C
    case 0x400: // Received message: 00000400     4    01 80 0C 00
//      printf("CAN: Received message: %08X     %d    %02X %02X %02X %02X %02X %02X %02X %02X\n", msg->identifier, msg->data_length_code, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
      return;
    case 0x402: // Received message: 00000402     8    45 00 00 00 5C B2 06 00
    case 0x403: // Received message: 00000403     8    60 00 00 00 EC AF 09 00
    case 0x401: // Received message: 00000401     8    D5 98 00 00 AC FF FF FF
    case 0x404: // Received message: 00000404     8    46 0F 4B 0F 49 0F 4A 0F
    case 0x405: // Received message: 00000405     8    4A 0F 46 0F 48 0F 49 0F
    case 0x406: // Received message: 00000406     8    4A 0F 47 0F 00 00 0A 00
//      printf("CAN: Received message: %08X     %d    %02X %02X %02X %02X %02X %02X %02X %02X\n", msg->identifier, msg->data_length_code, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
      return;
      // einmalig in dieser Reihenfolge beim Starten: (nach einer 0x3F0 (Received message: 000003F0     8    F1 BE C5 2E 18 04 C9 0C))
    case 0x3F1: // Received message: 000003F1     6    01 00 00 00 00 05 00 00 // einmalig in dieser Reihenfolge beim Starten
    case 0x3F2: // Received message: 000003F2     7    00 01 00 00 00 00 00 00 // einmalig in dieser Reihenfolge beim Starten
    case 0x3F3: // Received message: 000003F3     8    CC 8D 60 1B 13 7C ED 00 // einmalig in dieser Reihenfolge beim Starten
    case 0x3F4: // Received message: 000003F4     8    00 00 00 00 00 00 00 00 // einmalig in dieser Reihenfolge beim Starten
    case 0x3F6: // Received message: 000003F6     5    45 23 78 05 00 76 00 00 // einmalig in dieser Reihenfolge beim Starten
//      printf("CAN: Received message: %08X     %d    %02X %02X %02X %02X %02X %02X %02X %02X\n", msg->identifier, msg->data_length_code, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
      return;

    case 0x202: { // Byte 4-7: Laufleistung in metern! Little Endian ; Byte 0 immer 9A, Byte 2+3 immer 0; Byte 1 wechselt wild zwischen 12 bis 15
      uint32_t *mileagePointer = (uint32_t *) &data[4];
      mileage = *mileagePointer;
      return;
    }
    case 0x737: {
      printf("ERROR: VERY MUCH ATTENTION!!! received CAN message on realSpeedMsgId: Received message: %08X     %d    %02X %02X %02X %02X %02X %02X %02X %02X\n", msg->identifier, msg->data_length_code, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
      return;
    }
    default:
      printf("Attention!!! received unknown CAN message: Received message: %08X     %d    %02X %02X %02X %02X %02X %02X %02X %02X\n", msg->identifier, msg->data_length_code, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
      return;

  }
//      printf("CAN: Received message: %08X     %d    %02X %02X %02X %02X %02X %02X %02X %02X\n", msg->identifier, msg->data_length_code, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
//  bleSetData(data, msg->data_length_code + sizeof(msg->identifier));
//  int64_t now = esp_timer_get_time();
//  printf("Time since last wheel tick: %lld\n", (now - lastDoneEdgeTime)/1000);
//  if(!motorHasRealSpeed) {
//    static twai_message_t out = {};
//    out.identifier = 0x201;
//    out.data_length_code = 5;
//    uint16_t *speedLocation = (uint16_t *) out.data;
//    *speedLocation = (uint16_t) (currentSpeed_mph / 10);
//    canSend(&out);
//  }
}

void app_main(void)
{
  powerSavingInit();
//  ledInit(2);
  canInit(canHandler);
  wheelSensorInit();
//  speedDisplayFixInit();
  btInit();
  displayFakeInit();
//  bleInit();

//  while(1) {
//    vTaskDelay(100);
//    ESP_LOGI(TAG, "time since start: %lld us", esp_timer_get_time());
//  }
}
