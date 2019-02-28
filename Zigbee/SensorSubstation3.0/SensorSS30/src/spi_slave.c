#include <c8051f350.h>                 // SFR declarations
#include <stdio.h>
#include <string.h>
#include "c51f350.h"
#include "type.h"
#include "spi_slave.h"

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
static volatile UINT8 g_u8RecvIndex = 0;
static volatile UINT8 g_u8SendIndex = 0;
static volatile UINT8 g_u8FindHeadIndex = 0;
static volatile BOOL  g_bRecvFlag;
static volatile BOOL  g_bFindHead;

static volatile UINT8 g_au8SPI_SendData[SPI0_BUFFER_SIZE + SPI0_HEAD_SIZE] = SPI0_HEAD;
static volatile UINT8 g_au8SPI_SendBuff[SPI0_BUFFER_SIZE];
static volatile UINT8 g_au8SPI_RecvData[SPI0_BUFFER_SIZE];
static volatile UINT8 g_au8SPI_RecvBuff[SPI0_BUFFER_SIZE];
static UINT8* const g_pu8SPI_SendData =  g_au8SPI_SendData + SPI0_HEAD_SIZE;
//-----------------------------------------------------------------------------
// PORT_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function configures the crossbar and GPIO ports.
//
// P0.0  -  SCK  (SPI0), Open-Drain, Digital
// P0.1  -  MISO (SPI0), Push-Pull,  Digital
// P0.2  -  MOSI (SPI0), Open-Drain, Digital
// P0.3  -  NSS  (SPI0), Open-Drain, Digital
//
//-----------------------------------------------------------------------------
static VOID SPI0_Port_Init (VOID)
{
    WATCHDOG_FEED();

    P0MDOUT |= 0x02;                     // Make MISO push-pull
    XBR0 |= 0x02;                        // Enable the SPI on the XBAR
    XBR1 |= 0x40;                        // Enable the XBAR and weak pull-ups
}

//-----------------------------------------------------------------------------
// SPI0_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Configures SPI0 to use 4-wire Slave mode. The SPI timing is
// configured for Mode 0,0 (data centered on first edge of clock phase and
// SCK line low in idle state).
//
//-----------------------------------------------------------------------------
VOID SPI0_Init(VOID)
{
    WATCHDOG_FEED();
    SPI0_Port_Init();
    SPI0CFG = 0x00;                     // Operate in Slave mode
    // CKPHA = '0', CKPOL = '0'
    SPI0CN = 0x05;                      // 4-wire Slave mode, SPI enabled
    g_bRecvFlag = false;
    // memcpy(g_pu8SPI_SendData, g_au8SPI_SendBuff, SPI0_BUFFER_SIZE);
    SPI0DAT = g_au8SPI_SendData[g_u8SendIndex++];
    ESPI0 = 1;                     // Enable SPI interrupts
}

BOOL SPI0_GetSpiData(UINT8 *pu8Data)
{
    UINT8 u8INT;
    BOOL bRet = false;

    WATCHDOG_FEED();

    if (g_bRecvFlag && pu8Data)
    {
        SPI_ENTRY_CRITICAL(u8INT);
        memcpy(pu8Data, g_au8SPI_RecvBuff, SPI0_BUFFER_SIZE);
        SPI_EXIT_CRITICAL(u8INT);
        g_bRecvFlag = false;
        bRet = true;
    }

    return bRet;
}

BOOL SPI0_SetSpiData(UINT8 *pu8Data)
{
    UINT8 u8INT;
    
    WATCHDOG_FEED();

    if (pu8Data)
    {
        SPI_ENTRY_CRITICAL(u8INT);
        memcpy(g_au8SPI_SendBuff, pu8Data, SPI0_BUFFER_SIZE);
        SPI_EXIT_CRITICAL(u8INT);
    }

    return pu8Data != NULL;
}
//-----------------------------------------------------------------------------
// Interrupt Service Routines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// SPI_ISR
//-----------------------------------------------------------------------------
//
// Handles all incoming data and interprets the commands sent from the Master.
//
// Typical Write:
//
//              | 1st sent | 2nd sent | 3rd sent |   ...    | last sent |
//              ---------------------------------------------------------
//  Master NSSv | Command  |   Data1  |   Data2  |   ...    |   DataN   |  NSS^
//  Slave       |   N/A    |    N/A   |    N/A   |   ...    |    N/A    |
//
// Typical Read:
//
//              | 1st sent | 2nd sent | 3rd sent |   ...    | last sent |
//              ---------------------------------------------------------
//  Master NSSv | Command  |   dummy  |   dummy  |   ...    |   dummy   |  NSS^
//  Slave       |   N/A    |   Data1  |   Data2  |   ...    |   DataN   |
//-----------------------------------------------------------------------------

VOID SPI_ISR (VOID) interrupt 6
{
#if 1
    if (WCOL)                   // Write collision occurred
    {
        SPI0DAT = -1;           // Indicate an error occurred
        WCOL = 0;               // Clear the Write collision flag
    }
    else if (RXOVRN)            // Receive overrun occurred
    {
        SPI0DAT = -1;           // Indicate an error occurred
        RXOVRN = 0;             // Clear the Receive Overrun flag
    }
    else if (MODF)
    {
        MODF = 0;
    }
#endif
/*******************************recv data******************************/
    if (g_bFindHead)        //  judge data-head
    {
        if (SPI0DAT != SPI0_HEAD[g_u8FindHeadIndex])
            g_u8FindHeadIndex = 0;
        else if (SPI0_HEAD_SIZE == ++g_u8FindHeadIndex)
            g_bFindHead = false;
    }
    else                    // recv data-command
    {
        g_au8SPI_RecvData[g_u8RecvIndex++] = SPI0DAT;

        if (SPI0_BUFFER_SIZE == g_u8RecvIndex)
        {
            g_bFindHead = g_bRecvFlag = true;
            g_u8RecvIndex = 0;
            memcpy(g_au8SPI_RecvBuff, g_au8SPI_RecvData, SPI0_BUFFER_SIZE);
        }
    }

/***************************send data************************************/
    if (TXBMT)
    {
        SPI0DAT = g_au8SPI_SendData[g_u8SendIndex++];   // ready send data
    
        if (SPI0_TRANS_LEN == g_u8SendIndex)
        {
            g_u8SendIndex = 0;
            memcpy(g_pu8SPI_SendData, g_au8SPI_SendBuff, SPI0_BUFFER_SIZE);
        }
    }

    SPIF = 0;                                   // Clear the SPIF flag
}
