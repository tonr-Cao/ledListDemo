/**
  ******************************************************************************
  * @file    stm32f1xx_hal_conf.h
  * @brief   HAL configuration file for template_LED (STM32F103C8T6).
  *          Trimmed from stm32f1xx_hal_conf_template.h: only the modules used
  *          by this project are enabled.
  ******************************************************************************
  */

#ifndef __STM32F1xx_HAL_CONF_H
#define __STM32F1xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ########################## Module Selection ############################## */
#define HAL_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED

/* ########################## Oscillator Values adaptation ####################*/
#if !defined  (HSE_VALUE)
#define HSE_VALUE              8000000U  /*!< External oscillator in Hz (8 MHz crystal) */
#endif /* HSE_VALUE */

#if !defined  (HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT    100U      /*!< Time out for HSE start up, in ms */
#endif /* HSE_STARTUP_TIMEOUT */

#if !defined  (HSI_VALUE)
#define HSI_VALUE              8000000U  /*!< Internal oscillator in Hz */
#endif /* HSI_VALUE */

#if !defined  (LSI_VALUE)
#define LSI_VALUE               40000U   /*!< Internal Low Speed oscillator in Hz */
#endif /* LSI_VALUE */

#if !defined  (LSE_VALUE)
#define LSE_VALUE               32768U   /*!< External Low Speed oscillator in Hz */
#endif /* LSE_VALUE */

#if !defined  (LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT    5000U     /*!< Time out for LSE start up, in ms */
#endif /* LSE_STARTUP_TIMEOUT */

/* ########################### System Configuration ######################### */
#define  VDD_VALUE                    3300U /*!< Value of VDD in mv */
#define  TICK_INT_PRIORITY            0x0FU /*!< tick interrupt priority */
#define  USE_RTOS                     0U
#define  PREFETCH_ENABLE              1U

#define  USE_HAL_ADC_REGISTER_CALLBACKS         0U
#define  USE_HAL_SPI_REGISTER_CALLBACKS         0U
#define  USE_HAL_TIM_REGISTER_CALLBACKS         0U

/* ########################## Assert Selection ############################## */
/* #define USE_FULL_ASSERT    1U */

/* ################## SPI CRC feature selection ############################# */
/* 0U: HAL SPI 驱动中不编译CRC相关代码（本工程不使用SPI CRC） */
#define USE_SPI_CRC                     0U

/* ########################## Module Include Section ######################## */
#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32f1xx_hal_rcc.h"
#endif /* HAL_RCC_MODULE_ENABLED */

#ifdef HAL_EXTI_MODULE_ENABLED
#include "stm32f1xx_hal_exti.h"
#endif /* HAL_EXTI_MODULE_ENABLED */

#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32f1xx_hal_gpio.h"
#endif /* HAL_GPIO_MODULE_ENABLED */

#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32f1xx_hal_dma.h"
#endif /* HAL_DMA_MODULE_ENABLED */

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32f1xx_hal_cortex.h"
#endif /* HAL_CORTEX_MODULE_ENABLED */

#ifdef HAL_ADC_MODULE_ENABLED
#include "stm32f1xx_hal_adc.h"
#endif /* HAL_ADC_MODULE_ENABLED */

#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32f1xx_hal_flash.h"
#endif /* HAL_FLASH_MODULE_ENABLED */

#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32f1xx_hal_pwr.h"
#endif /* HAL_PWR_MODULE_ENABLED */

#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32f1xx_hal_spi.h"
#endif /* HAL_SPI_MODULE_ENABLED */

#ifdef HAL_TIM_MODULE_ENABLED
#include "stm32f1xx_hal_tim.h"
#endif /* HAL_TIM_MODULE_ENABLED */

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t* file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_HAL_CONF_H */
