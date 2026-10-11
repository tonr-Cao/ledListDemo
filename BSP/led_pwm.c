#include "led_pwm.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

#define LED_PWM_PERIOD_TICKS 1000

void BSP_LED_PWM_Init(void)
{
	GPIO_InitTypeDef gpio_init;
	TIM_TimeBaseInitTypeDef timer_base;
	TIM_OCInitTypeDef output_compare;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	gpio_init.GPIO_Pin = GPIO_Pin_6;
	gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_init);

	TIM_TimeBaseStructInit(&timer_base);
	timer_base.TIM_Prescaler = 71;
	timer_base.TIM_CounterMode = TIM_CounterMode_Up;
	timer_base.TIM_Period = LED_PWM_PERIOD_TICKS - 1;
	timer_base.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM3, &timer_base);

	TIM_OCStructInit(&output_compare);
	output_compare.TIM_OCMode = TIM_OCMode_PWM1;
	output_compare.TIM_OutputState = TIM_OutputState_Enable;
	output_compare.TIM_Pulse = 0;
	output_compare.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &output_compare);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM3, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
}

void BSP_LED_PWM_SetBrightness(uint8_t percent)
{
	uint16_t compare;

	if(percent > 100)
	{
		percent = 100;
	}

	compare = (LED_PWM_PERIOD_TICKS * percent) / 100;
	TIM_SetCompare1(TIM3, compare);
}
