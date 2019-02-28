
#include <jendefs.h>
#include <AppHardwareApi.h>

#include <mac_sap.h>
#include <mac_pib.h>
#include "JN5148_util.h"
#include "Utilities.h"

#define E_SPI_MSB_FIRST FALSE
#define E_SPI_CLOCK_2M  8 // 8*16M/64

PRIVATE uint32 u32RDYport=0;

PRIVATE uint32 au32SpiPendBuff[E_AHI_IP_MAX_MSG_SIZE];
PRIVATE uint32 au32SpiSendBuff[E_AHI_IP_MAX_MSG_SIZE];
PRIVATE uint8 u8DataPendLen=0;
PRIVATE uint8 u8DataLen=0;
PRIVATE volatile uint8 u8SentLen=0; //volatile because it's changed in interrupt, and also for avoiding removed by compiler

PRIVATE volatile bool bTxReady = TRUE;

//for wait
PRIVATE volatile uint32 dummy; 


PRIVATE void vSPItransfer32();
PRIVATE void vHandleSPIinterrupt(uint32 u32Device, uint32 u32ItemBitmap);


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

PUBLIC void SpiUtil_vInit(uint32 u32TXRDYport)
{
    vAHI_SpiConfigure(1, E_SPI_MSB_FIRST, E_AHI_SPIM_TXPOS_EDGE, E_AHI_SPIM_RXPOS_EDGE, E_SPI_CLOCK_2M, TRUE, FALSE);
    vAHI_SpiRegisterCallback(vHandleSPIinterrupt);
    vAHI_SpiSelect(0x2); // slave 1 selected

    vAHI_DioSetDirection(u32TXRDYport, 0);// DIO 1 as RDY
    vAHI_DioInterruptEnable(u32TXRDYport, 0);
    vAHI_DioInterruptEdge(0, u32TXRDYport);

    u32RDYport = u32TXRDYport;
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

PUBLIC bool SpiUtil_bWrite(uint32* pu32data, uint8 u8Len32, bool nonblock)
{
    while(u8SentLen!=0);

    memcpy(au32SpiSendBuff, pu32data, u8Len32*4);    
    vAHI_SpiWaitBusy();
    
    u8DataLen = u8Len32;

    //Wait ARM
    while(dummy<50) dummy++;
    dummy=0; 
   
    uint8 u8DioBitMap = u8AHI_DioReadByte(FALSE);

    //check if falling edge has already arrived and also need to check current DIO1 status
    while(!bTxReady || (u8DioBitMap & u32RDYport))
    {
        if(nonblock) return FALSE;
        u8DioBitMap = u8AHI_DioReadByte(FALSE);
    }

    bTxReady = FALSE;
    vSPItransfer32();

    return TRUE;
}

/****************************************************************************
 *
 * NAME: SpiUtil_bWrite128
 *
 * DESCRIPTION:
 * Always send 128 bytes;
 *
 * PARAMETERS:      Name            RW  Usage
 *                          block              R     if not ready, block or not; this has no effect yet
 *                          u8Len             R     length in BYTE; not larger than 128
 *
 * RETURNS:
 * TRUE if success
 * 
 ****************************************************************************/
PUBLIC bool SpiUtil_bWrite128(uint8* pu8data, uint8 u8Len, bool nonblock)
{
    if(u8Len > 128) u8Len = 128;
    
    while(u8SentLen!=0);
    memcpy(au32SpiSendBuff, pu8data, u8Len);
    vAHI_SpiWaitBusy();

    u8DataLen = 32;

    //Wait ARM
    while(dummy<50) dummy++;
    dummy=0;
   
    uint8 u8DioBitMap = u8AHI_DioReadByte(FALSE);

    //check if falling edge has already arrived and also need to check current DIO1 status
    while(!bTxReady || (u8DioBitMap & u32RDYport))
    {
        if(nonblock) return FALSE;
        u8DioBitMap = u8AHI_DioReadByte(FALSE);
    }

    bTxReady = FALSE;
    vSPItransfer32();

    return TRUE;
}


void vSPItransfer32()
{
    
    if(u8SentLen < u8DataLen)
    {  
        //PrintfUtil_vPrintf("T %x\n", au32SpiSendBuff[u8SentLen]);
        vAHI_SpiStartTransfer(31, au32SpiSendBuff[u8SentLen++]);        
    }
    else 
    {
        u8SentLen = 0;
        u8DataLen = 0;

        //not avaiable yet. for future development -- sending over then check pendding
        if(u8DataPendLen != 0)
        {
            memcpy(au32SpiSendBuff, au32SpiPendBuff, u8DataPendLen*4);
            u8DataLen = u8DataPendLen;
            u8DataPendLen = 0;
            u8SentLen = 0;

            vAHI_SpiStartTransfer(31, au32SpiSendBuff[u8SentLen++]);
        }
    }
}


/****************************************************************************
 *
 * NAME: vHandleSPIinterrupt
 *
 * DESCRIPTION:
 * function called when message sent/received over IP interface
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u32Device        R  always IP interface ID
 *                  u32ItemBitmap    R  contents of IP Status Reg
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleSPIinterrupt(uint32 u32Device, uint32 u32ItemBitmap)
{
    vSPItransfer32();
}


/****************************************************************************
 *
 * NAME: SpiUtil_vTxReady
 *
 * DESCRIPTION:
 * Set RDY of SPI
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 * void
 *
 * NOTE:
 * the interrupt handling function MUST be defined in APP and call this function to set ready
 *
 ****************************************************************************/
PUBLIC void SpiUtil_vTxReady()
{
    bTxReady = TRUE;
}
