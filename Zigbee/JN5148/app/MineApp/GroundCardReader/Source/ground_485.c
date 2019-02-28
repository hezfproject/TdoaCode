#include <jendefs.h>
#include <string.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
/*
#include <mac_sap.h>
#include <mac_pib.h>
#include <Utilities.h>
#include <OAD.h>


#include "MbusProto.h"
#include "RadioProto.h"
#include "GroundUart.h"
#include "lpbssnwk.h"
*/
#include "CRC.h"

#include "MicroSpecific.h"
#include "JN5148_util.h"
#include "ground_crsys.h"

#define GET_CRC16_H(p, len)  (*(p + len - 1))
#define GET_CRC16_L(p, len)  (*(p + len - 2))

typedef struct
{
    uint8 u8RefCnt;
    uint8 u8LoseCnt;
    uint16 u16Len;
    uint8 u8Buf[BUS485_MAX_RX_LEN];
}bus485_rxbuf_t;

typedef void (*_485_sync_fun)(UartHdl_t*, uint8);

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void _485_sync_idle(UartHdl_t *pHdl, uint8 data);
PRIVATE void _485_sync_sync1(UartHdl_t *pHdl, uint8 data);
PRIVATE void _485_sync_sync2(UartHdl_t *pHdl, uint8 data);
PRIVATE void _485_sync_cmd(UartHdl_t *pHdl, uint8 data);
PRIVATE void _485_sync_slvid(UartHdl_t *pHdl, uint8 data);
PRIVATE void _485_sync_data_len(UartHdl_t *pHdl, uint8 data);
PRIVATE void _485_sync_data(UartHdl_t *pHdl, uint8 data);

PRIVATE void _485_rx_done(uint8* p, uint16 len);
PRIVATE void _485_tx_cb(UartHdl_t* pHdl);
PRIVATE void _485_rx_cb(UartHdl_t* pHdl, uint8 data);
PRIVATE void _cmd_QRY_addr(mbus_hdr_t* pstHdr, uint16 size);
PRIVATE void _485_cmd_parser(mbus_hdr_t* phdr, uint16 size);
PRIVATE void _bus485_send_addr(void);
PRIVATE uint16 _get_cardlocbuf(uint8 * const pu8Buf, const uint16 size);
PRIVATE uint16 _get_cardinfobuf(uint8 * const pu8Buf, const uint16 size);
PRIVATE uint16 _get_cardverbuf(uint8 * const pu8Buf, const uint16 size);
PRIVATE void _bus485_send_data(void);

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE UartHdl_t s_stBus485;

PRIVATE uint8 s_u8Bus485RxBuf[BUS485_MAX_RX_LEN];

PRIVATE uint8 s_u8Bus485TxBuf[BUS485_MAX_TX_LEN];

const _485_sync_fun _485_sync_state_machine[BUS485_SYNC_KIND] = {
    [BUS485_SYNC_IDLE] = _485_sync_idle,
    [BUS485_SYNC_SYNC1] = _485_sync_sync1,
    [BUS485_SYNC_SYNC2] = _485_sync_sync2,
    [BUS485_SYNC_CMD] = _485_sync_cmd,
    [BUS485_SYNC_SLVID_H] = _485_sync_slvid,
    [BUS485_SYNC_SLVID_L] = _485_sync_slvid,
    [BUS485_SYNC_DATALEN_H] = _485_sync_data_len,
    [BUS485_SYNC_DATALEN_L] = _485_sync_data_len,
    [BUS485_SYNC_DATA] = _485_sync_data
};

PRIVATE uint16 u16SeqNum;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC bus485_rxbuf_t g_stBus485RxBuf;

/****************************************************************************/
/***        Local Function                                                ***/
/****************************************************************************/
PRIVATE void _485_sync_idle(UartHdl_t *pHdl, uint8 data)
{
    pHdl->rx_pos = 0;

    if (data != 'Y')
    {
        return;
    }

    pHdl->sync_state = BUS485_SYNC_SYNC1;
    pHdl->rxBuf[pHdl->rx_pos++] = data;
}

