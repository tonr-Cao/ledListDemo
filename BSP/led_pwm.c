#include "led_pwm.h"

#define LED_PWM_PERIOD_TICKS 1000
#define LED_PWM_CHANNEL TIM_CHANNEL_1

static TIM_HandleTypeDef led_pwm_timer;

void BSP_LED_PWM_Init(void)
{
	GPIO_InitTypeDef gpio_init = {0};
	TIM_OC_InitTypeDef output_compare = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_TIM3_CLK_ENABLE();

	gpio_init.Pin = GPIO_PIN_6;
	gpio_init.Mode = GPIO_MODE_AF_PP;
	gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &gpio_init);

	led_pwm_timer.Instance = TIM3;
	led_pwm_timer.Init.Prescaler = 71;
	led_pwm_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
	led_pwm_timer.Init.Period = LED_PWM_PERIOD_TICKS - 1;
	led_pwm_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	led_pwm_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	HAL_TIM_Base_Init(&led_pwm_timer);

	output_compare.OCMode = TIM_OCMODE_PWM1;
	output_compare.Pulse = 0;
	output_compare.OCPolarity = TIM_OCPOLARITY_HIGH;
	HAL_TIM_PWM_ConfigChannel(&led_pwm_timer, &output_compare, LED_PWM_CHANNEL);

	HAL_TIM_PWM_Start(&led_pwm_timer, LED_PWM_CHANNEL);
}

void BSP_LED_PWM_SetBrightness(uint8_t percent)
{
	uint16_t compare;

	if(percent > 100)
	{
		percent = 100;
	}

	compare = (LED_PWM_PERIOD_TICKS * percent) / 100;
	__HAL_TIM_SET_COMPARE(&led_pwm_timer, LED_PWM_CHANNEL, compare);
}
