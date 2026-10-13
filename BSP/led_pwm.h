#ifndef BSP_LED_PWM_H
#define BSP_LED_PWM_H

#include "stm32f1xx_hal.h"

void BSP_LED_PWM_Init(void);
void BSP_LED_PWM_SetBrightness(uint8_t percent);

#endif