PRIVATE void _485_sync_sync1(UartHdl_t *pHdl, uint8 data)
{
    if (data == 'I')
    {
        pHdl->sync_state = BUS485_SYNC_SYNC2;
        pHdl->rxBuf[pHdl->rx_pos++] = data;
    }
    else if(data == 'Y')
    {
        pHdl->rx_pos = 0;
        pHdl->sync_state = BUS485_SYNC_SYNC1;
        pHdl->rxBuf[pHdl->rx_pos++] = data;
    }
    else
    {
        pHdl->sync_state = BUS485_SYNC_IDLE;
    }
}

PRIVATE void _485_sync_sync2(UartHdl_t *pHdl, uint8 data)
{
    if(data == 'R')
    {
        pHdl->sync_state = BUS485_SYNC_CMD;
        pHdl->rxBuf[pHdl->rx_pos++] = data;
    }
    else if(data == 'Y')
    {
        pHdl->rx_pos = 0;
        pHdl->sync_state = BUS485_SYNC_SYNC1;
        pHdl->rxBuf[pHdl->rx_pos++] = data;
    }
    else
    {
        pHdl->sync_state = BUS485_SYNC_IDLE;
    }
}

PRIVATE void _485_sync_cmd(UartHdl_t *pHdl, uint8 data)
{
    pHdl->sync_state = BUS485_SYNC_SLVID_H;
    pHdl->rxBuf[pHdl->rx_pos++] = data;
}

PRIVATE void _485_sync_slvid(UartHdl_t *pHdl, uint8 data)
{
    if (pHdl->sync_state != BUS485_SYNC_SLVID_L)
        pHdl->sync_state = BUS485_SYNC_SLVID_L;
    else
        pHdl->sync_state = BUS485_SYNC_DATALEN_H;

    pHdl->rxBuf[pHdl->rx_pos++] = data;
}

PRIVATE void _485_sync_data_len(UartHdl_t *pHdl, uint8 data)
{
    pHdl->rxBuf[pHdl->rx_pos++] = data;

    if (pHdl->sync_state != BUS485_SYNC_DATALEN_L)
        pHdl->sync_state = BUS485_SYNC_DATALEN_L;
    else
    {   /* conver want data len */
        mbus_hdr_t *pHeader = (mbus_hdr_t *)(pHdl->rxBuf);

        pHdl->rx_len = pHeader->data_len;
        SysUtil_vConvertEndian(&(pHdl->rx_len), sizeof(pHdl->rx_len));
        pHdl->rx_len += sizeof(mbus_fdr_t); // add for fooder
        pHdl->rx_len += pHdl->rx_pos;       // total packet length

        if (pHdl->rx_len <= BUS485_MAX_RX_LEN && pHdl->rx_len > pHdl->rx_pos)
        {
            pHdl->sync_state = BUS485_SYNC_DATA;
        }
        else
        {
            pHdl->sync_state = BUS485_SYNC_IDLE;
        }
    }
}

PRIVATE void _485_sync_data(UartHdl_t *pHdl, uint8 data)
{
    pHdl->rxBuf[pHdl->rx_pos++] = data;

    if (pHdl->rx_len == pHdl->rx_pos)
    {
        _485_rx_done(pHdl->rxBuf,  pHdl->rx_len);

        pHdl->sync_state = BUS485_SYNC_IDLE;
    }
}

PRIVATE inline void _485_received(uint8 *p, uint16 len)
{
    g_stBus485RxBuf.u8LoseCnt = 0;
    g_stBus485RxBuf.u16Len = len;
    memcpy(g_stBus485RxBuf.u8Buf, p, len);
    EventUtil_vSetEvent(BUS485_PARSER_EVENT);
    DBG_LOG("recv 485 a packet\n");
}

/****************************************************************************
 *
 * NAME: vUartTxCallBack
 * DESCRIPTION:
 * OUTPUT:
 * INPUT:
 ****************************************************************************/
