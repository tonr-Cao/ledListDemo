#include "adc.h"

#define ADC_MONITOR_CHANNEL ADC_CHANNEL_3
#define ADC_MONITOR_RANK 1
#define ADC_CONVERSION_TIMEOUT_MS 10UL

static ADC_HandleTypeDef adc_monitor;

void BSP_ADC_Init(void)
{
	GPIO_InitTypeDef gpio_init = {0};
	ADC_ChannelConfTypeDef channel_config = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_ADC1_CLK_ENABLE();
	__HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV6);

	gpio_init.Pin = GPIO_PIN_3;
	gpio_init.Mode = GPIO_MODE_ANALOG;
	HAL_GPIO_Init(GPIOA, &gpio_init);

	adc_monitor.Instance = ADC1;
	adc_monitor.Init.ScanConvMode = ADC_SCAN_DISABLE;
	adc_monitor.Init.ContinuousConvMode = DISABLE;
	adc_monitor.Init.DiscontinuousConvMode = DISABLE;
	adc_monitor.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	adc_monitor.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	adc_monitor.Init.NbrOfConversion = 1;
	HAL_ADC_Init(&adc_monitor);

	channel_config.Channel = ADC_MONITOR_CHANNEL;
	channel_config.Rank = ADC_MONITOR_RANK;
	channel_config.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
	HAL_ADC_ConfigChannel(&adc_monitor, &channel_config);

	HAL_ADCEx_Calibration_Start(&adc_monitor);
}

uint16_t BSP_ADC_ReadChannel3(void)
{
	HAL_ADC_Start(&adc_monitor);
	HAL_ADC_PollForConversion(&adc_monitor, ADC_CONVERSION_TIMEOUT_MS);
	return (uint16_t)HAL_ADC_GetValue(&adc_monitor);
}
