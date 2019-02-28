/*******************************************************************************
  Filename:     rf.c
  Revised:        $Date: 18:21 2012Äê5ÔÂ7ÈÕ
  Revision:       $Revision: 1.0 $
  Description:  RF library

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <hal_mcu.h>

#include <hal_rf.h>
#include <rf.h>

#include <string.h>
#include <timer_event.h>
#include <mem.h>

/*******************************************************************************
* CONSTANTS AND DEFINES
*/

// Packet and packet part lengths
#define PKT_LEN_MIC                 8
#define PKT_LEN_SEC                 PKT_LEN_UNSEC + PKT_LEN_MIC
#define PKT_LEN_AUTH                8
#define PKT_LEN_ENCR                24

// Packet overhead ((frame control field, sequence number, PAN ID,
// destination and source) + (footer))
// Note that the length byte itself is not included included in the packet length
#define RF_PACKET_OVERHEAD_SIZE     ((2 + 1 + 2 + 2 + 2 + 2) + (2))
#define RF_MAX_PAYLOAD_SIZE         (127 - RF_PACKET_OVERHEAD_SIZE \
                                        - RF_AUX_HDR_LENGTH - RF_LEN_MIC)
#define RF_ACK_PACKET_SIZE          5
#define RF_FOOTER_SIZE              2
#define RF_HDR_SIZE                 12

// The time it takes for the acknowledgment packet to be received after the
// data packet has been transmitted.
#define RF_ACK_DURATION             (0.5 * 32 * 2 * ((4 + 1) + (1) + (2 + 1) + (2)))
#define RF_SYMBOL_DURATION          (32 * 0.5)

// The length byte
#define RF_PLD_LEN_MASK             0x7F

// Frame control field
#define RF_FCF_NOACK                0x8801
#define RF_FCF_ACK                  0x8821
#define RF_FCF_ACK_BM               0x0020
#define RF_FCF_BM                   (~RF_FCF_ACK_BM)
#define RF_SEC_ENABLED_FCF_BM       0x0008

// Frame control field LSB
#define RF_FCF_NOACK_L              LO_UINT16(RF_FCF_NOACK)
#define RF_FCF_ACK_L                LO_UINT16(RF_FCF_ACK)
#define RF_FCF_ACK_BM_L             LO_UINT16(RF_FCF_ACK_BM)
#define RF_FCF_BM_L                 LO_UINT16(RF_FCF_BM)
#define RF_SEC_ENABLED_FCF_BM_L     LO_UINT16(RF_SEC_ENABLED_FCF_BM)

// Auxiliary Security header
#define RF_AUX_HDR_LENGTH           5
#define RF_LEN_AUTH                 RF_PACKET_OVERHEAD_SIZE \
                                        + RF_AUX_HDR_LENGTH - RF_FOOTER_SIZE
#define RF_SECURITY_M               2
#define RF_LEN_MIC                  8

// Footer
#define RF_CRC_OK_BM                0x80

/*******************************************************************************
* TYPEDEFS
*/
// The receive struct
typedef struct
{
    UINT16          u16DstPanId;
    UINT16          u16DstAddr;
    UINT16          u16SrcPanId;
    UINT16          u16SrcAddr;
    UINT8           u8SeqNum;
    INT8            s8Length;
    UINT8*          pu8Payload;
    BOOL            bAckReq;
    INT16           s16Rssi;
    volatile BOOL   bIsReady;
} RF_RX_INFO_T;

// Tx state
typedef struct
{
    UINT8           u8TxSeqNum;
    volatile BOOL   bAckRecv;
    BOOL            bRecvOn;
    UINT16          u16FrameCnt;
} RF_TX_STATE_T;


// Basic RF packet header (IEEE 802.15.4)
typedef struct
{
    UINT8   u8PktLen;
    UINT8   u8FCF0;           // Frame control field LSB
    UINT8   u8FCF1;           // Frame control field MSB
    UINT8   u8SeqNum;
    UINT16  u16DestPanId;
    UINT16  u16DestAddr;
    UINT16  u16SrcPanId;
    UINT16  u16SrcAddr;
} RF_PKTHDR_T;

