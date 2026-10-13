#ifndef BSP_SERVO_PWM_H
#define BSP_SERVO_PWM_H

#include "stm32f1xx_hal.h"

#define SERVO_PULSE_MIN_US 1000
#define SERVO_PULSE_MAX_US 2000
#define SERVO_PULSE_CENTER_US 1500

void BSP_ServoPWM_Init(void);
uint8_t BSP_ServoPWM_SetLimits(uint16_t minimum_us, uint16_t center_us,
	uint16_t maximum_us);
void BSP_ServoPWM_SetPulse(uint16_t pulse_us);

#endif
