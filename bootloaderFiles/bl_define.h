/**
@file: bl_define.h
@Author: rRaufToprak
@Date: 11.01.2024
*/


#ifndef __BL_DEFINE_H
#define __BL_DEFINE_H

#define SRAM1_SIZE 128*1024
#define SRAM1_END (SRAM1_BASE + SRAM1_SIZE)

#define CRC_FAIL 								0x01
#define CRC_SUCCESS 						0x00
#define BL_ACK_VALUE 						0xA5
#define BL_NACK_VALUE  					0x7F
#define ADDR_VALID 							0x00
#define ADDR_INVALID						0x01
#define INVALID_SECTOR					0x04


#define BL_GET_RDP_STATUS				0x11
#define BL_GO_TO_ADDR						0x22
#define BL_FLASH_ERASE					0x33
#define BL_MEM_WRITE						0x44
#define BL_EN_RW_PROTECT				0x55




#endif /*__BL_DEFINE_H*/
