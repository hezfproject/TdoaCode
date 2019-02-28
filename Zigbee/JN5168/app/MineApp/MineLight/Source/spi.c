
#include <jendefs.h>
#include <AppHardwareApi.h>

#include <mac_sap.h>
#include <mac_pib.h>
#include "JN5148_util.h"
#include "Utilities.h"
#include "spi.h"

#define E_SPI_CLOCK_2M  8 // 8*16M/64
#define E_SPI_MSB_FIRST FALSE

PRIVATE volatile bool bTxReady = TRUE;







/****************************************************************************
 *
 * NAME: SpiUtil_vInit
 *
 * DESCRIPTION:
 * Initialize the SPI and the ready IO
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u32TXRDYport         R     the IO for RDY
 *
 * RETURNS:
 * void
 *
 * NOTE:
 * CPOL and CPHA both 0;
 * Slave 1 used;
 * Falling edge as RDY interrupt trigger; the interrupt handling function MUST be defined in APP
 *
 ****************************************************************************/

PUBLIC void As3933_SpiInit(void)
{
    vAHI_SpiConfigure(0, E_SPI_MSB_FIRST, E_AHI_SPIM_TXPOS_EDGE, E_AHI_SPIM_RXNEG_EDGE, E_SPI_CLOCK_2M, FALSE, FALSE);
    //vAHI_SpiRegisterCallback(SPIReceiveinterrupt);
    vAHI_SpiSelect(1); // slave 0 selected

    //vAHI_DioSetDirection(u32TXRDYport, 0);// DIO 1 as RDY
    //vAHI_DioInterruptEnable(u32TXRDYport, 0);
    //vAHI_DioInterruptEdge(0, u32TXRDYport);

    //u32RDYport = u32TXRDYport;
}



/****************************************************************************
 *
 * NAME: SpiUtil_bWrite
 *
 * DESCRIPTION:
 * Write data to SPI
 *
 * PARAMETERS:      Name            RW  Usage
 *                          block              R     if not ready, block or not; this has no effect yet
 *                          u8Len32          R    must lest than 63
 *
 * RETURNS:
 * TRUE if success
 *
 * NOTE:
 * data unit is 4 Bytes - uint32
 *
 ****************************************************************************/

uint16 SpiReadWriteRegister(uint8 addr,uint8 data)
{
	uint16 sendvalue;
	uint16 readvalue=0;

	sendvalue=addr;
	sendvalue=(sendvalue<<8)|data;

    vAHI_SpiSelect(0);
    vAHI_SpiStartTransfer(0xf,(uint32)sendvalue);

    vAHI_SpiWaitBusy();
    readvalue=(uint16)u16AHI_SpiReadTransfer16();
    vAHI_SpiSelect(1);

    return readvalue;
}

uint8 SpiCommandRegister( uint8 data)
{
	uint8 readvalue=0;
    vAHI_SpiSelect(0);
    vAHI_SpiStartTransfer(0x7,(uint32)data);

    vAHI_SpiWaitBusy();
    readvalue=(uint8)u8AHI_SpiReadTransfer8();
    vAHI_SpiSelect(1);

    return readvalue;
}
