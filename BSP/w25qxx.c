#include "w25qxx.h"
#include "delay.h"

#define W25QXX_CS_PORT GPIOB
#define W25QXX_CS_PIN GPIO_PIN_12
#define W25QXX_READ_JEDEC_ID 0x9F
#define W25QXX_READ_DATA 0x03
#define W25QXX_WRITE_ENABLE 0x06
#define W25QXX_READ_STATUS 0x05
#define W25QXX_PAGE_PROGRAM 0x02
#define W25QXX_SECTOR_ERASE 0x20
#define W25QXX_SPI_TIMEOUT_MS 100UL
#define W25QXX_SPI_TXE_TIMEOUT 100000UL
#define W25QXX_PROGRAM_TIMEOUT_MS 100UL
#define W25QXX_ERASE_TIMEOUT_MS 3000UL

static SPI_HandleTypeDef w25qxx_spi;

static uint8_t W25QXX_TransferByte(uint8_t transmit_data, uint8_t *receive_data)
{
	uint32_t timeout;

	// 与SPL版本一致：写DR之前先确认TXE已置位。
	// HAL在Size==1时会紧跟SPE使能后无条件写DR，若TXE尚未有效该字节会被丢弃。
	timeout = W25QXX_SPI_TXE_TIMEOUT;
	while(__HAL_SPI_GET_FLAG(&w25qxx_spi, SPI_FLAG_TXE) == RESET)
	{
		if(--timeout == 0)
		{
			return 0;
		}
	}

	return HAL_SPI_TransmitReceive(&w25qxx_spi, &transmit_data, receive_data, 1,
		W25QXX_SPI_TIMEOUT_MS) == HAL_OK;
}

static uint8_t W25QXX_SendCommand(uint8_t command)
{
	uint8_t receive_data;
	uint8_t success;

	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_RESET);
	success = W25QXX_TransferByte(command, &receive_data);
	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_SET);
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

	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_RESET);
	success = W25QXX_TransferByte(W25QXX_READ_STATUS, &receive_data);
	if(success)
	{
		success = W25QXX_TransferByte(0xFF, status);
	}
	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_SET);
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
	GPIO_InitTypeDef gpio_init = {0};

	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_SPI2_CLK_ENABLE();

	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_SET);
	gpio_init.Pin = W25QXX_CS_PIN;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init.Pull = GPIO_NOPULL;
	gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(W25QXX_CS_PORT, &gpio_init);

	gpio_init.Pin = GPIO_PIN_13 | GPIO_PIN_15;
	gpio_init.Mode = GPIO_MODE_AF_PP;
	HAL_GPIO_Init(GPIOB, &gpio_init);

	gpio_init.Pin = GPIO_PIN_14;
	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &gpio_init);

	w25qxx_spi.Instance = SPI2;
	w25qxx_spi.Init.Mode = SPI_MODE_MASTER;
	w25qxx_spi.Init.Direction = SPI_DIRECTION_2LINES;
	w25qxx_spi.Init.DataSize = SPI_DATASIZE_8BIT;
	w25qxx_spi.Init.CLKPolarity = SPI_POLARITY_LOW;
	w25qxx_spi.Init.CLKPhase = SPI_PHASE_1EDGE;
	w25qxx_spi.Init.NSS = SPI_NSS_SOFT;
	w25qxx_spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	w25qxx_spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
	w25qxx_spi.Init.TIMode = SPI_TIMODE_DISABLE;
	w25qxx_spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	w25qxx_spi.Init.CRCPolynomial = 7;
	HAL_SPI_Init(&w25qxx_spi);

	// 与SPL版本一致(SPI_Cmd(SPI2, ENABLE))：初始化阶段就使能SPI，
	// 使TXE状态在第一次收发之前就已经稳定。
	__HAL_SPI_ENABLE(&w25qxx_spi);
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
	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_RESET);
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
	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_SET);

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

	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_RESET);
	success = W25QXX_TransferByte(W25QXX_READ_DATA, &receive_data) &&
		W25QXX_SendAddress(address);
	for(i = 0; i < size && success; i++)
	{
		success = W25QXX_TransferByte(0xFF, &data[i]);
	}
	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_SET);
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

	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_RESET);
	success = W25QXX_TransferByte(W25QXX_SECTOR_ERASE, &receive_data) &&
		W25QXX_SendAddress(address);
	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_SET);
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

	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_RESET);
	success = W25QXX_TransferByte(W25QXX_PAGE_PROGRAM, &receive_data) &&
		W25QXX_SendAddress(address);
	for(i = 0; i < size && success; i++)
	{
		success = W25QXX_TransferByte(data[i], &receive_data);
	}
	HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_SET);
	if(!success)
	{
		return 0;
	}

	return W25QXX_WaitReady(W25QXX_PROGRAM_TIMEOUT_MS);
}
