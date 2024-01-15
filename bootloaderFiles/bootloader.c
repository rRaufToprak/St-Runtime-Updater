/**
@file: bootloader.c
@Author: rRaufToprak
@Date: 11.01.2024
*/
#include "bootloader.h"

//***************Verify Functions***************************************
static uint8_t bl_verify_crc(uint8_t *buffer, uint32_t len, uint32_t hostCrc)
{
	uint32_t crcValue = 0xFF;
	uint32_t data = 0;
	
	for(uint32_t i = 0;i < len; i++)
	{
		data = buffer[i];
		crcValue = HAL_CRC_Accumulate(&hcrc, &data, 1);
	}
	__HAL_CRC_DR_RESET(&hcrc);
	
	if(crcValue == hostCrc)
		return CRC_SUCCESS;
	else 
		return CRC_FAIL;
}

static void bl_send_ack(uint8_t followLen)
{
	uint8_t ack_buff[2];
	ack_buff[0] = BL_ACK_VALUE;
	ack_buff[1] = followLen;
	
	HAL_UART_Transmit(&huart1, ack_buff, 2, HAL_MAX_DELAY);
}

static void bl_send_nack(void)
{	
	uint8_t nackValue = BL_NACK_VALUE;
	HAL_UART_Transmit(&huart1, &nackValue, 1, HAL_MAX_DELAY);
}

static uint8_t get_flash_rdp_level(void)
{
	uint8_t rdpLevel = 0;
	
	volatile uint32_t *OB_Addr = (uint32_t *) 0x1FFFC000;
	rdpLevel = (uint8_t)(*OB_Addr>>8);

	return rdpLevel;
}
static uint8_t bootloader_verify_address(uint32_t goAddress)
{
	if(goAddress >= FLASH_BASE && goAddress <= FLASH_END)
		return ADDR_VALID;
	else if(goAddress >= SRAM1_BASE && goAddress <= SRAM1_END)
		return ADDR_VALID;
	else	
		return ADDR_INVALID;
}
static void bl_write_uart_data(uint8_t *buffer, uint32_t len)
{
	HAL_UART_Transmit(&huart1, buffer, len, HAL_MAX_DELAY);
}
static uint8_t flash_erase(uint8_t sectorNum, uint8_t numberOfSector)
{
	FLASH_EraseInitTypeDef FlashEraseInitStruct = {0};
	uint32_t sectorError = 0;
	uint8_t status;
	
	if(numberOfSector > 7)
		return INVALID_SECTOR;
	
	if(sectorNum <=7 || sectorNum == 0xFF)
	{
		if(sectorNum == 0xFF)
		{
			FlashEraseInitStruct.TypeErase = FLASH_TYPEERASE_MASSERASE;
		}
		else
		{
				uint8_t remaingSector = 7 - sectorNum;
				if(sectorNum > remaingSector)
					sectorNum = remaingSector;
		
		FlashEraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
		FlashEraseInitStruct.Sector = sectorNum;
		FlashEraseInitStruct.NbSectors = numberOfSector;
		
		}
		FlashEraseInitStruct.Banks = FLASH_BANK_1;
		
		HAL_FLASH_Unlock();
		FlashEraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
		status = (uint8_t) HAL_FLASHEx_Erase(&FlashEraseInitStruct, &sectorError);
		HAL_FLASH_Lock();
		
		return status;
	}
}

static uint8_t memory_write(uint8_t *buffer, uint32_t memAddr, uint32_t len)
{
	HAL_StatusTypeDef status;
	
	HAL_FLASH_Unlock();
	
	for(uint32_t i = 0 ;i < len ; i++)
	{
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, memAddr, buffer[i]);
	}
	HAL_FLASH_Lock();
	
	return status;
}
//**************Command Functions***************************************
void bl_get_rdp_cmd(uint8_t *bootloader_rx_data)//Read RDP Level
{
	uint8_t rdpLevel = 0;
	
	printMessage("DEBUG: Bootloader_Get_Rdp_Cmd\n");
	
	uint32_t command_packet_len = bootloader_rx_data[0] + 1;
	
	uint32_t hostCrc = *((uint32_t *) (bootloader_rx_data + command_packet_len - 4));
	if(!bl_verify_crc(&bootloader_rx_data[0], command_packet_len-4, hostCrc))
	{
		printMessage("DEBUG: Checksum success\n");
		bl_send_ack(1);
		rdpLevel = get_flash_rdp_level();
		printMessage("DEBUG: Device RDP Level: %#x \n", rdpLevel);
		bl_write_uart_data(&rdpLevel, 1);
	}	
	else
	{
		printMessage("DEBUG: Checksum fail \n");
		bl_send_nack();
	}
}

