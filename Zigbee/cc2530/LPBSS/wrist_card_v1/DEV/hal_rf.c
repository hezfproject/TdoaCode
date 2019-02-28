/*******************************************************************************
  Filename:       hal_rf.c
  Revised:        $Date: 18:21 2012Äê5ÔÂ7ÈÕ
  Revision:       $Revision: 1.0 $

  Description:    CC2530 radio interface.

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <hal_mcu.h>
#include <hal_rf.h>
#include <string.h>

/*******************************************************************************
* CONSTANTS AND DEFINES
*/

// Chip revision
#define REV_A                       0x01
#define CHIPREVISION                REV_A

// CC2530 RSSI Offset
#define RSSI_OFFSET                 73
#define RSSI_OFFSET_LNA_HIGHGAIN    79
#define RSSI_OFFSET_LNA_LOWGAIN     67

// Various radio settings
#define AUTO_ACK                    0x20
#define AUTO_CRC                    0x40

// TXPOWER values
#define CC2530_TXPOWER_4P5_DBM      0xF5
#define CC2530_TXPOWER_2P5_DBM      0xE5
#define CC2530_TXPOWER_1_DBM        0xD5
#define CC2530_TXPOWER_M0P5_DBM     0xC5
#define CC2530_TXPOWER_M1P5_DBM     0xB5
#define CC2530_TXPOWER_M3_DBM       0xA5
#define CC2530_TXPOWER_M6_DBM       0x85
#define CC2530_TXPOWER_M18_DBM      0x25

// RF interrupt flags
#define IRQ_TXDONE                 0x02
#define IRQ_RXPKTDONE              0x40

// Selected strobes
#define ISRXON()                st(RFST = 0xE3;)
#define ISTXON()                st(RFST = 0xE9;)
#define ISTXONCCA()             st(RFST = 0xEA;)
#define ISRFOFF()               st(RFST = 0xEF;)
#define ISFLUSHRX()             st(RFST = 0xED;RFST = 0xED;)
#define ISFLUSHTX()             st(RFST = 0xEE;)

// CC2590-CC2591 support
#ifdef USE_CC2591

// Support for PA/LNA
#define HAL_PA_LNA_INIT()

// Select CC2591 RX high gain mode
#define HAL_PA_LNA_RX_HGM() st(UINT8 i; P0_7 = 1; for(i=0; i<8; i++) asm("NOP");)

// Select CC2591 RX low gain mode
#define HAL_PA_LNA_RX_LGM() st(UINT8 i; P0_7 = 0; for(i=0; i<8; i++) asm("NOP");)

#else // dummy macros when not using CC2591

#define HAL_PA_LNA_INIT()
#define HAL_PA_LNA_RX_LGM()
#define HAL_PA_LNA_RX_HGM()

#endif

/*******************************************************************************
* GLOBAL DATA
*/

/*******************************************************************************
* LOCAL DATA
*/
static ISR_PFN s_pfnISR_RF = NULL;
static UINT8 s_u8RssiOffset = RSSI_OFFSET;
static const UINT8 s_cu8TxDBM[] = {
    CC2530_TXPOWER_4P5_DBM,
    CC2530_TXPOWER_2P5_DBM,
    CC2530_TXPOWER_1_DBM,
    CC2530_TXPOWER_M0P5_DBM,
    CC2530_TXPOWER_M1P5_DBM,
    CC2530_TXPOWER_M3_DBM,
    CC2530_TXPOWER_M6_DBM,
    CC2530_TXPOWER_M18_DBM
};

/*******************************************************************************
* LOCAL FUNCTIONS
*/
#ifdef USE_CC2591
static VOID hal_PaLnaInit(HAL_RF_GAIN_MODE_E emGainMode);
#endif

/*******************************************************************************
* GLOBAL FUNCTIONS
*/

