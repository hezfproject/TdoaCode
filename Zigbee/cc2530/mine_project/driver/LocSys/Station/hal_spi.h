/**************************************************************************************************
  Filename:       hal_spi.h
  Description:     This file contains the interface to config spi for the DMA Service.
**************************************************************************************************/

#ifndef HAL_SPI_H
#define HAL_SPI_H

#include "ZComDef.h"

#ifdef __cplusplus
extern "C"
{
#endif
/*********************************************************************
 * INCLUDES
 */
#include "hal_board.h"


typedef struct
{
    uint16 bufSize;
    uint8   taskID;
    uint16 eventID;
}halSPICfg_t;

//////////////////////////////////////////////////////
#define HAL_SPI_0_PERCFG_BIT     0x01  // SPI0 on P0, so clear this bit.
#define HAL_SPI_0_P0_MI_MO       0x0c  // Peripheral I/O Select for MI/MO.
#define HAL_SPI_0_P0_SSN         0x10  // Peripheral I/O Select for SSN.
#define HAL_SPI_0_P0_SCK         0x20  // Peripheral I/O Select for SCK.

#define HAL_SPI_1_PERCFG_BIT     0x02  // SPI1 on P1, so set this bit.
#define HAL_SPI_1_P1_SCK         0x20  // Peripheral I/O Select for SCK.
#define HAL_SPI_1_P1_SSN         0x10  // Peripheral I/O Select for SSN.
#define HAL_SPI_1_P1_MI_MO       0xC0  // Peripheral I/O Select for MI/MO.

#define HAL_DMA_CH_TX    3
#define HAL_DMA_CH_RX    4
#define DMA_RXBUF   (0x7000 + 0xF9) //0xDFF9
#define DMA_TXBUF   (0x7000 + 0xC1) //0xDFC1

// UxCSR - USART Control and Status Register.
#define CSR_MODE      0x80
#define CSR_SPI      0x00
#define CSR_RE        0x40
#define CSR_SLAVE     0x20
#define CSR_FE        0x10
#define CSR_ERR       0x08
#define CSR_RX_BYTE   0x04
#define CSR_TX_BYTE   0x02
#define CSR_ACTIVE    0x01

// UxGCR - USART SPI Generic Control Register.
#define GCR_POSITIVE 0x80
#define GCR_MSB 0x20
#define GCR_BE 0x10

// UxBAUD - USART UART Baud Rate Control Register.
#define BAUD_BM 0x0

/* SPI Status */
#define  HAL_SPI_SUCCESS        			0x00
#define  HAL_SPI_UNCONFIGURED   		0x01
#define  HAL_SPI_NOT_SUPPORTED  		0x02
#define  HAL_SPI_MEM_FAIL       			0x03
#define  HAL_SPI_BAUDRATE_ERROR 		0x04
#define  HAL_SPI_EMPTY_QUEUE      		0x05
#define  HAL_SPI_FULL_QUEUE      			0x06
#define  HAL_SPI_ON_TX      				0x07
#define  HAL_SPI_ON_RX     				0x08
#define  HAL_SPI_HANDLINE_NOREADY      	0x09

void HalSpiInit(void);

void HalSpiStart(halSPICfg_t* config);

/*
 * Read a buffer from the SPI
 */
extern  uint8 HalSPIRead(uint8 *buf, uint16* plen);

/*
 * Write a buff to the SPI
 */
extern  uint8 HalSPIWrite (uint8 *pBuffer, uint16 length );

/*
 *Poll SPI.
 */
extern __near_func void HalSPIPoll(void);


#ifdef __cplusplus
}
#endif

#endif
