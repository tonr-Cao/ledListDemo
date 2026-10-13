/**
  ******************************************************************************
  * @file    si2c.c
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2022年9月3日
  * @brief   软i2c驱动源文件
  ******************************************************************************
  */
#include "si2c.h"

#define scl_w(v) HAL_GPIO_WritePin(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin, ((v)?GPIO_PIN_SET:GPIO_PIN_RESET))
#define sda_w(v) HAL_GPIO_WritePin(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin, ((v)?GPIO_PIN_SET:GPIO_PIN_RESET))
#define scl_r ((HAL_GPIO_ReadPin(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin) == GPIO_PIN_SET) ? 1 : 0)
#define sda_r ((HAL_GPIO_ReadPin(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin) == GPIO_PIN_SET) ? 1 : 0)
void delay(uint32_t us) { for(uint32_t i = 0; i<8*us; i++); }

static uint8_t SendByte(SI2C_TypeDef *SI2C, uint8_t Byte);
static uint8_t ReceiveByte(SI2C_TypeDef *SI2C, uint8_t Ack);
static void SendStop(SI2C_TypeDef *SI2C);

static void SI2C_EnableGpioClock(GPIO_TypeDef *GPIOx)
{
	if(GPIOx == GPIOA)
	{
		__HAL_RCC_GPIOA_CLK_ENABLE();
	}
	else if(GPIOx == GPIOB)
	{
		__HAL_RCC_GPIOB_CLK_ENABLE();
	}
	else if(GPIOx == GPIOC)
	{
		__HAL_RCC_GPIOC_CLK_ENABLE();
	}
	else if(GPIOx == GPIOD)
	{
		__HAL_RCC_GPIOD_CLK_ENABLE();
	}
}


//
// @简介：对软件I2C进行初始化
//
__weak void My_SI2C_Init(SI2C_TypeDef *SI2C)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// #1. 使能SCL和SDA引脚的时钟
	SI2C_EnableGpioClock(SI2C->SCL_GPIOx);
	SI2C_EnableGpioClock(SI2C->SDA_GPIOx);

	// #2. 对SCL和SDA写1
	HAL_GPIO_WritePin(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin, GPIO_PIN_SET);

	// #3. 初始化SCL引脚为输出开漏
	GPIO_InitStruct.Pin = SI2C->SCL_GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(SI2C->SCL_GPIOx, &GPIO_InitStruct);

	// #4. 初始化SDA引脚为输出开漏
	GPIO_InitStruct.Pin = SI2C->SDA_GPIO_Pin;
	HAL_GPIO_Init(SI2C->SDA_GPIOx, &GPIO_InitStruct);
}

//
// @简介：通过软件I2C向从机写入多个字节
// 
// @参数 I2Cx：填写要操作的I2C的名称，可以是I2C1或I2C2
// @参数 Addr：填写从机的地址，左对齐 - A6 A5 A4 A3 A2 A1 A0 0
// @参数 pData：要发送的数据（数组）
// @参数 Size：要发送的数据的数量，以字节为单位
//
// @返回值：0 - 发送成功， -1 - 寻址失败， -2 - 数据被拒收
//
__weak int My_SI2C_SendBytes(SI2C_TypeDef *SI2C, uint8_t Addr, const uint8_t *pData, uint16_t Size)
{
	sda_w(1);
	scl_w(1);
	
	// #1. 发送起始位
	sda_w(0);
	delay(1);
	
	// #2. 发送从机地址+RW
	if(SendByte(SI2C, Addr & 0xfe) != 0)
	{
		SendStop(SI2C);
		return -1; // 寻址失败
	}
	
	// #3. 发送数据
	for(uint16_t i=0; i<Size; i++)
	{
		if(SendByte(SI2C, pData[i]) != 0)
		{
			SendStop(SI2C);
			return -2; // 数据被拒收
		}
	}
	
	// #4. 发送停止位
	SendStop(SI2C);
	
	return 0;
}

//
// @简介：通过软件I2C从从机读多个字节
// 
// @参数 I2Cx：填写要操作的I2C的名称，可以是I2C1或I2C2
// @参数 Addr：填写从机的地址，左对齐 - A6 A5 A4 A3 A2 A1 A0 0
// @参数 pBuffer：接收缓冲区（数组）
// @参数 Size：要读取的数据的数量，以字节为单位
//
// @返回值：0 - 发送成功， -1 - 寻址失败
//
__weak int My_SI2C_ReceiveBytes(SI2C_TypeDef *SI2C, uint8_t Addr, uint8_t *pBuffer, uint16_t Size)
{
	sda_w(1);
	scl_w(1);
	
	// #1. 发送起始位
	sda_w(0);
	delay(1);
	
	// #2. 发送从机地址+RW
	if(SendByte(SI2C, Addr | 0x01) != 0)
	{
		SendStop(SI2C);
		return -1; // 寻址失败
	}
	
	// #3. 接收
	for(uint16_t i=0; i<Size; i++)
	{
		pBuffer[i] = ReceiveByte(SI2C, (i==Size-1) ? 1 : 0);
	}
	
	// #4. 发送停止位
	SendStop(SI2C);
	
	return 0;
}

//
// @简介：发送一个字节
//
// @返回值：0-ACK，其它-NAK
//
static uint8_t SendByte(SI2C_TypeDef *SI2C, uint8_t Byte)
{
	for(int8_t i=7; i>=0; i--)
	{
		scl_w(0); // 将SCL拉低
		sda_w((Byte & (0x01<<i)) ? 1 : 0); // 变SDA的电压
		delay(2); // 延迟1/2周期
		
		scl_w(1); // 将SCL拉高
		delay(2); // 延迟1/2周期
	}
	
	// 读取ACK
	scl_w(0); // 将SCL拉低
	sda_w(1); // 将SDA释放
	delay(2); // 延迟1/4周期
	
	scl_w(1); // 将SCL拉高
	delay(2); // 延迟1/4周期
	
	return sda_r;
}

//
// @简介：发送停止位
//
static void SendStop(SI2C_TypeDef *SI2C)
{
	scl_w(0); // scl拉低
	delay(1); // 延迟1/4周期
	sda_w(0); // sda拉低
	delay(1); // 延迟1/4周期
	scl_w(1); // scl拉高
	delay(1); // 延迟1/4周期
	sda_w(1); // sda拉高
	delay(1); // 延迟1/4周期
}


//
// @简介：从从机读取一个字节的数据
// @参数 Ack：0 - 回NAK，1 - 回ACK
// @返回值：读取到的数据
//
static uint8_t ReceiveByte(SI2C_TypeDef *SI2C, uint8_t Ack)
{
	uint8_t ret = 0;
	
	for(int8_t i=7; i>=0; i--)
	{
		scl_w(0); // scl拉低
		sda_w(1); // 释放SDA
		delay(2); // 延迟1/2周期
		scl_w(1); // scl拉高
		delay(2); // 延迟1/2周期
		
		if(sda_r) // 如果读到的比特位为1
		{
			ret |= 0x01 << i; // 写入比特位
		}
		else // 如果读到的比特位为0
		{
			// 什么也不干
		}
	}
	
	// 回复ACK或NAK
	
	scl_w(0); // scl拉低
	
	if(Ack)
	{
		sda_w(0); // sda拉低
	}
	else
	{
		sda_w(1); // sda拉高
	}
	
	delay(2); // 延迟1/2周期
	
	return ret;
}