/*******************************************************************************
* @fn      HAL_RF_Init
*
* @brief   Power up, sets default tuning settings, enables autoack, enables random
*          generator.
*
* @param   none
*
* @return  SUCCESS always (for interface compatibility)
*/
UINT8 HAL_RF_Init(HAL_RF_DEV_T* pstRfDevCfg)
{
    // Enable auto ack and auto crc
    FRMCTRL0 |= (AUTO_ACK | AUTO_CRC);

    // Recommended RX settings
    TXFILTCFG = 0x09;
    AGCCTRL1 = 0x15;
    FSCAL1 = 0x00;

    // Enable random generator -> Not implemented yet

    // Enable CC2591 with High Gain Mode
#ifdef USE_CC2591
    if (pstRfDevCfg)
        hal_PaLnaInit(pstRfDevCfg->emGainMode);
#endif

    // Enable RX interrupt
    HAL_RF_EnableRxInterrupt();

    return SUCCESS;
}

/*******************************************************************************
* @fn      HAL_RF_GetChipId
*
* @brief   Get chip id
*
* @param   none
*
* @return  UINT8 - result
*/
UINT8 HAL_RF_GetChipId(VOID)
{
    return CHIPID;
}

/*******************************************************************************
* @fn      HAL_RF_GetChipVer
*
* @brief   Get chip version
*
* @param   none
*
* @return  UINT8 - result
*/
UINT8 HAL_RF_GetChipVer(VOID)
{
    // return major revision (4 upper bits)
    return (CHVER >> 4);
}

/*******************************************************************************
* @fn      HAL_RF_GetRandomByte
*
* @brief   Return random byte
*
* @param   none
*
* @return  UINT8 - random byte
*/
UINT8 HAL_RF_GetRandomByte(VOID)
{
    // Not yet implemented
    // HAL_ASSERT(false);
    return 0;
}


/*******************************************************************************
* @fn      HAL_RF_GetRssiOffset
*
* @brief   Return RSSI Offset
*
* @param   none
*
* @return  UINT8 - RSSI offset
*/
UINT8 HAL_RF_GetRssiOffset(VOID)
{
    return s_u8RssiOffset;
}

/*******************************************************************************
* @fn      HAL_RF_SetChannel
*
* @brief   Set RF channel in the 2.4GHz band. The Channel must be in the range 11-26,
*          11= 2005 MHz, channel spacing 5 MHz.
*
* @param   channel - logical channel number
*
* @return  none
*/
VOID HAL_RF_SetChannel(UINT8 u8Channel)
{
    FREQCTRL = (MIN_CHANNEL + (u8Channel - MIN_CHANNEL) * CHANNEL_SPACING);
}

/*******************************************************************************
* @fn      HAL_RF_SetShortAddr
*
* @brief   Write short address to chip
*
* @param   none
*
* @return  none
*/
VOID HAL_RF_SetShortAddr(UINT16 u16ShortAddr)
{
    SHORT_ADDR0 = LO_UINT16(u16ShortAddr);
    SHORT_ADDR1 = HI_UINT16(u16ShortAddr);
}

/*******************************************************************************
* @fn      HAL_RF_SetPanId
*
* @brief   Write PAN Id to chip
*
* @param   none
*
* @return  none
*/
VOID HAL_RF_SetPanId(UINT16 u16PanId)
{
    PAN_ID0 = LO_UINT16(u16PanId);
    PAN_ID1 = HI_UINT16(u16PanId);
}

/*******************************************************************************
* @fn      HAL_RF_SetTxPower
*
* @brief   Set TX output power
*
* @param   UINT8 power - power level: TXPOWER_MIN_4_DBM, TXPOWER_0_DBM,
*                        TXPOWER_4_DBM
*
* @return  UINT8 - SUCCESS or FAILED
*/
UINT8 HAL_RF_SetTxPower(HAL_RF_TXPOWER_INDEX_E emIndex)
{
    if (emIndex > HAL_RF_TXPOWER_M18_DBM
        /*|| emIndex < HAL_RF_TXPOWER_4P5_DBM*/
        )
    {
        return FAILURE;
    }
    // Set TX power
    TXPOWER = s_cu8TxDBM[emIndex];

    return SUCCESS;
}