PRIVATE void _485_tx_done(void)
{
    // must delay 1ms after send
    //JN5148 finished before  485
    DBG_LOG("TX Success\n");
}

/****************************************************************************
 *
 * NAME: vUartRxCallBack
 * DESCRIPTION:
 * OUTPUT:
 * INPUT:  uint8* p, uint16 len
 ****************************************************************************/
PRIVATE void _485_rx_done(uint8* p, uint16 len)
{
    mbus_hdr_t* phdr = (mbus_hdr_t*) p;
    uint16 slvID;

    DBG_JUDGE_RET(p == NULL, "rx uart buf is NULL\n");
    DBG_JUDGE_RET((len < (sizeof(mbus_hdr_t) + sizeof(mbus_fdr_t))), "rx uart "
                "buf length = %d\n", len);

    slvID = phdr->slv_id;

    CONVERT_ENDIAN(slvID);

    DBG_JUDGE_RET((slvID != g_stSysMgr.u16SysAddr) && (slvID != 0xFFFF), "rx uart"
                " packet not me:%d\n", slvID);

    if((len <= BUS485_MAX_RX_LEN) && (g_stBus485RxBuf.u8RefCnt == 0))
    {
        _485_received(p, len);
    }
    else
    {
        ++g_stBus485RxBuf.u8LoseCnt;

        DBG_LOG("rx lose a packet!\n");

        DBG_JUDGE(len > BUS485_MAX_RX_LEN, "len = %d  too long!\n",
                len);
    }

    if (g_stBus485RxBuf.u8LoseCnt > BUS485_MAX_LOSE)
    {
        DBG_WARN("lose: %d, system restart\n", g_stBus485RxBuf.u8LoseCnt);
        vAHI_SwReset();
    }
}

PRIVATE void _485_tx_cb(UartHdl_t* pHdl)
{
    _485_tx_done();

    vAHI_UartSetInterrupt(E_AHI_UART_1, FALSE, FALSE, FALSE, FALSE,
                        E_AHI_UART_FIFO_LEVEL_8);

    pHdl->work_state = RX;
    UART1_SWITCH_RX();

    /* empty the datas in my recv fifo*/
    vAHI_UartReset(pHdl->uart_port, TRUE, TRUE);

    // go to rx
    vAHI_UartSetInterrupt(E_AHI_UART_1, FALSE, FALSE, FALSE, TRUE,
                        E_AHI_UART_FIFO_LEVEL_8);

}

PRIVATE void _485_rx_cb(UartHdl_t* pHdl, uint8 data)
{
    DBG_JUDGE_RET(pHdl == NULL, "rx uart handel is NULL\n");
    _485_sync_state_machine[pHdl->sync_state](pHdl, data);
}

PRIVATE void _cmd_QRY_addr(mbus_hdr_t* pstHdr, uint16 size)
{
    bool reportFlag = TRUE;

    if (pstHdr->data_len && (!(pstHdr->data_len & 1)))
    {
        uint16 i;
        uint16* pu16Addr = (uint16*)(&pstHdr->data_len + 1);

        size -= sizeof(mbus_hdr_t);
        size >>= 1;
        for (i=0; i<size; i++)
        {
            // if arm have own addr,return;
            SysUtil_vConvertEndian(pu16Addr + i, sizeof(uint16));

            if (*(pu16Addr + i) == g_stSysMgr.u16SysAddr)
            {
                reportFlag = FALSE;
                break;
            }
        }
    }

    if (reportFlag)
    {
        uint32 u16BackOffTime;

        vAHI_StartRandomNumberGenerator(E_AHI_RND_SINGLE_SHOT, E_AHI_INTS_DISABLED);
        u16BackOffTime = u16AHI_ReadRandomNumber();
        u16BackOffTime %= uart_getaddrbackoff();

        if (u16BackOffTime < _10ms) u16BackOffTime += _10ms;

        TimerUtil_eSetTimer(BUS485_SEND_ADDR_EVENT, u16BackOffTime);
    }
}

