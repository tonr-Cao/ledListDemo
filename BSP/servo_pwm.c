#include "servo_pwm.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

#define SERVO_TIMER_PRESCALER 71
#define SERVO_TIMER_PERIOD 20000

static uint16_t servo_minimum_us = SERVO_PULSE_MIN_US;
static uint16_t servo_maximum_us = SERVO_PULSE_MAX_US;

void BSP_ServoPWM_Init(void)
{
	GPIO_InitTypeDef gpio_init;
	TIM_TimeBaseInitTypeDef timer_base;
	TIM_OCInitTypeDef output_compare;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1,
		ENABLE);

	gpio_init.GPIO_Pin = GPIO_Pin_8;
	gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_init);

	TIM_TimeBaseStructInit(&timer_base);
	timer_base.TIM_Prescaler = SERVO_TIMER_PRESCALER;
	timer_base.TIM_CounterMode = TIM_CounterMode_Up;
	timer_base.TIM_Period = SERVO_TIMER_PERIOD - 1;
	timer_base.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM1, &timer_base);

	TIM_OCStructInit(&output_compare);
	output_compare.TIM_OCMode = TIM_OCMode_PWM1;
	output_compare.TIM_OutputState = TIM_OutputState_Enable;
	output_compare.TIM_Pulse = SERVO_PULSE_CENTER_US;
	output_compare.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM1, &output_compare);
	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM1, ENABLE);
	TIM_CtrlPWMOutputs(TIM1, ENABLE);
	TIM_Cmd(TIM1, ENABLE);
}

void BSP_ServoPWM_SetPulse(uint16_t pulse_us)
{
	if(pulse_us < servo_minimum_us)
	{
		pulse_us = servo_minimum_us;
	}
	else if(pulse_us > servo_maximum_us)
	{
		pulse_us = servo_maximum_us;
	}

	TIM_SetCompare1(TIM1, pulse_us);
}

uint8_t BSP_ServoPWM_SetLimits(uint16_t minimum_us, uint16_t center_us,
	uint16_t maximum_us)
{
	if(minimum_us < SERVO_PULSE_MIN_US ||
		minimum_us >= center_us ||
		center_us >= maximum_us ||
		maximum_us > SERVO_PULSE_MAX_US)
	{
		return 0;
	}

	servo_minimum_us = minimum_us;
	servo_maximum_us = maximum_us;
	BSP_ServoPWM_SetPulse(center_us);
	return 1;
}
