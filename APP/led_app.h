#ifndef APP_LED_APP_H
#define APP_LED_APP_H

#include "stm32f1xx_hal.h"

void LED_App_Init(uint8_t initial_brightness);
uint8_t LED_App_AdjustBrightness(int8_t direction);
uint8_t LED_App_GetBrightness(void);
void LED_App_SetBrightness(uint8_t brightness);

#endif
