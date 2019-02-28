//#include <types.h>
//#include <hal_mcu.h>
#include "cs5460a.h"
#include <jendefs.h>
#include <AppHardwareApi.h>


#include <mac_sap.h>
#include <mac_pib.h>
#include "JN5148_util.h"
#include "Utilities.h"
#include "spi.h"

#define E_SPI_CLOCK_2M  8 // 8*16M/64
#define E_SPI_MSB_FIRST FALSE


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