/*******************************************************************************
* LOCAL VARIABLES
*/
static RF_RX_INFO_T  s_stRxi = { 0xFF }; // Make sure sequence numbers are
static RF_TX_STATE_T s_stTxState = { 0x00 }; // initialised and distinct.

static RF_CFG_T s_stRfCfg;

static UINT8    s_u8TxMpdu[RF_MAX_PAYLOAD_SIZE+RF_PACKET_OVERHEAD_SIZE+1];
static UINT8    s_u8RxMpdu[128];

/*******************************************************************************
* GLOBAL VARIABLES
*/


/*******************************************************************************
* LOCAL FUNCTIONS
*/

/*******************************************************************************
* @fn          rf_BuildHeader
*
* @brief       Builds packet header according to IEEE 802.15.4 frame format
*
* @param       buffer - Pointer to buffer to write the header
*              destAddr - destination short address
*              payloadLength - length of higher layer payload
*
* @return      UINT8 - length of header
*/
static UINT8 rf_BuildHeader(UINT8* pu8Buffer, UINT16 u16DestAddr,
    UINT16 destpanid, UINT8 u8PayLen)
{
    RF_PKTHDR_T *pstHdr;
    UINT16 u16FCF;

    pstHdr              = (RF_PKTHDR_T*)pu8Buffer;

    // Populate packet header
    pstHdr->u8PktLen    = u8PayLen + RF_PACKET_OVERHEAD_SIZE;
    u16FCF              = s_stRfCfg.bAckReq
                        ? RF_FCF_ACK : RF_FCF_NOACK;
    pstHdr->u8FCF0      = LO_UINT16(u16FCF);
    pstHdr->u8FCF1      = HI_UINT16(u16FCF);
    pstHdr->u8SeqNum    = s_stTxState.u8TxSeqNum;
    pstHdr->u16DestPanId= destpanid;
    pstHdr->u16DestAddr = u16DestAddr;
    pstHdr->u16SrcPanId = s_stRfCfg.u16PanId;
    pstHdr->u16SrcAddr  = s_stRfCfg.u16MyAddr;

    return RF_HDR_SIZE;
}

/*******************************************************************************
* @fn          rf_BuildMpdu
*
* @brief       Builds mpdu (MAC header + payload) according to IEEE 802.15.4
*              frame format
*
* @param       destAddr - Destination short address
*              pPayload - pointer to buffer with payload
*              payloadLength - length of payload buffer
*
* @return      UINT8 - length of mpdu
*/
static UINT8 rf_BuildMpdu(UINT16 u16DestAddr, UINT16 destpanid,
    UINT8* pu8Payload, UINT8 u8PayLen)
{
    UINT8 u8HdrLen;
    UINT8 n;

    u8HdrLen = rf_BuildHeader(s_u8TxMpdu, u16DestAddr, destpanid, u8PayLen);

    for (n=0; n<u8PayLen; n++)
    {
        s_u8TxMpdu[u8HdrLen + n] = pu8Payload[n];
    }

    return u8HdrLen + u8PayLen; // total mpdu length
}