void bl_go_to_addr_cmd(uint8_t *bootloader_rx_data)//Jump to Address
{
	uint32_t address = 0;
	
	printMessage("DEBUG: Bootloader_Go_To_Addr_Cmd\n");
	
	uint32_t command_packet_len = bootloader_rx_data[0] + 1;
	
	uint32_t hostCrc = *((uint32_t *)(bootloader_rx_data + command_packet_len - 4));
	
	if(!bl_verify_crc(&bootloader_rx_data[0], command_packet_len-4, hostCrc))
	{
		printMessage("DEBUG: Cheksum success\n");
		bl_send_ack(1);
		
		address = *((uint32_t *)&bootloader_rx_data[2]);
		printMessage("DEBUG: Go Addr: %#x \n", address);
		
		if(bootloader_verify_address(bootloader_rx_data[2])==ADDR_VALID)
		{
			uint8_t addrValid = ADDR_VALID;
			bl_write_uart_data(&addrValid, 1);
			address += 1;//T bit
			void (*jump_address)(void) = (void*) address;
			
			printMessage("DEBUG: Jumping to Address");
			jump_address();
		}
		else
		{
			printMessage("DEBUG: Address Invalid");
		}
		
	}
	else
	{
		printMessage("DEBUG: Checksum fail \n");
		bl_send_nack();
	}
}

void bl_flash_erase_cmd(uint8_t *bootloader_rx_data)//Erase Flash Address
{
	uint8_t eraseStat = 0;
	
	printMessage("DEBUG: Bootloader_Flash_Erase_Cmd\n");
	uint32_t command_packet_len = bootloader_rx_data[0] + 1;
	
	uint32_t hostCrc = *((uint32_t *)(bootloader_rx_data + command_packet_len - 4));
	
	if(!bl_verify_crc(&bootloader_rx_data[0], command_packet_len-4, hostCrc))
	{
		printMessage("DEBUG: Cheksum success\n");
		bl_send_ack(1);
		eraseStat = flash_erase(bootloader_rx_data[2], bootloader_rx_data[3]);
		
		printMessage("DEBUG: Flash Erase Status: %d\n", eraseStat);
		
		bl_write_uart_data(&eraseStat, 1);
		
	}
	else
	{
		printMessage("DEBUG: Checksum fail \n");
		bl_send_nack();
	}
}

void bl_mem_write_cmd(uint8_t *bootloader_rx_data)
{
	
	uint8_t writeStatus = 0x00;

	
	uint8_t payLoadLen = bootloader_rx_data[6];
	
	uint32_t memAddr = *((uint32_t*)(&bootloader_rx_data[2]));
	
	
	printMessage("DEBUG: Update Command\n");
	
	uint32_t command_packet_len = bootloader_rx_data[0] + 1;
	
	uint32_t hostCrc = *((uint32_t *)(bootloader_rx_data + command_packet_len - 4));
	
	if(!bl_verify_crc(&bootloader_rx_data[0], command_packet_len-4, hostCrc))
	{
		printMessage("DEBUG: Cheksum success\n");
		bl_send_ack(1);
		
		printMessage("DEBUG: Memory Write Address: %#x", memAddr);
		
		if(bootloader_verify_address(memAddr)==ADDR_VALID)
		{
			printMessage("DEBUG: Valid Memory Write Address\n");
			writeStatus = memory_write(bootloader_rx_data, memAddr, payLoadLen);
			bl_write_uart_data(&writeStatus, 1);
		}
		else
		{
			printMessage("DEBUG: Invalid Memory Address\n");
			writeStatus = ADDR_INVALID;
			bl_write_uart_data(&writeStatus, 1);
		}
	}
	else
	{
		printMessage("DEBUG: Checksum fail \n");
		bl_send_nack();
	}
}


