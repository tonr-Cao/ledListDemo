#include "w25qxx.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "delay.h"

#define W25QXX_CS_PORT GPIOB
#define W25QXX_CS_PIN GPIO_Pin_12
#define W25QXX_READ_JEDEC_ID 0x9F
#define W25QXX_READ_DATA 0x03
#define W25QXX_WRITE_ENABLE 0x06
#define W25QXX_READ_STATUS 0x05
#define W25QXX_PAGE_PROGRAM 0x02
#define W25QXX_SECTOR_ERASE 0x20
#define W25QXX_SPI_TIMEOUT 100000UL
#define W25QXX_PROGRAM_TIMEOUT_MS 100UL
#define W25QXX_ERASE_TIMEOUT_MS 3000UL

static uint8_t W25QXX_TransferByte(uint8_t transmit_data, uint8_t *receive_data)
{
	uint32_t timeout;

	timeout = W25QXX_SPI_TIMEOUT;
	while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
	{
		if(--timeout == 0)
		{
			return 0;
		}
	}
	SPI_I2S_SendData(SPI2, transmit_data);

	timeout = W25QXX_SPI_TIMEOUT;
	while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET)
	{
		if(--timeout == 0)
		{
			return 0;
		}
	}
	*receive_data = (uint8_t)SPI_I2S_ReceiveData(SPI2);
	timeout = W25QXX_SPI_TIMEOUT;
	while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET)
	{
		if(--timeout == 0)
		{
			return 0;
		}
	}
	return 1;
}

static uint8_t W25QXX_SendCommand(uint8_t command)
{
	uint8_t receive_data;
	uint8_t success;

	GPIO_ResetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	success = W25QXX_TransferByte(command, &receive_data);
	GPIO_SetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	return success;
}

static uint8_t W25QXX_WriteEnable(void)
{
	return W25QXX_SendCommand(W25QXX_WRITE_ENABLE);
}

static uint8_t W25QXX_ReadStatus(uint8_t *status)
{
	uint8_t success;
	uint8_t receive_data;

	GPIO_ResetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	success = W25QXX_TransferByte(W25QXX_READ_STATUS, &receive_data);
	if(success)
	{
		success = W25QXX_TransferByte(0xFF, status);
	}
	GPIO_SetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	return success;
}

static uint8_t W25QXX_WaitReady(uint32_t timeout_ms)
{
	uint32_t started_at;
	uint8_t status;

	started_at = GetTick();
	while((uint32_t)(GetTick() - started_at) < timeout_ms)
	{
		if(!W25QXX_ReadStatus(&status))
		{
			return 0;
		}
		if((status & 0x01) == 0)
		{
			return 1;
		}
	}
	return 0;
}

static uint8_t W25QXX_SendAddress(uint32_t address)
{
	uint8_t receive_data;

	return W25QXX_TransferByte((uint8_t)(address >> 16), &receive_data) &&
		W25QXX_TransferByte((uint8_t)(address >> 8), &receive_data) &&
		W25QXX_TransferByte((uint8_t)address, &receive_data);
}

void W25QXX_Init(void)
{
	GPIO_InitTypeDef gpio_init;
	SPI_InitTypeDef spi_init;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	GPIO_SetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	gpio_init.GPIO_Pin = W25QXX_CS_PIN;
	gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(W25QXX_CS_PORT, &gpio_init);

	gpio_init.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
	gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &gpio_init);

	gpio_init.GPIO_Pin = GPIO_Pin_14;
	gpio_init.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &gpio_init);

	SPI_I2S_DeInit(SPI2);
	SPI_StructInit(&spi_init);
	spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spi_init.SPI_Mode = SPI_Mode_Master;
	spi_init.SPI_DataSize = SPI_DataSize_8b;
	spi_init.SPI_CPOL = SPI_CPOL_Low;
	spi_init.SPI_CPHA = SPI_CPHA_1Edge;
	spi_init.SPI_NSS = SPI_NSS_Soft;
	spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
	spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
	spi_init.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &spi_init);
	SPI_NSSInternalSoftwareConfig(SPI2, SPI_NSSInternalSoft_Set);
	SPI_Cmd(SPI2, ENABLE);
}

