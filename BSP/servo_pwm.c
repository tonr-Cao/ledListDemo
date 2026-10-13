#include "servo_pwm.h"

#define SERVO_TIMER_PRESCALER 71
#define SERVO_TIMER_PERIOD 20000
#define SERVO_PWM_CHANNEL TIM_CHANNEL_1

static TIM_HandleTypeDef servo_pwm_timer;

static uint16_t servo_minimum_us = SERVO_PULSE_MIN_US;
static uint16_t servo_maximum_us = SERVO_PULSE_MAX_US;

void BSP_ServoPWM_Init(void)
{
	GPIO_InitTypeDef gpio_init = {0};
	TIM_OC_InitTypeDef output_compare = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_TIM1_CLK_ENABLE();

	gpio_init.Pin = GPIO_PIN_8;
	gpio_init.Mode = GPIO_MODE_AF_PP;
	gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &gpio_init);

	servo_pwm_timer.Instance = TIM1;
	servo_pwm_timer.Init.Prescaler = SERVO_TIMER_PRESCALER;
	servo_pwm_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
	servo_pwm_timer.Init.Period = SERVO_TIMER_PERIOD - 1;
	servo_pwm_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	servo_pwm_timer.Init.RepetitionCounter = 0;
	servo_pwm_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	HAL_TIM_Base_Init(&servo_pwm_timer);

	output_compare.OCMode = TIM_OCMODE_PWM1;
	output_compare.Pulse = SERVO_PULSE_CENTER_US;
	output_compare.OCPolarity = TIM_OCPOLARITY_HIGH;
	HAL_TIM_PWM_ConfigChannel(&servo_pwm_timer, &output_compare, SERVO_PWM_CHANNEL);

	/* TIM1是高级定时器，HAL_TIM_PWM_Start内部会同时使能MOE主输出 */
	HAL_TIM_PWM_Start(&servo_pwm_timer, SERVO_PWM_CHANNEL);
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

	__HAL_TIM_SET_COMPARE(&servo_pwm_timer, SERVO_PWM_CHANNEL, pulse_us);
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
