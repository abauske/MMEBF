//
// Created by adria on 17.03.2022.
//

#ifndef MMEBF_CAN_H
#define MMEBF_CAN_H


#include "driver/twai.h"

typedef void (*CAN_MsgHandler)(twai_message_t* msg);

void canInit(CAN_MsgHandler handler);
void canSend(const twai_message_t *msg);

#endif //MMEBF_CAN_H
