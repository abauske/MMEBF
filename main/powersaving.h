//
// Created by adria on 19.03.2022.
//

#ifndef MMEBF_POWERSAVING_H
#define MMEBF_POWERSAVING_H

typedef void (*powerSaveAction)();

void powerSavingInit();
void enterPowerSaveMode();
void resumeFromPowerSaveMode();
void registerPowerSavingActions(powerSaveAction enterPowerSaveAction, powerSaveAction exitPowerSaveAction);

#endif //MMEBF_POWERSAVING_H