uint8_t W25QXX_ReadJEDECID(uint8_t jedec_id[3])
{
	uint8_t receive_data;
	uint8_t i;
	uint8_t success;

	if(jedec_id == 0)
	{
		return 0;
	}

	success = 1;
	GPIO_ResetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	if(!W25QXX_TransferByte(W25QXX_READ_JEDEC_ID, &receive_data))
	{
		success = 0;
	}
	for(i = 0; i < 3 && success; i++)
	{
		if(!W25QXX_TransferByte(0xFF, &jedec_id[i]))
		{
			success = 0;
		}
	}
	GPIO_SetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);

	if(!success)
	{
		jedec_id[0] = 0;
		jedec_id[1] = 0;
		jedec_id[2] = 0;
	}
	return success;
}

uint8_t W25QXX_IsConnected(const uint8_t jedec_id[3])
{
	if(jedec_id == 0)
	{
		return 0;
	}

	return (uint8_t)(jedec_id[0] == 0xEF &&
		jedec_id[1] != 0x00 && jedec_id[1] != 0xFF &&
		jedec_id[2] != 0x00 && jedec_id[2] != 0xFF);
}

uint8_t W25QXX_ReadData(uint32_t address, uint8_t *data, uint16_t size)
{
	uint16_t i;
	uint8_t receive_data;
	uint8_t success;

	if((size > 0 && data == 0) || address >= W25QXX_CAPACITY_BYTES ||
		size > W25QXX_CAPACITY_BYTES - address)
	{
		return 0;
	}
	if(size == 0)
	{
		return 1;
	}

	GPIO_ResetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	success = W25QXX_TransferByte(W25QXX_READ_DATA, &receive_data) &&
		W25QXX_SendAddress(address);
	for(i = 0; i < size && success; i++)
	{
		success = W25QXX_TransferByte(0xFF, &data[i]);
	}
	GPIO_SetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	return success;
}

uint8_t W25QXX_EraseSector(uint32_t address)
{
	uint8_t receive_data;
	uint8_t success;

	if(address >= W25QXX_CAPACITY_BYTES ||
		(address % W25QXX_SECTOR_SIZE) != 0)
	{
		return 0;
	}
	if(!W25QXX_WaitReady(W25QXX_ERASE_TIMEOUT_MS) ||
		!W25QXX_WriteEnable())
	{
		return 0;
	}

	GPIO_ResetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	success = W25QXX_TransferByte(W25QXX_SECTOR_ERASE, &receive_data) &&
		W25QXX_SendAddress(address);
	GPIO_SetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	if(!success)
	{
		return 0;
	}

	return W25QXX_WaitReady(W25QXX_ERASE_TIMEOUT_MS);
}

uint8_t W25QXX_WritePage(uint32_t address, const uint8_t *data, uint16_t size)
{
	uint16_t i;
	uint8_t receive_data;
	uint8_t success;

	if(data == 0 || size == 0 || size > 256 ||
		address >= W25QXX_CAPACITY_BYTES ||
		size > W25QXX_CAPACITY_BYTES - address ||
		(address & 0xFF) + size > 256)
	{
		return 0;
	}
	if(!W25QXX_WaitReady(W25QXX_PROGRAM_TIMEOUT_MS) ||
		!W25QXX_WriteEnable())
	{
		return 0;
	}

	GPIO_ResetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	success = W25QXX_TransferByte(W25QXX_PAGE_PROGRAM, &receive_data) &&
		W25QXX_SendAddress(address);
	for(i = 0; i < size && success; i++)
	{
		success = W25QXX_TransferByte(data[i], &receive_data);
	}
	GPIO_SetBits(W25QXX_CS_PORT, W25QXX_CS_PIN);
	if(!success)
	{
		return 0;
	}

	return W25QXX_WaitReady(W25QXX_PROGRAM_TIMEOUT_MS);
}
