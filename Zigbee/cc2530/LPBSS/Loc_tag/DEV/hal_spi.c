#include "hal_mcu.h"
#include "hal_spi.h"
#include <types.h>
#include <string.h>
#include <mem.h>

#define BAUD_M 216
#define BAUD_E 11
/****************************************************************
*函数功能 ：串初始化SPI相关寄存器        			*
*入口参数 : 无           					*   
*返 回 值 ：无						        *
*说    明 ：							*
****************************************************************/
/*---------------------------------------------------------------- 
    Master                 Slave 
-------------          ------------- 
|           |          |           | 
|P1_4   SSN |--------->|SSN    P1_4| 
|           |          |           | 
|P1_5   SCK |--------->|SCK    P1_5| 
|           |          |           | 
|P1_6   MOSI|--------->|MOSI   P1_6| 
|           |          |           | 
|P1_7   MISO|<---------|MISO   P1_7| 
|           |          |           | 
-------------          ------------- 
------------------------------------------------------------------*/ 

void SPI_CC_Init(void) 
{
	// Set baud rate (system clock frequency / 8)  115200
	U1CSR = 0; 
	U1GCR = BAUD_E;                 //BAUD_E
	U1BAUD = BAUD_M;                //BAUD_M
	//USART 1 Baud-Rate Control /SPI master SCK clock frequency. 
	
	U1GCR |= 0x20; 				  // ORDER = 1   MSB first  
	U1GCR &= ~0x80;                 // MSB CPOL  = 0 
	U1GCR |= 0x40;                  // MSB CPHA  = 1
//	U1GCR &= ~0x40;                 // MSB CPHA  = 0
	
	PERCFG |= 0x02;                 // ap USART1 to its alternative 2 location. 
	P1SEL |= 0xF0;                  // P1_7, P1_6, and P1_5 are peripherals 
	P1SEL &= ~0x10;                 // P1_4 is GPIO (SSN)
	P1DIR |= 0x10;                  // SSN is set as output
	U1CSR &= ~0xA0;                  // SPI Master Mode 
	
	
} 

/****************************************************************
*函数功能 ：读取寄存器结果 				*
*入口参数 : Opcode:八位操作码   						*
*返 回 值 ：rxBufferMaster[3]：寄存器值			        *
*说    明 ：							*
****************************************************************/
 void SPI_Read_Register(UINT8 data)
{ 
	SSN=HIGH;
	SSN=LOW;  
	asm("NOP");
	                
	while(!(U1CSR&0x1));
        U1CSR &=0xFD;
	U1DBUF =data;   
	
	SSN = HIGH; 
	asm("NOP"); 
 
}


/****************************************************************
*函数功能 ：通过SPI写寄存器的八位操作码  		 	*
*入口参数 : Opcode:八位操作码                                   *
*返 回 值 ：无                           		        *
*说    明 ：							*
****************************************************************/
 void SPI_Write_Data(UINT8 data)
{ 
	SSN=HIGH;
	SSN=LOW;
	P1_4 =0x0;
	asm("NOP");
	asm("NOP");
	U1DBUF=data; 
	while (!U1TX_BYTE);
	U1TX_BYTE = 0; 
//	while(!(U1CSR&0x2));
//	U1CSR &=0xFD;
  
	SSN=HIGH;
}

void SpiReadWriteRegister(UINT8 addr,UINT8 data)
{
//	SPI_Write_Data(addr);
//	SPI_Write_Data(data);

	
	SSN=LOW; 
	SSN=HIGH;
	asm("NOP");
	
	U1DBUF =addr;                   //发送读数操作码,0xB0读测量结果，0xB3读reg0 
	while (!U1TX_BYTE);
	U1TX_BYTE = 0; 
	asm("NOP");
        
	U1DBUF = data; 
	while (!U1TX_BYTE);
	U1TX_BYTE = 0;   
//	SSN = HIGH; 
	SSN=LOW;
	asm("NOP"); 
  
}
