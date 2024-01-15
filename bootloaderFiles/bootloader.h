/**
@file: bootloader.h
@Author: rRaufToprak
@Date: 11.01.2024

*/
#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "bl_define.h"
#include "main.h"
#include "stm32f4xx_hal.h"

extern CRC_HandleTypeDef hcrc;
extern UART_HandleTypeDef huart1;
extern void printMessage(char *format, ...);
//---------------------Command Functions-------------------------------//
void bl_get_rdp_cmd(uint8_t *bootloader_rx_data);
void bl_go_to_addr_cmd(uint8_t *bootloader_rx_data);
void bl_flash_erase_cmd(uint8_t *bootloader_rx_data);
void bl_mem_write_cmd(uint8_t *bootloader_rx_data);
//---------------------Command Functions-------------------------------//





#endif /*__BOOTLOADER_H*/