/*******************************************************************************
* @fn          basicRfRxFrmDoneIsr
*
* @brief       Interrupt service routine for received frame from radio
*              (either data or acknowlegdement)
*
* @param       s_stRxi - file scope variable info extracted from the last incoming
*                    frame
*              s_stTxState - file scope variable that keeps tx state info
*
* @return      none
*/
static VOID rf_RxFrmDoneIsr(VOID)
{
    UINT8 *pu8StatusWord;
    RF_PKTHDR_T *pstHdr;

    // Map header to packet buffer
    pstHdr = (RF_PKTHDR_T*)s_u8RxMpdu;

    // Clear interrupt and disable new RX frame done interrupt
    HAL_RF_DisableRxInterrupt();

    // Enable all other interrupt sources (enables interrupt nesting)
    HAL_ENABLE_INTERRUPTS();

    // Read payload length.
    HAL_RF_ReadRxBuf(&pstHdr->u8PktLen, 1);
    pstHdr->u8PktLen &= RF_PLD_LEN_MASK; // Ignore MSB
    
    // Is this an acknowledgment packet?
    // Only ack packets may be 5 bytes in total.
    if (RF_ACK_PACKET_SIZE == pstHdr->u8PktLen)
    {

        // Read the packet
        HAL_RF_ReadRxBuf(&s_u8RxMpdu[1], pstHdr->u8PktLen);

        s_stRxi.bAckReq = (BOOL)!!(pstHdr->u8FCF0 & RF_FCF_ACK_BM_L);

        // Read the status word and check for CRC OK
        pu8StatusWord = s_u8RxMpdu + 4;

        // Indicate the successful ACK reception if CRC and sequence number OK
        if ((pu8StatusWord[1] & RF_CRC_OK_BM)
            && (pstHdr->u8SeqNum == s_stTxState.u8TxSeqNum))
        {
            s_stTxState.bAckRecv = true;
        }

        // No, it is data
    }
    else
    {
        HAL_RF_ReadRxBuf(&s_u8RxMpdu[1], pstHdr->u8PktLen);

        // Read the address, panid
        s_stRxi.u16DstPanId = pstHdr->u16DestPanId;
        s_stRxi.u16DstAddr = pstHdr->u16DestAddr;
        s_stRxi.u16SrcPanId = pstHdr->u16SrcPanId;
        s_stRxi.u16SrcAddr = pstHdr->u16SrcAddr;

        /*
         * It is assumed that the radio rejects packets with invalid length.
         * Subtract the number of bytes in the frame overhead to get actual
         * payload.
         */
        s_stRxi.s8Length = pstHdr->u8PktLen - RF_PACKET_OVERHEAD_SIZE;

        // Read the packet payload
        s_stRxi.pu8Payload = s_u8RxMpdu + RF_HDR_SIZE;

        s_stRxi.bAckReq = (BOOL)!!(pstHdr->u8FCF0 & RF_FCF_ACK_BM_L);

        // Read the FCS to get the RSSI and CRC
        pu8StatusWord = s_stRxi.pu8Payload + s_stRxi.s8Length;
        s_stRxi.s16Rssi = pu8StatusWord[0];

        /*
         * Notify the application about the received data packet if the CRC is OK
         * Throw packet if the previous packet had the same sequence number
         */
        if ( (pu8StatusWord[1] & RF_CRC_OK_BM)
            && (s_stRxi.u8SeqNum != pstHdr->u8SeqNum) )
        {
            if ( ((pstHdr->u8FCF0 & (RF_FCF_BM_L)) == RF_FCF_NOACK_L) )
            {
                s_stRxi.bIsReady = true;
                event_timer_set(EVENT_MAC_MSG);
            }

            s_stRxi.u8SeqNum = pstHdr->u8SeqNum;
        }
    }

    // Enable RX frame done interrupt again
    HAL_DISABLE_INTERRUPTS();
    HAL_RF_EnableRxInterrupt();
}

/*******************************************************************************
* GLOBAL FUNCTIONS
*/

/*******************************************************************************
* @fn          RF_Init
*
* @brief       Initialise basic RF datastructures. Sets channel, short address and
*              PAN id in the chip and configures interrupt on packet reception
*
* @param       pRfConfig - pointer to RF_CFG_T struct.
*                          This struct must be allocated by higher layer
*              s_stTxState - file scope variable that keeps tx state info
*              s_stRxi - file scope variable info extracted from the last incoming
*              frame
* @Parame      pstRfDevCfg - pointer to RF_DEV_T struct.
*                           config gain mode and tx power
*                   if NULL that : GAIN_MODE_HIGH/HAL_RF_TXPOWER_4P5_DBM
*
* @return      none
*/
UINT8 RF_Init(RF_CFG_T* pstRfCfg, RF_DEV_T* pstRfDevCfg)
{
    HAL_RF_DEV_T *pstHalRfDevCfg = (HAL_RF_DEV_T *)pstRfDevCfg;
    HAL_RF_TXPOWER_INDEX_E emTxPowIdx;

    if (!pstRfCfg || FAILURE == HAL_RF_Init(pstHalRfDevCfg))
        return FAILURE;

    HAL_DISABLE_INTERRUPTS();

    // Set the protocol configuration
    s_stRfCfg = *pstRfCfg;
    s_stRxi.pu8Payload = NULL;

    s_stTxState.bRecvOn = false;
    s_stTxState.u16FrameCnt = 0;

    // Set channel
    HAL_RF_SetChannel(s_stRfCfg.u8Channel);

    // Write the short address and the PAN ID to the CC2520 RAM
    HAL_RF_SetShortAddr(s_stRfCfg.u16MyAddr);
    HAL_RF_SetPanId(s_stRfCfg.u16PanId);

    // Set up receive interrupt (received data or acknowlegment)
    HAL_RF_RxInterruptConfig(rf_RxFrmDoneIsr);

    if (pstHalRfDevCfg)
        emTxPowIdx = pstHalRfDevCfg->emTxPowIndex;
    else
        emTxPowIdx = HAL_RF_TXPOWER_4P5_DBM;

    HAL_RF_SetTxPower(emTxPowIdx);

    HAL_ENABLE_INTERRUPTS();

    return SUCCESS;
}