#if 0
PRIVATE void _485_cmd_transmit(uint8 const * const buf)
{
    static uint8 u8Transmit[BUS485_MAX_RX_LEN];
    mbus_hdr_t* phdr = (mbus_hdr_t*)(buf);
    mbus_tlv_t* ptlv = (mbus_tlv_t*)(phdr + 1);
    LPBSS_nwk_header_t* pnwk = (LPBSS_nwk_header_t*)(ptlv + 1);
    LPBSS_Msg_Header_t* pmsghdr = (LPBSS_Msg_Header_t*)(pnwk + 1);
    uint16 u16Len;

    ASSERT_RET(buf != NULL);

    switch (phdr->cmd)
    {
#if 0
    case MBUS_CMD_ADDR_QRY:
        pnwk = (LPBSS_nwk_header_t*)(u8Transmit);
        pnwk->u16SrcAddr = g_stSysMgr.u16SysAddr;
        pnwk->u16DstAddr = 0xFFFF;
        pnwk->u8SrcDevType = READSTATION_DEVICE_ID;
        pnwk->u8DstDevType = READSTATION_DEVICE_ID;
        pnwk->u8Model = g_stSysMgr.u8SysModel;
        pnwk->u8Padding1 = 0;
        pnwk->u16Padding2 = 0;
        pnwk->u16Len = sizeof(LPBSS_Msg_Header_t);
        pmsghdr = (LPBSS_Msg_Header_t*)(pnwk + 1);
        pmsghdr->u8Msg = LPBSS_JMP_TOPO;
        pmsghdr->u8Padding = 0;
        pmsghdr->u16len = 0;
        CONVERT_ENDIAN(pnwk->u16SrcAddr);
        CONVERT_ENDIAN(pnwk->u16DstAddr);
        CONVERT_ENDIAN(pnwk->u16Len);
        break;
#endif
    case MBUS_CMD_SEARCH_CARD:
    case MBUS_CMD_SET_INFO:
    case MBUS_CMD_DATA:
        CONVERT_ENDIAN(ptlv->len);
        ASSERT_RET(ptlv->len < JMP_BUFFER_LEN - sizeof(RADIO_JMP_HDR_T));
        memcpy(u8Transmit, pnwk, ptlv->len);
        break;
    default:
        return;
    }

    jmp_transmit_down(pnwk);
}
#endif

