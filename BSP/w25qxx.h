#ifndef BSP_W25QXX_H
#define BSP_W25QXX_H

#include "stm32f1xx_hal.h"

#define W25QXX_CAPACITY_BYTES 0x00800000UL
#define W25QXX_SECTOR_SIZE 4096UL

void W25QXX_Init(void);
uint8_t W25QXX_ReadJEDECID(uint8_t jedec_id[3]);
uint8_t W25QXX_IsConnected(const uint8_t jedec_id[3]);
uint8_t W25QXX_ReadData(uint32_t address, uint8_t *data, uint16_t size);
uint8_t W25QXX_EraseSector(uint32_t address);
uint8_t W25QXX_WritePage(uint32_t address, const uint8_t *data, uint16_t size);

#endif
