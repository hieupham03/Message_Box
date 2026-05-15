#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "Globals.h"

void checkAlarm();
void TaskTelegram(void *pvParameters);
void setupNetworkAndBlynk();
void processNetworkTasks();
void blynkWrite(int vPin, int value);

#endif // NETWORK_MANAGER_H