/*******************************************************************************
* @fn      HAL_RF_SetGain
*
* @brief   Set gain mode - only applicable for units with CC2590/91.
*
* @param   UINT8 - gain mode
*
* @return  none
*/
VOID HAL_RF_SetGain(HAL_RF_GAIN_MODE_E emGainMode)
{
    if (HAL_RF_GAIN_LOW == emGainMode)
    {
        HAL_PA_LNA_RX_LGM();
        s_u8RssiOffset = RSSI_OFFSET_LNA_LOWGAIN;
    }
    else
    {
        HAL_PA_LNA_RX_HGM();
        s_u8RssiOffset = RSSI_OFFSET_LNA_HIGHGAIN;
    }
}

/*******************************************************************************
* @fn      HAL_RF_WriteTxBuf
*
* @brief   Write to TX buffer
*
* @param   UINT8* pData - buffer to write
*          UINT8 length - number of bytes
*
* @return  none
*/
VOID HAL_RF_WriteTxBuf(UINT8* pu8Data, UINT8 u8Length)
{
    UINT8 i;

    ISFLUSHTX();          // Making sure that the TX FIFO is empty.

    RFIRQF1 = ~IRQ_TXDONE;   // Clear TX done interrupt

    // Insert data
    for (i=0; i<u8Length; i++)
    {
        RFD = pu8Data[i];
    }
}

/*******************************************************************************
* @fn      HAL_RF_AppendTxBuf
*
* @brief   Write to TX buffer
*
* @param   UINT8* pData - buffer to write
*          UINT8 length - number of bytes
*
* @return  none
*/
VOID HAL_RF_AppendTxBuf(UINT8* pu8Data, UINT8 u8Length)
{
    UINT8 i;

    // Insert data
    for(i=0; i<u8Length; i++)
    {
        RFD = pu8Data[i];
    }
}

/*******************************************************************************
* @fn      HAL_RF_ReadRxBuf
*
* @brief   Read RX buffer
*
* @param   UINT8* pData - data buffer. This must be allocated by caller.
*          UINT8 length - number of bytes
*
* @return  none
*/
VOID HAL_RF_ReadRxBuf(UINT8* pu8Data, UINT8 u8Length)
{
    // Read data
    while (u8Length > 0)
    {
        *pu8Data++ = RFD;
        u8Length--;
    }
}

/*******************************************************************************
* @fn      HAL_RF_ReadMemory
*
* @brief   Read RF device memory
*
* @param   UINT16 addr - memory address
*          UINT8* pData - data buffer. This must be allocated by caller.
*          UINT8 length - number of bytes
*
* @return  Number of bytes read
*/
UINT8 HAL_RF_ReadMemory(UINT16 u16Addr, UINT8* pu8Data, UINT8 u8Length)
{
    return 0;
}

/*******************************************************************************
* @fn      HAL_RF_WriteMemory
*
* @brief   Write RF device memory
*
* @param   UINT16 addr - memory address
*          UINT8* pData - data buffer. This must be allocated by caller.
*          UINT8 length - number of bytes
*
* @return  Number of bytes written
*/
UINT8 HAL_RF_WriteMemory(UINT16 u16Addr, UINT8* pu8Data, UINT8 u8Length)
{
    return 0;
}

/*******************************************************************************
* @fn      HAL_RF_Transmit
*
* @brief   Transmit frame with Clear Channel Assessment.
*
* @param   none
*
* @return  UINT8 - SUCCESS or FAILED
*/
UINT8 HAL_RF_Transmit(VOID)
{
    UINT8 u8Stat;

    ISTXON(); // Sending

    // Waiting for transmission to finish
    while(!(RFIRQF1 & IRQ_TXDONE));

    //P1_1 = 0;
    //P1_4 = 1;

    RFIRQF1 = ~IRQ_TXDONE;
    u8Stat = SUCCESS;

    return u8Stat;
}