PRIVATE bool _485_private_cmd(mbus_hdr_t* phdr, uint16 size)
{
    switch(phdr->cmd)
    {
    case MBUS_CMD_CLR:
        break;

    case MBUS_CMD_QRY:
        g_stSysMgr.u8CmdType = MBUS_CMD_QRY;
        TimerUtil_eSetTimer(BUS485_SEND_DATA_EVENT, _15ms);
        break;

    case MBUS_CMD_ADDR_QRY:
        g_stSysMgr.u8CmdType = MBUS_CMD_ADDR_QRY;
        _cmd_QRY_addr(phdr, size);
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

PRIVATE bool _485_data_cmd(mbus_hdr_t* phdr, uint16 size)
{
    mbus_tlv_t* ptlv = (mbus_tlv_t*)(phdr + 1);
    LPBSS_nwk_header_t* pnwk = (LPBSS_nwk_header_t*)(ptlv + 1);
    LPBSS_Msg_Header_t* pmsghdr = (LPBSS_Msg_Header_t*)(pnwk + 1);
    uint16 u16SrcAddr;
    uint16 u16DstAddr;

    if (size < sizeof(mbus_hdr_t)
                + sizeof(mbus_tlv_t)
                + sizeof(LPBSS_nwk_header_t)
                + sizeof(LPBSS_Msg_Header_t))
    {
        return FALSE;
    }

    size -= sizeof(mbus_hdr_t) + sizeof(mbus_tlv_t);

    u16DstAddr = pnwk->u16DstAddr;
    u16SrcAddr = pnwk->u16SrcAddr;
    CONVERT_ENDIAN(u16SrcAddr);
    CONVERT_ENDIAN(u16DstAddr);

    if (u16DstAddr != NWK_BROADCAST_ADDR
        && u16DstAddr != g_stSysMgr.u16SysAddr)
    {
        return FALSE;
    }

    switch (phdr->cmd)
    {
    case MBUS_CMD_SEARCH_CARD:
        size -= sizeof(LPBSS_nwk_header_t);
        app_cmd_search(pmsghdr, size);
        break;

    case MBUS_CMD_SET_INFO:
        size -= sizeof(LPBSS_nwk_header_t);
        app_cmd_setinfo(pmsghdr, size);
        break;

    case MBUS_CMD_DATA:
#ifdef USE_JMP_NET
        jmp_transmit_down(pnwk, size);
#endif
        size -= sizeof(LPBSS_nwk_header_t);
        app_packet_parser(pmsghdr, size);
        break;

    default:
        DBG_WARN("Unkown BUS_CMD: %d\n", phdr->cmd);
        return FALSE;
    }

    return TRUE;
}

PRIVATE void _485_cmd_parser(mbus_hdr_t* phdr, uint16 size)
{
    DBG_LOG("recv cmd %d\n", phdr->cmd);

    if (!_485_private_cmd(phdr, size))
    {
        _485_data_cmd(phdr, size);
    }
}

PRIVATE void _bus485_send_addr(void)
{
    uint16 total_len;
    mbus_hdr_t* phdr = (mbus_hdr_t*)s_u8Bus485TxBuf;
    mbus_tlv_t* ptlv = (mbus_tlv_t*)(phdr + 1);
    mbus_fdr_t* pfdr = (mbus_fdr_t*)(ptlv + 1);

    ptlv->len = 0;
    ptlv->type = MBUS_TLV_CARD_READER;
    ptlv->model = g_stSysMgr.u8SysModel;
    phdr->sync[0] = 'Y';
    phdr->sync[1] = 'I';
    phdr->sync[2] = 'R';
    phdr->cmd = MBUS_CMD_ADDR_RSP;
    phdr->slv_id = g_stSysMgr.u16SysAddr;
    phdr->data_len = ptlv->len + sizeof(mbus_tlv_t);

    pfdr->padding = u16SeqNum;

    total_len = sizeof(mbus_hdr_t) + sizeof(mbus_tlv_t) + sizeof(mbus_fdr_t);

    CONVERT_ENDIAN(phdr->slv_id);
    CONVERT_ENDIAN(phdr->data_len);
    CONVERT_ENDIAN(pfdr->padding);
    CONVERT_ENDIAN(ptlv->len);

    pfdr->crc = CRC16(s_u8Bus485TxBuf, total_len - 2, 0xFFFF);
    CONVERT_ENDIAN(pfdr->crc);

    if (BUS485_SUCCESS != bus485_tx_start(s_u8Bus485TxBuf, total_len))
    {
        DBG_WARN("Err: Uart Send AddrRsp Failed\n");
    }
}

PRIVATE uint16 _get_cardlocbuf(uint8 * const pu8Buf, const uint16 size)
{
    LPBSS_Msg_Header_t* pstHdr;
    uint8* psrcdata = g_st485LocBuf.u8CardLocBuf;
    uint16 dataLen = g_st485LocBuf.u16CardLocDataLen;
    uint16 u16BufLen = sizeof(LPBSS_Msg_Header_t) + dataLen;

    if (0 == dataLen || size < u16BufLen)
        return 0;

    pstHdr = (LPBSS_Msg_Header_t*)pu8Buf;
    pstHdr->u8Msg = LPBSS_CARD_LOC_DATA;
    pstHdr->u8Padding = 0;
    pstHdr->u16len = dataLen;

    memcpy((uint8*)(pstHdr + 1), psrcdata, pstHdr->u16len);

    CONVERT_ENDIAN(pstHdr->u16len);

    return u16BufLen;
}

PRIVATE uint16 _get_cardinfobuf(uint8 * const pu8Buf, const uint16 size)
{
    LPBSS_Msg_Header_t* pstHdr;
    uint8* psrcdata = g_st485InfoBuf.u8CardInfoBuf;
    uint16 dataLen = g_st485InfoBuf.u16CardInfoLen;
    uint16 u16BufLen = sizeof(LPBSS_Msg_Header_t) + dataLen;

    if (0 == dataLen || size < u16BufLen)
        return 0;

    pstHdr = (LPBSS_Msg_Header_t*)pu8Buf;
    pstHdr->u8Msg = LPBSS_DEV_INFO;
    pstHdr->u8Padding = 0;
    pstHdr->u16len = dataLen;

    memcpy((uint8*)(pstHdr + 1), psrcdata, pstHdr->u16len);

    CONVERT_ENDIAN(pstHdr->u16len);

    return u16BufLen;
}

PRIVATE uint16 _get_cardverbuf(uint8 * const pu8Buf, const uint16 size)
{
    LPBSS_Msg_Header_t* pstHdr;
    uint8* psrcdata = g_st485VerBuf.u8CardVerBuf;
    uint16 dataLen = g_st485VerBuf.u16CardVerLen;
    uint16 u16BufLen = sizeof(LPBSS_Msg_Header_t) + dataLen;

    if (0 == dataLen || size < u16BufLen)
        return 0;

    pstHdr = (LPBSS_Msg_Header_t*)pu8Buf;
    pstHdr->u8Msg = LPBSS_CARD_VERSION;
    pstHdr->u8Padding = 0;
    pstHdr->u16len = dataLen;

    memcpy((uint8*)(pstHdr + 1), psrcdata, pstHdr->u16len);

    CONVERT_ENDIAN(pstHdr->u16len);

    return u16BufLen;
}

/****************************************************************************
 *
 * NAME: vSendCardData
 * DESCRIPTION: 将无线上收到的数据打包后发到UART
 * OUTPUT: NONE
 * INPUT:  pRadio,  u16CardAddr,  rssi
 ****************************************************************************/
PRIVATE void _bus485_send_data(void)
{
    /* the data is mbus_hdr_t +LPBSS_NWK_HEADER_T + u8CardNum*mbus_card_data_t
        + mbus_fdr_t */
    static uint8 TxErrCount = 0;
    uint8 u8SendState;

    uint8* p;
    uint16 total_len;
    uint16 new_size;
    int32 free_size;
    uint16 crc;
    mbus_hdr_t* phdr = (mbus_hdr_t*)s_u8Bus485TxBuf;
    mbus_tlv_t* ptlv = (mbus_tlv_t*)(phdr + 1);
    LPBSS_nwk_header_t* pnwk = (LPBSS_nwk_header_t*)(ptlv + 1);
    mbus_fdr_t* pfdr;

    phdr->data_len = 0;
    ptlv->len = 0;

    total_len = sizeof(mbus_hdr_t) + sizeof(mbus_tlv_t)
                + sizeof(LPBSS_nwk_header_t);
    new_size = total_len;
    free_size = sizeof(s_u8Bus485TxBuf) - total_len - sizeof(mbus_fdr_t);

    pnwk->u16Len = 0;
    pnwk->u16SrcAddr = g_stSysMgr.u16SysAddr;
    pnwk->u16DstAddr = 0;
    pnwk->u8SrcDevType = READSTATION_DEVICE_ID;
    pnwk->u8DstDevType = STATION_CONTROL_DEVICE_ID;
    pnwk->u8Model = g_stSysMgr.u8SysModel;

    p = s_u8Bus485TxBuf + total_len;
    new_size = _get_cardlocbuf(p, free_size);
    pnwk->u16Len += new_size;
    total_len += new_size;
    free_size -= new_size;

    p = s_u8Bus485TxBuf + total_len;
    new_size = _get_cardinfobuf(p, free_size);
    pnwk->u16Len += new_size;
    total_len += new_size;
    free_size -= new_size;

    p = s_u8Bus485TxBuf + total_len;
    new_size = _get_cardverbuf(p, free_size);
    pnwk->u16Len += new_size;
    total_len += new_size;
    free_size -= new_size;

    p = s_u8Bus485TxBuf + total_len;
    new_size = app_get_sysver(p, free_size);
    pnwk->u16Len += new_size;
    total_len += new_size;
    free_size -= new_size;

#ifdef USE_JMP_NET
    DBG_LOG("cardnum:%d,\tinfbytenum:%d,\tverbytenum:%d\t"
                "jmpbytenum=%d\n",
                g_st485LocBuf.u16CardLocDataLen,
                g_st485InfoBuf.u16CardInfoLen,
                g_st485VerBuf.u16CardVerLen,
                g_u16485Pos);

    if (free_size >= g_u16485Pos)
    {
        p = s_u8Bus485TxBuf + total_len;
        memcpy(p, g_au8485Poll, g_u16485Pos);
        total_len += g_u16485Pos;
        ptlv->len += g_u16485Pos;
        free_size -= g_u16485Pos;
    }
    g_u16485Pos = 0;
#else
    DBG_LOG("cardnum:%d,\tinfbytenum:%d,\tverbytenum:%d\n",
            g_st485LocBuf.u16CardLocDataLen,
            g_st485InfoBuf.u16CardInfoLen,
            g_st485VerBuf.u16CardVerLen);
#endif

    ptlv->type = MBUS_TLV_CARD_READER;
    ptlv->model = g_stSysMgr.u8SysModel;
    ptlv->len += pnwk->u16Len + sizeof(LPBSS_nwk_header_t);

    phdr->sync[0] = 'Y';
    phdr->sync[1] = 'I';
    phdr->sync[2] = 'R';

    phdr->cmd = MBUS_CMD_RSP;
    phdr->slv_id = g_stSysMgr.u16SysAddr;
    phdr->data_len += ptlv->len + sizeof(mbus_tlv_t);

    p = s_u8Bus485TxBuf + total_len;
    pfdr = (mbus_fdr_t*)p;
    pfdr->padding = u16SeqNum;
    total_len += sizeof(mbus_fdr_t);

    CONVERT_ENDIAN(phdr->slv_id);
    CONVERT_ENDIAN(phdr->data_len);
    CONVERT_ENDIAN(pnwk->u16SrcAddr);
    CONVERT_ENDIAN(pnwk->u16DstAddr);
    CONVERT_ENDIAN(pnwk->u16Len);
    CONVERT_ENDIAN(ptlv->len);
    CONVERT_ENDIAN(pfdr->padding);
    crc = CRC16(s_u8Bus485TxBuf, total_len - 2, 0xFFFF);
    pfdr->crc = crc;
    CONVERT_ENDIAN(pfdr->crc);

    u8SendState = bus485_tx_start(s_u8Bus485TxBuf, total_len);
//#define BUS485_DUMP
#ifdef BUS485_DUMP
{
    uint16 idx;
    DBG_LOG("485 send %d bytes, seqnum %d:\n", total_len, u16SeqNum);

    for (idx=0; idx<total_len; idx++)
    {
        if ((idx & 7) == 0)
            DBG_LOG("\n");
        else
            DBG_LOG(" ");

        DBG_LOG("%x", s_u8Bus485TxBuf[idx]);
    }
    DBG_LOG("\n");
}
#endif
    if (BUS485_SUCCESS != u8SendState)
    {
        DBG_ERR("Err: 485 Send Data Failed\n");

        if (TxErrCount++ > BUS485_MAX_LOSE)
            vAHI_SwReset();
    }
    else
    {
        g_st485LocBuf.u16CardLocDataLen = 0;
        g_st485InfoBuf.u16CardInfoLen = 0;
        g_st485VerBuf.u16CardVerLen = 0;
        TxErrCount = 0;
        DBG_LOG("send data success\n");
    }
}

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void bus485_init(void)
{
    s_stBus485.baud_rate = UART_BAUDRATE_115200;
    s_stBus485.uart_port = E_AHI_UART_1;

    s_stBus485.rxBuf = s_u8Bus485RxBuf;
    s_stBus485.rx_pos = 0;
    s_stBus485.txBuf = NULL;
    s_stBus485.tx_pos = 0;

    s_stBus485.work_state = RX;

    s_stBus485.tx_callback = _485_tx_cb;
    s_stBus485.rx_callback = _485_rx_cb;

    /* initial uart */
    uart_init(&s_stBus485);
}

PUBLIC uint32 bus485_event_process(uint32 u32Event)
{
    if (BUS485_PARSER_EVENT & u32Event)
    {
        bus485_rx_parser();
        EventUtil_vUnsetEvent(BUS485_PARSER_EVENT);
    }

    if (BUS485_SEND_ADDR_EVENT & u32Event)
    {
        _bus485_send_addr();
        EventUtil_vUnsetEvent(BUS485_SEND_ADDR_EVENT);
    }

    if (BUS485_SEND_DATA_EVENT & u32Event)
    {
        /* pingpang operation, send old buffer to uart,
            use other buffer for air receive */
        _bus485_send_data();
        EventUtil_vUnsetEvent(BUS485_SEND_DATA_EVENT);
    }

    return u32Event;
}

PUBLIC bool_t bus485_is_tx(void)
{
    return uart_is_tx(&s_stBus485);
}

PUBLIC void bus485_setbuaudrate(uint8 u8BaudRate)
{
    s_stBus485.work_state = RX;
    s_stBus485.sync_state = BUS485_SYNC_IDLE;
    s_stBus485.rx_pos = 0;

    uart_setbaudrate(&s_stBus485, u8BaudRate);
}

PUBLIC uint8 bus485_tx_start(uint8 *p, uint16 len)
{
    DBG_JUDGE_RETV((p == NULL || len == 0 || len > BUS485_MAX_TX_LEN),
                    BUS485_INVALIDPARAM, "BUS485_INVALIDPARAM: %d\n", len);

    if (s_stBus485.work_state != RX)
    {
        DBG_WARN("BUS485_BUSY\n");
        return BUS485_BUSY;
    }

    s_stBus485.txBuf = p;
    s_stBus485.tx_max = len;
    s_stBus485.tx_pos = 0;

    uart_tx_start(&s_stBus485);

    return BUS485_SUCCESS;
}

PUBLIC void bus485_rx_parser(void)
{
    uint16 len = g_stBus485RxBuf.u16Len;
    uint8* p = g_stBus485RxBuf.u8Buf;
    mbus_hdr_t* phdr = (mbus_hdr_t*)p;
    uint16 crc;
    uint16 crc_recv;

    g_stBus485RxBuf.u8RefCnt++;

    crc = CRC16(p, len - 2, 0xFFFF);
    crc_recv = (GET_CRC16_H(p, len) << 8) | GET_CRC16_L(p, len);

    if(crc != crc_recv)
    {
        DBG_WARN("485 CRC err: %x %x\n", crc, crc_recv);
        g_stBus485RxBuf.u8RefCnt--;
        //PrintfUtil_vPrintMem(p,len);
        return;
    }

    g_stSysMgr.u8LoseLinkCnt = 0;

    if(g_stSysMgr.u8StationState != E_STATE_STARTED)
    {
        /* if received a command,  sync end */
        g_stSysMgr.u8StationState = E_STATE_STARTED;
#ifdef USE_JMP_NET
        jmp_type_change(NET_TYPE_485);
#endif
    }

    u16SeqNum = (*(p + len - 3) << 8) | *(p + len - 4);

    CONVERT_ENDIAN(phdr->slv_id);
    CONVERT_ENDIAN(phdr->data_len);

    if ((phdr->slv_id == g_stSysMgr.u16SysAddr) || (phdr->slv_id == 0xFFFF))
    {
        app_led_ontimer(LED_LINK, 100);
        _485_cmd_parser(phdr, len - sizeof(mbus_fdr_t));
    }

    g_stBus485RxBuf.u8RefCnt--;
}