/*******************************************************************************
* @fn          RF_SendPacket
*
* @brief       Send packet
*
* @param       destAddr - destination short address
*              pPayload - pointer to payload buffer. This buffer must be
*                         allocated by higher layer.
*              length - length of payload
*              s_stTxState - file scope variable that keeps tx state info
*              mpdu - file scope variable. Buffer for the frame to send
*
* @return      basicRFStatus_t - SUCCESS or FAILURE
*/
UINT8 RF_SendPacket(UINT16 u16DestAddr, UINT16 destpanid, UINT8* pu8Payload,
    UINT8 u8Length)
{
    UINT8 u8MpduLen;
    UINT8 u8Status;

    // Turn on receiver if its not on
    if (!s_stTxState.bRecvOn)
    {
        HAL_RF_ReceiveOn();
    }

    // Check packet length
    u8Length = MIN(u8Length, RF_MAX_PAYLOAD_SIZE);

    // Wait until the transceiver is idle
    // HAL_RF_WaitTransceiverReady();

    // Turn off RX frame done interrupt to aVOID interference on the SPI interface
    HAL_RF_DisableRxInterrupt();

    u8MpduLen = rf_BuildMpdu(u16DestAddr, destpanid, pu8Payload, u8Length);

    HAL_CLOCK_STABLE();
    HAL_RF_WriteTxBuf(s_u8TxMpdu, u8MpduLen);

    // Turn on RX frame done interrupt for ACK reception
    HAL_RF_EnableRxInterrupt();

    // Send frame with CCA. return FAILURE if not successful
    if (HAL_RF_Transmit() != SUCCESS)
    {
        u8Status = FAILURE;
    }
#if 0
    // Wait for the acknowledge to be received, if any
    if (s_stRfCfg.bAckReq)
    {
        s_stTxState.bAckRecv = false;

        /*
        * We'll enter RX automatically, so just wait until we can
        * be sure that the ack reception should have finished
        * The timeout consists of a 12-symbol turnaround time,
        * the ack packet duration, and a small margin
        */
        HAL_WaitUs((12 * RF_SYMBOL_DURATION)
                + (RF_ACK_DURATION)
                + (2 * RF_SYMBOL_DURATION)
                + 10);

        /*
        * If an acknowledgment has been received (by RxFrmDoneIsr),
        * the ackReceived flag should be set
        */
        u8Status = s_stTxState.bAckRecv ? SUCCESS : FAILURE;

    }
    else
    {
        u8Status = SUCCESS;
    }


#endif
    // Turn off the receiver if it should not continue to be enabled
    if (!s_stTxState.bRecvOn)
    {
        HAL_RF_ReceiveOff();
    }

    if (SUCCESS == u8Status)
    {
        s_stTxState.u8TxSeqNum++;
    }

    return u8Status;
}

/*******************************************************************************
* @fn          RF_PacketIsReady
*
* @brief       Check if a new packet is ready to be read by next higher layer
*
* @param       none
*
* @return      UINT8 - TRUE if a packet is ready to be read by higher layer
*/
BOOL RF_PacketIsReady(VOID)
{
    return s_stRxi.bIsReady;
}

