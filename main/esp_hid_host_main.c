/* This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this software is
   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <memory.h>
#include "esp_log.h"

#include "led.h"
#include "can.h"
#include "ble_beacon.h"

static const char *TAG = "MAIN";

static void canHandler(twai_message_t* msg) {
  uint8_t data[12];
  memcpy(data, &msg->identifier, sizeof(msg->identifier));
  memcpy(data + sizeof(msg->identifier), msg->data, msg->data_length_code);
  printf("CAN: Received message: %08X         %02X %02X %02X %02X %02X %02X %02X %02X", msg->identifier, data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);
  bleSetData(data, msg->data_length_code + sizeof(msg->identifier));
}

void app_main(void)
{
  ledInit(2);
//  canInit(canHandler);
  bleInit();
}
