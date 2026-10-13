#ifndef BSP_ADC_H
#define BSP_ADC_H

#include "stm32f1xx_hal.h"

void BSP_ADC_Init(void);
uint16_t BSP_ADC_ReadChannel3(void);

#endif
