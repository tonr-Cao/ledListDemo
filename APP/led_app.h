#ifndef APP_LED_APP_H
#define APP_LED_APP_H

#include "stm32f10x.h"

void LED_App_Init(void);
uint8_t LED_App_AdjustBrightness(int8_t direction);
uint8_t LED_App_GetBrightness(void);

#endif