/*******************************************************************************
* @fn      HAL_RF_ReceiveOn
*
* @brief   Turn receiver on
*
* @param   none
*
* @return  none
*/
VOID HAL_RF_ReceiveOn(VOID)
{
    ISFLUSHRX();     // Making sure that the TX FIFO is empty.
    ISRXON();
}

/*******************************************************************************
* @fn      HAL_RF_ReceiveOff
*
* @brief   Turn receiver off
*
* @param   none
*
* @return  none
*/
VOID HAL_RF_ReceiveOff(VOID)
{
    ISRFOFF();
    ISFLUSHRX();    // Making sure that the TX FIFO is empty.
}

/*******************************************************************************
* @fn      HAL_RF_DisableRxInterrupt
*
* @brief   Clear and disable RX interrupt.
*
* @param   none
*
* @return  none
*/
VOID HAL_RF_DisableRxInterrupt(VOID)
{
    // disable RXPKTDONE interrupt
    RFIRQM0 &= ~BV(6);
    // disable general RF interrupts
    IEN2 &= ~BV(0);
}

/*******************************************************************************
* @fn      HAL_RF_EnableRxInterrupt
*
* @brief   Enable RX interrupt.
*
* @param   none
*
* @return  none
*/
VOID HAL_RF_EnableRxInterrupt(VOID)
{
    // enable RXPKTDONE interrupt
    RFIRQM0 |= BV(6);
    // enable general RF interrupts
    IEN2 |= BV(0);
}

/*******************************************************************************
* @fn      HAL_RF_RxInterruptConfig
*
* @brief   Configure RX interrupt.
*
* @param   none
*
* @return  none
*/
VOID HAL_RF_RxInterruptConfig(ISR_PFN pfn)
{
    HAL_CRITICAL_STATEMENT(s_pfnISR_RF = pfn);
}

/*******************************************************************************
* @fn      HAL_RF_WaitTransceiverReady
*
* @brief   Wait until the transciever is ready (SFD inactive).
*
* @param   none
*
* @return  none
*/
VOID HAL_RF_WaitTransceiverReady(VOID)
{
    // Wait for SFD not active and TX_Active not active
    while (FSMSTAT1 & (BV(1) | BV(5)));
}

/*******************************************************************************
* @fn          rfIsr
*
* @brief       Interrupt service routine that handles RFPKTDONE interrupt.
*
* @param       none
*
* @return      none
*/
HAL_ISR_FUNCTION(rfIsr, RF_VECTOR)
{
    UINT8 x;

    HAL_ENTER_CRITICAL_SECTION(x);

    if ( RFIRQF0 & IRQ_RXPKTDONE )
    {
        if (s_pfnISR_RF)
        {
            (*s_pfnISR_RF)();                 // Execute the custom ISR
        }

        S1CON = 0;                   // Clear general RF interrupt flag
        RFIRQF0 &= ~IRQ_RXPKTDONE;   // Clear RXPKTDONE interrupt
    }

    HAL_EXIT_CRITICAL_SECTION(x);
}

/*******************************************************************************
* LOCAL FUNCTIONS
*/
/*******************************************************************************
* LOCAL FUNCTIONS
*/
#ifdef USE_CC2591
static VOID hal_PaLnaInit(HAL_RF_GAIN_MODE_E emGainMode)
{

    // Initialize CC2591 to RX high gain mode
        AGCCTRL1  = 0x15;
        FSCAL1 = 0x0;
        RFC_OBS_CTRL0 = 0x68;
        RFC_OBS_CTRL1 = 0x6A;
        OBSSEL1 = 0xFB;
        OBSSEL4 = 0xFC;
        P0DIR |= 0x80;
        HAL_RF_SetGain(emGainMode);

}
#endif
