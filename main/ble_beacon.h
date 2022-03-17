//
// Created by adria on 17.03.2022.
//

#ifndef MMEBF_BLE_BEACON_H
#define MMEBF_BLE_BEACON_H

#include <stdint-gcc.h>

void bleInit();
void bleSetData(uint8_t* data, uint8_t len);

#endif //MMEBF_BLE_BEACON_H
