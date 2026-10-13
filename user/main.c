#include "stm32f1xx_hal.h"
#include "delay.h"
#include "oled.h"
#include "menu.h"

static void Error_Handler(void);
void SystemClock_Config(void);

int main(void)
{
	static OLED_TypeDef display;

	HAL_Init();
	SystemClock_Config();

	Delay_Init();
	if(BSP_OLED_Init(&display) != 0)
	{
		return -1;
	}
	Menu_Init(&display);

	while(1)
	{
		Menu_Process();
		Delay(5);
	}
}

//
// @简介：配置系统时钟
// 			HSE(8MHz) -> PLL x9 -> SYSCLK 72MHz
// 			AHB = 72MHz，APB1 = 36MHz，APB2 = 72MHz，FLASH_LATENCY_2
//
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
		RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
}

//
// @简介：HAL的底层初始化回调
//
void HAL_MspInit(void)
{
	__HAL_RCC_AFIO_CLK_ENABLE();
	__HAL_RCC_PWR_CLK_ENABLE();
}

//
// @简介：初始化失败时停在此处，方便调试定位
//
static void Error_Handler(void)
{
	__disable_irq();
	while(1)
	{
	}
}
