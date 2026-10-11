#include "led_app.h"
#include "led_pwm.h"

#define LED_BRIGHTNESS_STEP 5
#define LED_BRIGHTNESS_INITIAL 65

static uint8_t brightness;

void LED_App_Init(void)
{
	brightness = LED_BRIGHTNESS_INITIAL;
	BSP_LED_PWM_Init();
	BSP_LED_PWM_SetBrightness(brightness);
}

uint8_t LED_App_AdjustBrightness(int8_t direction)
{
	if(direction < 0)
	{
		if(brightness == 0)
		{
			return 0;
		}
		brightness = (brightness < LED_BRIGHTNESS_STEP) ?
			0 : brightness - LED_BRIGHTNESS_STEP;
	}
	else if(direction > 0)
	{
		if(brightness == 100)
		{
			return 0;
		}
		brightness = (brightness > 100 - LED_BRIGHTNESS_STEP) ?
			100 : brightness + LED_BRIGHTNESS_STEP;
	}
	else
	{
		return 0;
	}

	BSP_LED_PWM_SetBrightness(brightness);
	return 1;
}

uint8_t LED_App_GetBrightness(void)
{
	return brightness;
}