/*******************************************************************************
* @fn          RF_Receive
*
* @brief       Copies the payload of the last incoming packet into a buffer
*
* @param       pRxData - pointer to data buffer to fill. This buffer must be
*                        allocated by higher layer.
*              len - Number of bytes to read in to buffer
*              s_stRxi - file scope variable holding the information of the last
*                    incoming packet
*
* @return      UINT8 - number of bytes actually copied into buffer
*/
UINT8 RF_Receive(UINT8* pu8RxData, UINT8 u8Len, INT16* ps16Rssi)
{
    // Accessing shared variables -> this is a critical region
    // Critical region start
    UINT8 u8Critical;

    HAL_ENTER_CRITICAL_SECTION(u8Critical);
    memcpy(pu8RxData, s_stRxi.pu8Payload, MIN(s_stRxi.s8Length, u8Len));

    if (ps16Rssi != NULL)
    {
        if (s_stRxi.s16Rssi < 128)
        {
            *ps16Rssi = s_stRxi.s16Rssi - HAL_RF_GetRssiOffset();
        }
        else
        {
            *ps16Rssi = (s_stRxi.s16Rssi - 256) - HAL_RF_GetRssiOffset();
        }
    }

    s_stRxi.bIsReady = false;
    // Critical region end
    HAL_EXIT_CRITICAL_SECTION(u8Critical);

    return MIN(s_stRxi.s8Length, u8Len);
}

/*******************************************************************************
* @fn          RF_PacketIsOK
*
* @brief       Check if a new packet is ready to be read by next higher layer
*
* @param   input    u16SrcPanId - opposite panid
*                   u16SrcAddr - opposite source addr
*
* @return      UINT8 - TRUE if a packet is ready to be read by higher layer
*/
BOOL RF_PacketIsOK(UINT16 u16SrcPanId, UINT16 u16SrcAddr)
{
    if (s_stRxi.u16SrcPanId == u16SrcPanId && s_stRxi.u16SrcAddr == u16SrcAddr)
        return s_stRxi.bIsReady;
    return false;
}

/*******************************************************************************
* @fn          RF_GetRssi
*
* @brief       Copies the payload of the last incoming packet into a buffer
*
* @param       none

* @return      int8 - RSSI value
*/
INT8 RF_GetRssi(VOID)
{
    return s_stRxi.s16Rssi - (s_stRxi.s16Rssi < 128 ? 0 : 256)
                                    - HAL_RF_GetRssiOffset();
}

/*******************************************************************************
* @fn          RF_ReceiveOn
*
* @brief       Turns on receiver on radio
*
* @param       s_stTxState - file scope variable
*
* @return      none
*/
VOID RF_ReceiveOn(VOID)
{
    s_stTxState.bRecvOn = true;
    HAL_RF_ReceiveOn();
}

/*******************************************************************************
* @fn          RF_ReceiveOff
*
* @brief       Turns off receiver on radio
*
* @param       s_stTxState - file scope variable
*
* @return      none
*/
VOID RF_ReceiveOff(VOID)
{
    s_stTxState.bRecvOn = false;
    HAL_RF_ReceiveOff();
}

RF_DATA_T* RF_ReceivePkt(VOID)
{
    RF_DATA_T *pkt = NULL;
    UINT8 u8Critical;

    HAL_ENTER_CRITICAL_SECTION(u8Critical);

    if (!RF_PacketIsReady() || s_stRxi.s8Length <= 0)
        goto RET;

    pkt = rt_malloc(s_stRxi.s8Length + sizeof(RF_DATA_T));
    if (!pkt)
        goto RET;

    pkt->u16DstAddr = s_stRxi.u16DstAddr;
    pkt->u16DstPanId = s_stRxi.u16DstPanId;
    pkt->u16SrcAddr = s_stRxi.u16SrcAddr;
    pkt->u16SrcPanId = s_stRxi.u16SrcPanId;
    pkt->s16Rssi = s_stRxi.s16Rssi;
    pkt->s8Len = s_stRxi.s8Length;
    memcpy(pkt->u8Data, s_stRxi.pu8Payload, s_stRxi.s8Length);

    s_stRxi.bIsReady = false;

RET:
    // Critical region end
    HAL_EXIT_CRITICAL_SECTION(u8Critical);

    return pkt;
}

VOID RF_RevertPkt(RF_DATA_T* pkt)
{
    if (pkt)
        rt_free(pkt);
}

