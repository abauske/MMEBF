/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <intctrl.h>
#include <hardware/watchdog.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "can2040.h"

#define CAN_TIMEOUT_US 500000
#define REAL_SPEED_TIMEOUT_MS 10000

typedef struct can2040 CANHandle;
typedef struct can2040_msg CANMsg;

static CANHandle can0;
static CANHandle can1;

static absolute_time_t can0TxTime;
static absolute_time_t can0RxTime;
static absolute_time_t can1TxTime;
static absolute_time_t can1RxTime;

static absolute_time_t realSpeedMsgTime;
static uint16_t realSpeed;

static void can2040_cb0(CANHandle *cd, uint32_t notify, CANMsg *msg) {
  switch (notify) {
    case CAN2040_NOTIFY_RX:
      can0RxTime = get_absolute_time();
//      printf("Received CAN0\n");
      can2040_transmit(&can1, msg);
      break;
    case CAN2040_NOTIFY_TX:
      can0TxTime = get_absolute_time();
      break;
    case CAN2040_NOTIFY_ERROR:
      break;
  }
}

static void can2040_cb1(CANHandle *cd, uint32_t notify, CANMsg *msg) {
  switch (notify) {
    case CAN2040_NOTIFY_RX: {
      can1RxTime = get_absolute_time();
//      printf("Received CAN1\n");
      if(msg->id == 0x201) { // motor speed msg
        if(!time_reached(delayed_by_ms(realSpeedMsgTime, REAL_SPEED_TIMEOUT_MS))) {
          // no timeout
          *((uint16_t*) msg->data) = realSpeed;
        }
        can2040_transmit(&can0, msg);
      } else if(msg->id == 0x737) { // real speed msg
        realSpeedMsgTime = get_absolute_time();
        realSpeed = *((uint16_t*) msg->data);
      } else {
        can2040_transmit(&can0, msg);
      }
      break;
    }
    case CAN2040_NOTIFY_TX:
      can1TxTime = get_absolute_time();
      break;
    case CAN2040_NOTIFY_ERROR:
      break;
  }
}

static void PIO0_IRQHandler(void) {
  can2040_pio_irq_handler(&can0);
}

static void PIO1_IRQHandler(void) {
  can2040_pio_irq_handler(&can1);
}

void canbus_setup(uint32_t pio_num, uint32_t gpio_rx, uint32_t gpio_tx) {
  uint32_t sys_clock = 125000000, bitrate = 250000;

  CANHandle* canX = pio_num == 0 ? &can0 : &can1;

  // Setup canbus
  can2040_setup(canX, pio_num == 0 ? 0 : 1);
  can2040_callback_config(canX, pio_num == 0 ? can2040_cb0 : can2040_cb1);

  // Enable irqs
  uint irqn = pio_num == 0 ? PIO0_IRQ_0 : PIO1_IRQ_0;
  irq_set_exclusive_handler(irqn, pio_num == 0 ? PIO0_IRQHandler : PIO1_IRQHandler);
  irq_set_priority(irqn, 1);
  irq_set_enabled(irqn, true);

  // Start canbus
  can2040_start(canX, sys_clock, bitrate, gpio_rx, gpio_tx);
}

void wd_cond_trigger() {
  absolute_time_t now = get_absolute_time();
  if(absolute_time_diff_us(can0TxTime, now) < CAN_TIMEOUT_US &&
      absolute_time_diff_us(can0RxTime, now) < CAN_TIMEOUT_US &&
      absolute_time_diff_us(can1TxTime, now) < CAN_TIMEOUT_US &&
      absolute_time_diff_us(can1RxTime, now) < CAN_TIMEOUT_US) {
    watchdog_update();
  }
}

int main() {
  stdio_init_all();

//  sleep_ms(10000);

  watchdog_enable(0x7fffff, 1);
  watchdog_update();

//  printf("Startup delay over\n");

//  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
//  gpio_init(LED_PIN);
//  gpio_set_dir(LED_PIN, GPIO_OUT);
//  uint ledStatus = 0;

  printf("Starting Initialization of CAN\n");
  canbus_setup(0, 13, 14);
  printf("Initialized CAN0\n");
  canbus_setup(1, 8, 10);
  printf("Initialized CAN1\n");

  can0TxTime = make_timeout_time_ms(CAN_TIMEOUT_US / 1000 * 30);
  can0RxTime = can0TxTime;
  can1TxTime = can0TxTime;
  can1RxTime = can0TxTime;


  wd_cond_trigger();
//  sleep_ms(1000);
//  wd_cond_trigger();

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

  absolute_time_t lastPrintTime = get_absolute_time();
  while (true) {
    wd_cond_trigger();
//    CANMsg msg;
//    msg.dlc = 8;
//    msg.id = 34 + ledStatus;
//    msg.data[0] = 11;
//    msg.data[1] = 22;
//    msg.data[2] = 33;
//    msg.data[3] = 44;
//    msg.data[4] = 55;
//    msg.data[5] = 66;
//    msg.data[6] = 77;
//    msg.data[7] = 88;
////    int res = can2040_transmit(&can1, &msg);
//    int res = ledStatus == 0 ? can2040_transmit(&can0, &msg) : can2040_transmit(&can1, &msg);
//    printf("Sending! returned: %d\n", res);
    sleep_ms(100);
//    gpio_put(LED_PIN, ledStatus);
//    ledStatus = !ledStatus;

    absolute_time_t now = get_absolute_time();
    if(absolute_time_diff_us(lastPrintTime, now) >= 1000000) {
      printf("ms since can0Tx: %ld can0Rx: %ld can1Tx: %ld can1Rx: %ld\n", absolute_time_diff_us(can0TxTime, now) / 1000,
             absolute_time_diff_us(can0RxTime, now) / 1000,
             absolute_time_diff_us(can1TxTime, now) / 1000,
             absolute_time_diff_us(can1RxTime, now) / 1000);
      lastPrintTime = now;
    }
  }
#pragma clang diagnostic pop
}
