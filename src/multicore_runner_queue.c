/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <intctrl.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "can2040.h"

typedef struct can2040 CANHandle;
typedef struct can2040_msg CANMsg;

static CANHandle can0;
static CANHandle can1;

static void can2040_cb0(CANHandle *cd, uint32_t notify, CANMsg *msg) {
  switch (notify) {
    case CAN2040_NOTIFY_RX:
      printf("can0 received a message. sending it back!");
      can2040_transmit(&can0, msg); // just send it back
      break;
    case CAN2040_NOTIFY_TX:
      printf("can0 successfully send message\n");
      break;
    case CAN2040_NOTIFY_ERROR:
      printf("can0 error\n");
      break;
  }
}

static void can2040_cb1(CANHandle *cd, uint32_t notify, CANMsg *msg) {
  switch (notify) {
    case CAN2040_NOTIFY_RX: {
      uint8_t* data = msg->data;
      printf("Received message: %08X     %d    %02X %02X %02X %02X %02X %02X %02X %02X\n", msg->id, msg->dlc, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
      break;
    }
    case CAN2040_NOTIFY_TX:
      printf("can1 successfully send message\n");
      break;
    case CAN2040_NOTIFY_ERROR:
      printf("can1 error\n");
      break;
  }
}

static void PIO0_IRQHandler(void) {
  can2040_pio_irq_handler(&can0);
  printf("PIO0_IRQHandler called\n");
}

static void PIO1_IRQHandler(void) {
  can2040_pio_irq_handler(&can1);
  printf("PIO1_IRQHandler called\n");
}

void canbus_setup(uint32_t pio_num, uint32_t gpio_rx, uint32_t gpio_tx) {
  uint32_t sys_clock = 125000000, bitrate = 500;

  CANHandle* canX = pio_num == 0 ? &can0 : &can1;

  // Setup canbus
  can2040_setup(&can0, pio_num);
  can2040_callback_config(&can0, pio_num == 0 ? can2040_cb0 : can2040_cb1);

  // Enable irqs
  uint irqn = pio_num == 0 ? PIO0_IRQ_0 : PIO1_IRQ_0;
  irq_set_exclusive_handler(irqn, pio_num == 0 ? PIO0_IRQHandler : PIO1_IRQHandler);
  irq_set_priority(irqn, 1);
  irq_set_enabled(irqn, true);

  // Start canbus
  can2040_start(&can0, sys_clock, bitrate, gpio_rx, gpio_tx);
}

int main() {
  stdio_init_all();

  sleep_ms(10000);
  printf("Startup delay over\n");

  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  uint ledStatus = 0;

  printf("Starting Initialization of CAN\n");
  canbus_setup(0, 4, 5);
  printf("Initialized CAN0\n");
//  canbus_setup(1, 2, 5);
  printf("Initialized CAN1\n");


  sleep_ms(1000);
//  gpio_set_pulls(2, 0, 0);


//  while (true) {
//    gpio_put(LED_PIN, 1);
//    sleep_ms(250);
//    gpio_put(LED_PIN, 0);
//    sleep_ms(250);
//  }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
  while (true) {
    CANMsg msg;
    msg.dlc = 8;
    msg.id = 34;
    msg.data[0] = 11;
    msg.data[1] = 22;
    msg.data[2] = 33;
    msg.data[3] = 44;
    msg.data[4] = 55;
    msg.data[5] = 66;
    msg.data[6] = 77;
    msg.data[7] = 88;
    int res = can2040_transmit(&can0, &msg);
    printf("Sending! returned: %d\n", res);
    sleep_ms(5000);
    gpio_put(LED_PIN, ledStatus);
    ledStatus = !ledStatus;
  }
#pragma clang diagnostic pop
}
