#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <Utilities.h>

#include "JN5148_util.h"
#include "MbusProto.h"
#include "RadioProto.h"
#include "lpbssnwk.h"

#include "CRC.h"
#include "ground_crsys.h"
#include "ground_jmp.h"

#define JMP_DIREC_UP    (0)
#define JMP_DIREC_DOWN  (1)

#define JMP_CHILD_MAX   (2)
#define JMP_SCAN_MAX    (8)

#define JMP_FAULT_MAX   (3)

#define JMP_INVALID_ADDR    (0xFFFF)

#define JMP_HI_FREQ         (5 * 1000)                  // 5 sec
#define JMP_JOIN_CN_FREQ    (10 * 60 * 1000)
#define JMP_JOIN_RQ_FREQ    (1000)

#define JMP_LINK_UNSTABLE   (-65)
#define JMP_LINK_TIMEOUT    (30 * 1000)                 // 30sec

#define JMP_BYE_TIMEOUT     (10 * 1000)
#define JMP_BYE_FREQ        (1000)                      // 1 sec

#define JMP_SCAN_TIMEOUT    (10 * 1000)
#define JMP_JOIN_TIMEOUT    (5 * 1000)                 // 5sec

#define JMP_TOPO_FREQ       (2*60*1000)

#define JMP_VER_FREQ        (1*60*1000)

#define STRUCT_ENTRY(type, field_addr)   \
    (type *)((uint8 *)(field_addr) - (uint32)(&(((type*)0)->pstCurPoll)))

uint16 jmp_seqnum;
uint32 hi_tick;
bool join_start;
uint32 join_start_time;
uint32 send_tick;
uint32 send_topo_tick;
uint32 send_ver_tick;
uint32 bye_tick, bye_start;
uint32 rq_last_time = 0;
uint32 scan_start_time;
bool scan_ok;

typedef struct
{
    uint16 src_addr;
    uint16 root_addr;
    uint8 weight;
    uint8 level;
    uint8 link_fault;
    int8 rssi;
    uint32 last_time;
} JMP_NODE_T;

typedef struct
{
    JMP_NODE_T node;
    uint8 size;
    uint8 *data_buf;
} JMP_PKT_T;

typedef struct
{
    uint8 net_state;
    uint8 net_type;
    uint8 scan_num;
    uint8 ask_idx;
    JMP_NODE_T self;
    JMP_NODE_T child_list[JMP_CHILD_MAX];
    JMP_NODE_T scan_list[JMP_SCAN_MAX];
} JMP_MGR_T;

static void _jmp_state_t_x_sorry(JMP_PKT_T* jmp);
static void _jmp_state_r_x_join(JMP_PKT_T* jmp);

JMP_MGR_T jmpmgr;

INLINE uint8 _node_find(JMP_NODE_T *node, JMP_NODE_T *node_list, uint8 num_list)
{
    uint8 idx;

    for (idx=0; idx<num_list; idx++)
    {
        if (node_list[idx].src_addr == node->src_addr)
        {
            break;
        }
    }

    return idx;
}

static void _node_del(JMP_NODE_T *node, JMP_NODE_T *node_list, uint8 *num_list)
{
    if (*num_list > 0)
    {
        uint8 idx;

        idx = _node_find(node, node_list, *num_list);

        for (idx+=1; idx<*num_list; idx++)
        {
            node_list[idx-1] = node_list[idx];
        }

        (*num_list)--;
    }
}

static void _node_timeout_del(JMP_NODE_T *node_list, uint8 *num_list)
{
    uint8 i, j;
    uint32 cur_time = get_system_ms();

    for (i=0; i<*num_list; i++)
    {
        if (cur_time - node_list[i].last_time > JMP_LINK_TIMEOUT)
        {
            for (j=i+1; j<*num_list; j++)
            {
                node_list[j-1] = node_list[j];
            }

            (*num_list)--;
            i--;
        }
    }
}

// find current scanlist rootnode
static uint8 _find_good_scan(void)
{
    uint8 i;
    uint8 u8CurLevel, u8NewLevel;
    int8 s8CurRssi, s8NewRssi;
    uint8 good_idx;
    uint8 have_stable = 0;

    _node_timeout_del(jmpmgr.scan_list, &jmpmgr.scan_num);

    if (!jmpmgr.scan_num)
        return jmpmgr.scan_num;
    else
        good_idx = jmpmgr.scan_num;

    if (NET_STATE_CN == jmpmgr.net_state)
    {
        s8CurRssi = jmpmgr.self.rssi;
        have_stable = (JMP_LINK_UNSTABLE < s8CurRssi);
        u8CurLevel = jmpmgr.self.level - 1;
    }
    else
    {
        s8CurRssi = -128;
        have_stable = 0;
        u8CurLevel = 0xFF;
    }

    for (i=0; i<jmpmgr.scan_num; i++)
    {
        if (jmpmgr.scan_list[i].src_addr == jmpmgr.self.root_addr)
            continue;

        s8NewRssi = jmpmgr.scan_list[i].rssi;
        u8NewLevel = jmpmgr.scan_list[i].level;

        DBG_LOG("current value = %i, new node value = %i\n", s8CurRssi, s8NewRssi);

        if (have_stable)
        {
            if (u8NewLevel < u8CurLevel)
            {
                u8CurLevel = u8NewLevel;
                s8CurRssi = s8NewRssi;
                good_idx = i;
            }

            continue;
        }

        if (s8NewRssi <= s8CurRssi)
            continue;

        u8CurLevel = u8NewLevel;
        s8CurRssi = s8NewRssi;
        good_idx = i;

        if (JMP_LINK_UNSTABLE < s8NewRssi)
        {
            have_stable = 1;
        }
    }

    return good_idx;
}

INLINE void _scan_start(uint32 cur_tick)
{
    jmpmgr.ask_idx = -1;
    scan_ok = FALSE;
    scan_start_time = cur_tick + JMP_SCAN_TIMEOUT;
}

INLINE void _scan_end(void)
{
    scan_ok = TRUE;
}

INLINE void _request_start(uint32 cur_tick)
{
    join_start_time = cur_tick;
    rq_last_time = cur_tick + JMP_JOIN_TIMEOUT;
    jmpmgr.self.root_addr = JMP_INVALID_ADDR;
}

INLINE uint8 _request_next(void)
{
    return _find_good_scan();
}

INLINE void _scan_add(JMP_NODE_T *node, JMP_NODE_T *node_list, uint8 *num_list)
{
    if (*num_list < JMP_SCAN_MAX)
    {
        node_list[*num_list] = *node;
        (*num_list)++;
    }
}

INLINE uint8 _rssi_refresh(JMP_NODE_T *node, JMP_NODE_T *node_list, uint8 num_list)
{
    uint8 idx;
    int16 rssi;

    idx = _node_find(node, node_list, num_list);

    if (idx != num_list)
    {
        rssi = (node->rssi + (int16)2 * node_list[idx].rssi) / 3;
        node_list[idx] = *node;
        node_list[idx].rssi = rssi;
    }

    return idx;
}
static void _node_refresh(JMP_PKT_T *jmp)
{
    JMP_NODE_T *node_list;
    uint8 num_list;
    int16 rssi;

    if (NET_STATE_CN == jmpmgr.net_state)
    {
        if (jmp->node.src_addr == jmpmgr.self.root_addr)
        {
            rssi = (jmp->node.rssi + (int16)2 * jmpmgr.self.rssi) / 3;
            jmpmgr.self.rssi = rssi;
            jmpmgr.self.level = jmp->node.level + 1;
            jmpmgr.self.last_time = jmp->node.last_time;
            return;
        }
        else if (jmp->node.root_addr == jmpmgr.self.src_addr)
        {
            num_list = jmpmgr.self.weight;
            node_list = jmpmgr.child_list;

            if (_rssi_refresh(&jmp->node, node_list, num_list) == num_list)
            {
                _jmp_state_r_x_join(jmp);
            }
            return;
        }
    }

    if (jmp->node.weight >= JMP_CHILD_MAX)
        return;

    num_list = jmpmgr.scan_num;
    node_list = jmpmgr.scan_list;

    if (_rssi_refresh(&jmp->node, node_list, num_list) >= num_list)
    {
        _scan_add(&jmp->node, node_list, &jmpmgr.scan_num);
    }
}

static bool _child_join(JMP_NODE_T* node)
{
    uint8 idx;

    if (NET_STATE_CN != jmpmgr.net_state)
    {
        return FALSE;
    }

    idx = _node_find(node, jmpmgr.child_list, jmpmgr.self.weight);

    if (idx >= JMP_CHILD_MAX)
       return FALSE;

    jmpmgr.child_list[idx].src_addr = node->src_addr;
    jmpmgr.child_list[idx].last_time = node->last_time;
    jmpmgr.self.weight++;

    return TRUE;
}

static bool _data_filter(JMP_PKT_T* jmp)
{
    RADIO_JMP_HDR_T *pstJmpHdr = (RADIO_JMP_HDR_T *)(jmp->data_buf);

    if (jmp->size <= JMP_PLD_OFFSET)
        return FALSE;

    switch (pstJmpHdr->u8Direc)
    {
    case JMP_DIREC_DOWN:
        if (jmp->node.src_addr != jmpmgr.self.root_addr)
        {
            return FALSE;
        }

        break;

    case JMP_DIREC_UP:
        if (_node_find(&jmp->node, jmpmgr.child_list, jmpmgr.self.weight)
            == jmpmgr.self.weight)
        {
            return FALSE;
        }
        else if (0 == pstJmpHdr->u8Ttl)
        {
            _jmp_state_t_x_sorry(jmp);
            return FALSE;
        }

        break;

    default:
        return FALSE;
    }

    return TRUE;
}

typedef struct
{
    uint8 hold;
    uint8 pos;
    uint8 r_idx;
    uint8 w_idx;
    uint8 size[JMP_BUFFER_NUM];
    uint8 buf[JMP_BUFFER_NUM][JMP_BUFFER_LEN];
} JMP_BUF_T;

JMP_BUF_T jmpbuf={0,20,0,0};
//JMP_BUF_T jmpbuf;

static void _jmpbuf_fill_hdr(uint8 *buf, uint8 tail_size)
{
    RADIO_JMP_HDR_T *pstJmpHdr = (RADIO_JMP_HDR_T*)buf;
    LPBSS_nwk_header_t *pstNwk = (LPBSS_nwk_header_t*)(pstJmpHdr + 1);

    pstJmpHdr->u8PktType = RADIO_JMP_PT;
    pstJmpHdr->u8MsgType = JMP_MT_DATA;
    pstJmpHdr->u16SeqNum = jmp_seqnum++;
    pstJmpHdr->u8Direc = JMP_DIREC_UP;
    pstJmpHdr->u8Ttl = 0xFF;
    pstJmpHdr->u16Len = tail_size + sizeof(LPBSS_nwk_header_t);

    pstNwk->u16SrcAddr = g_stSysMgr.u16SysAddr;
    pstNwk->u16DstAddr = 0x0000;
    pstNwk->u8SrcDevType = READSTATION_DEVICE_ID;
    pstNwk->u8DstDevType = ANALYSIS_DEVICE_ID;
    pstNwk->u8Model = g_stSysMgr.u8SysModel;
    pstNwk->u8Padding1 = 0;
    pstNwk->u16Padding2 = 0;
    pstNwk->u16Len = tail_size;

    CONVERT_ENDIAN(pstNwk->u16SrcAddr);
    CONVERT_ENDIAN(pstNwk->u16Len);
}

static void _jmpbuf_w_adjust(void)
{
    uint8 w_idx;
    uint8 r_idx;

    w_idx = jmpbuf.w_idx;
    r_idx = jmpbuf.r_idx;

    w_idx++;

    if (w_idx == JMP_BUFFER_NUM)
    {
        w_idx = 0;
    }

    if (w_idx == r_idx)
    {
        DBG_WARN("lose a packed\n");
        r_idx++;

        if (r_idx == JMP_BUFFER_NUM)
            r_idx = 0;
    }

    jmpbuf.r_idx = r_idx;
    jmpbuf.w_idx = w_idx;
    jmpbuf.hold = 0;
    jmpbuf.pos = JMP_PLD_OFFSET;
}

static void _next_jmpbuf(void)
{
	uint16 u16Crcsum;

    if (jmpbuf.hold)
    {
    	u16Crcsum=CRC16(jmpbuf.buf[jmpbuf.w_idx] + JMP_PLD_OFFSET, jmpbuf.pos - JMP_PLD_OFFSET,0xffff);
		memcpy(jmpbuf.buf[jmpbuf.w_idx]+jmpbuf.pos, (uint8 *)&u16Crcsum, 2);

        _jmpbuf_fill_hdr(jmpbuf.buf[jmpbuf.w_idx], jmpbuf.pos - JMP_PLD_OFFSET);
        jmpbuf.size[jmpbuf.w_idx] = jmpbuf.pos+2;
		//jmpbuf.size[jmpbuf.w_idx] = jmpbuf.pos;
        _jmpbuf_w_adjust();
    }
}

static void _transit_packed(uint8 *buf, uint8 size)
{
    DBG_JUDGE_RET(size > JMP_BUFFER_LEN, "size error:%d\n", size);

    _next_jmpbuf();

    jmpbuf.size[jmpbuf.w_idx] = size;
    memcpy(jmpbuf.buf[jmpbuf.w_idx], buf, size);

    _jmpbuf_w_adjust();
}

// 跳传数据包回复
void jmp_tx(const uint8 u8CmdType, void* buf, const uint8 u8Len,
            const uint16 u16DstAddr, const uint16 u16DstPanId,
            const uint8 u8TxOptions)
{
    MacUtil_SendParams_s sParams;
    RADIO_JMP_HDR_T RfHeadType;
    uint8 *pu8SendBuffer;
    uint8 u8SendLen;

    g_stSysMgr.bMsgSending = TRUE;

    sParams.u8Radius = 1;
    sParams.u16DstAddr = u16DstAddr;
    sParams.u16DstPanId = u16DstPanId;
    sParams.u16ClusterId = 0;
    sParams.u8DeliverMode = MAC_UTIL_UNICAST;

    RfHeadType.u8PktType = RADIO_JMP_PT;
    RfHeadType.u8MsgType = u8CmdType;
    RfHeadType.u8Direc = JMP_DIREC_UP;
    RfHeadType.u8Ttl = 1;
    RfHeadType.u16Len = 0;
    pu8SendBuffer = (uint8 *)(&RfHeadType);
    u8SendLen = sizeof(RADIO_JMP_HDR_T);

    switch (u8CmdType)
    {
        case JMP_MT_HI:// 发送网络通知包，带有节点信息
        case JMP_MT_DATA:
            u8SendLen = u8Len;
            pu8SendBuffer = (uint8 *)buf;
            break;
        case JMP_MT_TOPO:
             RfHeadType.u8Direc = JMP_DIREC_DOWN;
        case JMP_MT_SORRY:
        case JMP_MT_JOIN:
        case JMP_MT_OK:
        case JMP_MT_BYE:
            RfHeadType.u16SeqNum = jmp_seqnum++;
            break;

        default:
            return;
    }

    g_stSysMgr.u8LastHandle = MacUtil_vSendData(&sParams, pu8SendBuffer,
                                                u8SendLen, u8TxOptions);

}

typedef void (*PKT_PARSER)(JMP_PKT_T*);

/*******************************************************************************
*
*   state machine function
*
*******************************************************************************/
// 当前状态下发送一个SORRY包
static void _jmp_state_t_x_sorry(JMP_PKT_T* jmp)
{
    jmp_tx(JMP_MT_SORRY, NULL, 0, jmp->node.src_addr, READ_STATION_PANID, 1);
}

// 当前状态下侦听到的HI包
static void _jmp_state_r_x_hi(JMP_PKT_T* jmp)
{
    RADIO_JMP_HDR_T *pstJmpHdr;
    JMP_MSG_HI_T *pstMsgHi;

    pstJmpHdr = (RADIO_JMP_HDR_T *)(jmp->data_buf);
    pstMsgHi = (JMP_MSG_HI_T *)(pstJmpHdr + 1);

    if (sizeof(JMP_MSG_HI_T) + sizeof(RADIO_JMP_HDR_T) > jmp->size
        || sizeof(JMP_MSG_HI_T) > pstJmpHdr->u16Len)
    {
        DBG_WARN("recv srcaddr = %d, HI packet error, sudlen = %d, u16len = %d\n",
            jmp->node.src_addr, jmp->size, pstJmpHdr->u16Len);
        return;
    }

    DBG_LOG("jmp rx HI of %d\n", jmp->node.src_addr);

//    jmp.node.src_addr = psFrame->sSrcAddr.uAddr.u16Short;
    jmp->node.root_addr = pstMsgHi->u16RootAddr;
    jmp->node.weight = pstMsgHi->u8Weight;
    jmp->node.level = pstMsgHi->u8Level;
//    jmp.node.link_fault = 0;
//    jmp.node.rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
//    jmp.node.last_time = get_system_ms();
//    jmp.size = 0;
//    jmp.data_buf = NULL;

    // 扫描更新
    _node_refresh(jmp);

    if (!scan_ok && (JMP_SCAN_MAX == jmpmgr.scan_num
        || get_system_ms() - scan_start_time < TIMER_TICK_MAX))
    {
        _scan_end();
    }
}

// 当前状态下有节点申请加入
static void _jmp_state_r_x_join(JMP_PKT_T* jmp)
{
    if (!_child_join(&jmp->node))
    {
        DBG_LOG("%d joined sorry\n", jmp->node.src_addr);
        jmp_tx(JMP_MT_SORRY, NULL, 0, jmp->node.src_addr, READ_STATION_PANID, 1);
    }
    else
    {
        DBG_LOG("%d joined ok\n", jmp->node.src_addr);
        jmp_tx(JMP_MT_OK, NULL, 0, jmp->node.src_addr, READ_STATION_PANID, 1);
        _node_del(&jmp->node, jmpmgr.scan_list, &jmpmgr.scan_num);
    }
}

// init recv ok,init状态下会请求入网，收到OK入网成功
static void _jmp_state_r_rq_ok(JMP_PKT_T* jmp)
{
    JMP_NODE_T *node = &jmp->node;

    if (jmpmgr.scan_list[jmpmgr.ask_idx].src_addr == node->src_addr)
    {
        // goto state CN from RQ(request)
        jmp_state_change(NET_STATE_CN);

        jmpmgr.self.root_addr = node->src_addr;
        jmpmgr.self.weight = 0;
        jmpmgr.self.level = jmpmgr.scan_list[jmpmgr.ask_idx].level + 1;
        jmpmgr.self.link_fault = 0;
        jmpmgr.self.rssi = jmpmgr.scan_list[jmpmgr.ask_idx].rssi;
        jmpmgr.self.last_time = node->last_time;

        _node_del(&jmp->node, jmpmgr.scan_list, &jmpmgr.scan_num);

        DBG_LOG("join %X success\n", node->src_addr);
    }
}

// init 状态下的入网申请失败
static void _jmp_state_r_rq_sorry(JMP_PKT_T* jmp)
{
    DBG_JUDGE_RET(!jmpmgr.scan_num, "unexpect jmp-sorry\n");

    if (jmpmgr.scan_list[jmpmgr.ask_idx].src_addr == jmp->node.src_addr)
    {
        DBG_LOG("join %d failure\n", jmp->node.src_addr);
        _node_del(&jmp->node, jmpmgr.scan_list, &jmpmgr.scan_num);
        _request_next();
    }
}

// 连接状态下收到的sorry，很可能是data发送失败
static void _jmp_state_r_cn_sorry(JMP_PKT_T* jmp)
{
    if (jmp->node.src_addr != jmpmgr.self.root_addr)
        return;

    jmpmgr.self.link_fault++;

    if (jmpmgr.self.link_fault >= JMP_FAULT_MAX)
    {
        jmp_state_change(NET_STATE_NC);
    }
}

#define JMP_485_BUFFER_LEN  (1 << 11)                   // 2^11 = 2k
uint16 g_u16485Pos;
uint8 g_au8485Poll[JMP_485_BUFFER_LEN];

// 连接状态下收到的数据包
static void _jmp_state_r_cn_data(JMP_PKT_T* jmp)
{
    RADIO_JMP_HDR_T *pstJmpHdr;
    LPBSS_nwk_header_t *pstNwkHdr;
    uint16 u16Addr,u16Crcsum,u16ReceiveCrcsum;


    if (_data_filter(jmp) != TRUE)
    {
        DBG_WARN("jmp data recv filter\n");
        return;
    }

    pstJmpHdr = (RADIO_JMP_HDR_T *)(jmp->data_buf);
    pstNwkHdr = (LPBSS_nwk_header_t*)(pstJmpHdr + 1);

    if (NET_TYPE_485 == jmpmgr.net_type)
    {
        uint16 free_size;
        uint8 size;

        size = jmp->size - sizeof(RADIO_JMP_HDR_T)-2;
   		//size = jmp->size - sizeof(RADIO_JMP_HDR_T);
        free_size = sizeof(g_au8485Poll) - g_u16485Pos;


		memcpy((uint8 *)&u16ReceiveCrcsum,(uint8 *)(pstNwkHdr+1)+size - sizeof(LPBSS_nwk_header_t),2);
		u16Crcsum=CRC16((uint8 *)(pstNwkHdr+1),size - sizeof(LPBSS_nwk_header_t),0xffff);
		DBG_LOG("u16Crcsum= %d u16ReceiveCrcsum= %d\n", u16Crcsum, u16ReceiveCrcsum);
		DBG_LOG("size= %d \n", size);

		if(u16ReceiveCrcsum!=u16Crcsum)
			return;

        DBG_JUDGE_RET(size > free_size, "485buf full:%d,%d\n", size, free_size);

        memcpy(g_au8485Poll+g_u16485Pos, pstNwkHdr, size);
        g_u16485Pos += size;
    }
    else
    {
        if (JMP_DIREC_DOWN == pstJmpHdr->u8Direc)
        {
            uint16 tail_size = jmp->size - sizeof(RADIO_JMP_HDR_T)
                            - sizeof(LPBSS_nwk_header_t);
            LPBSS_Msg_Header_t *pstMsg = (LPBSS_Msg_Header_t*)(pstNwkHdr + 1);
            app_packet_parser(pstMsg, tail_size);
        }

        u16Addr = pstNwkHdr->u16DstAddr;
        CONVERT_ENDIAN(u16Addr);

        if (u16Addr != g_stSysMgr.u16SysAddr)
        {
            pstJmpHdr->u8Ttl--;
            _transit_packed(jmp->data_buf, jmp->size);
        }
    }
}

// bye recv ok
static void _jmp_state_r_nc_ok(JMP_PKT_T* jmp)
{
    if (jmp->node.src_addr != jmpmgr.self.root_addr)
    {
        _node_del(&jmp->node, jmpmgr.child_list, &jmpmgr.self.weight);
    }
    else
    {
        jmpmgr.self.root_addr = JMP_INVALID_ADDR;
    }
}

// 当前状态下收到BYE包
static void _jmp_state_r_x_bye(JMP_PKT_T* jmp)
{
    if (jmp->node.src_addr != jmpmgr.self.root_addr)
    {
        _node_del(&jmp->node, jmpmgr.child_list, &jmpmgr.self.weight);
    }
    else if (jmpmgr.net_type != NET_TYPE_485)
    {
        jmp_state_change(NET_STATE_NC);
        jmpmgr.self.root_addr = JMP_INVALID_ADDR;
    }

    jmp_tx(JMP_MT_OK, NULL, 0, jmp->node.src_addr, READ_STATION_PANID, 1);
}

static void _jmp_state_r_x_topo(JMP_PKT_T* jmp)
{
    if (jmp->node.src_addr != jmpmgr.self.root_addr)
        return;

    jmp_start_topo_proc();
}

/* 跳传接收包的解析器 */
static const PKT_PARSER s_apfnParser[][JMP_MT_MAX] =
{
    /********************************************
    *    state         MSG_TYPE                **
    ********************************************/
    [NET_STATE_RQ][JMP_MT_HI] = _jmp_state_r_x_hi,
    [NET_STATE_RQ][JMP_MT_JOIN] = _jmp_state_r_x_join,
    [NET_STATE_RQ][JMP_MT_OK] = _jmp_state_r_rq_ok,
    [NET_STATE_RQ][JMP_MT_SORRY] = _jmp_state_r_rq_sorry,
    [NET_STATE_RQ][JMP_MT_BYE] = NULL,
    [NET_STATE_RQ][JMP_MT_DATA] = _jmp_state_t_x_sorry,
    [NET_STATE_RQ][JMP_MT_TOPO] = NULL,

    [NET_STATE_CN][JMP_MT_HI] = _jmp_state_r_x_hi,
    [NET_STATE_CN][JMP_MT_JOIN] = _jmp_state_r_x_join,
    [NET_STATE_CN][JMP_MT_OK] = NULL,
    [NET_STATE_CN][JMP_MT_SORRY] = _jmp_state_r_cn_sorry,
    [NET_STATE_CN][JMP_MT_BYE] = _jmp_state_r_x_bye,
    [NET_STATE_CN][JMP_MT_DATA] = _jmp_state_r_cn_data,
    [NET_STATE_CN][JMP_MT_TOPO] = _jmp_state_r_x_topo,

    [NET_STATE_NC][JMP_MT_HI] = _jmp_state_r_x_hi,
    [NET_STATE_NC][JMP_MT_JOIN] = _jmp_state_r_x_join,
    [NET_STATE_NC][JMP_MT_OK] = _jmp_state_r_nc_ok,
    [NET_STATE_NC][JMP_MT_SORRY] = NULL,
    [NET_STATE_NC][JMP_MT_BYE] = NULL,
    [NET_STATE_NC][JMP_MT_DATA] = _jmp_state_t_x_sorry,
    [NET_STATE_NC][JMP_MT_TOPO] = NULL
};

/* 跳传包解析 */
bool_t jmp_rx_parser(const MAC_RxFrameData_s* const psFrame)
{
    RADIO_JMP_HDR_T *pstJmpHdr;
    JMP_PKT_T jmp;

    if (NULL == psFrame || psFrame->u8SduLength < sizeof(RADIO_JMP_HDR_T))
        return FALSE;

    pstJmpHdr = (RADIO_JMP_HDR_T *)(psFrame->au8Sdu);
    DBG_LOG("jmp rx %d a msg = %d\n", psFrame->sSrcAddr.uAddr.u16Short, pstJmpHdr->u8MsgType);

    if (RADIO_JMP_PT != pstJmpHdr->u8PktType)
        return FALSE;

    if (JMP_MT_MAX <= pstJmpHdr->u8MsgType)
        return FALSE;

    jmp.node.src_addr = psFrame->sSrcAddr.uAddr.u16Short;
    jmp.node.link_fault = 0;
    jmp.node.rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
    jmp.node.last_time = get_system_ms();
    jmp.size = psFrame->u8SduLength;
    jmp.data_buf = (uint8*)psFrame->au8Sdu;

    if (s_apfnParser[jmpmgr.net_state][pstJmpHdr->u8MsgType] != NULL)
        s_apfnParser[jmpmgr.net_state][pstJmpHdr->u8MsgType](&jmp);

    return TRUE;
}

void jmp_Init(void)
{
    jmpmgr.net_state = NET_STATE_RQ;
    jmpmgr.net_type = NET_TYPE_JMP;
    jmpmgr.self.src_addr = g_stSysMgr.u16SysAddr;

    _scan_start(get_system_ms());
    _request_start(get_system_ms());

    TimerUtil_eSetCircleTimer(JMP_EVENT_TICK, 5);
}

static void _transit_send(uint32 cur_tick)
{
    uint16 next_time = _100ms;

    if (jmpbuf.r_idx != jmpbuf.w_idx)
    {
        RADIO_JMP_HDR_T *pJmpHdr;
        LPBSS_nwk_header_t *pNwkHdr;
        uint16 dstaddr;
        uint16 srcaddr;
        uint8 txopt = 1;
        uint8 r_idx;

        pJmpHdr = (RADIO_JMP_HDR_T*)(jmpbuf.buf[jmpbuf.r_idx]);
        pNwkHdr = (LPBSS_nwk_header_t*)(pJmpHdr + 1);
        dstaddr = jmpmgr.self.root_addr;
        srcaddr = pNwkHdr->u16SrcAddr;
        CONVERT_ENDIAN(srcaddr);
        if (pJmpHdr->u8Direc != JMP_DIREC_UP)
        {
            dstaddr = 0xFFFF;
            txopt = 0;
        }
        DBG_LOG("time=%d, %d send data to %d length %d\n",
            cur_tick, srcaddr, dstaddr, jmpbuf.size[jmpbuf.r_idx]);
        jmp_tx(JMP_MT_DATA, jmpbuf.buf[jmpbuf.r_idx], jmpbuf.size[jmpbuf.r_idx],
            dstaddr, READ_STATION_PANID, txopt);

        r_idx = jmpbuf.r_idx + 1;

        if (r_idx == JMP_BUFFER_NUM)
        {
            r_idx = 0;
        }

        jmpbuf.r_idx = r_idx;

        vAHI_StartRandomNumberGenerator(E_AHI_RND_SINGLE_SHOT, E_AHI_INTS_DISABLED);
        next_time = (u16AHI_ReadRandomNumber() & _15ms);
        next_time += _5ms;
    }
    else
    {
        _next_jmpbuf();
    }

    send_tick = cur_tick + next_time;
}

static bool _link_update(uint32 cur_tick)
{
    uint8 good_idx;

    if (cur_tick - jmpmgr.self.last_time >= JMP_LINK_TIMEOUT)
    {
        DBG_WARN("link lose\n");
        jmp_state_change(NET_STATE_NC);
        return FALSE;
    }

    if (cur_tick - join_start_time < TIMER_TICK_MAX)
    {
        join_start_time  = cur_tick + JMP_JOIN_CN_FREQ;

        good_idx = _find_good_scan();

        if (good_idx != jmpmgr.scan_num)
        {
            jmp_state_change(NET_STATE_NC);
            jmpmgr.ask_idx = good_idx;

            DBG_LOG("link update to %d\n", jmpmgr.scan_list[good_idx].src_addr);
            return TRUE;
        }

        jmpmgr.self.link_fault = 0;
    }

    return FALSE;
}

static void _send_hi(void)
{
    static uint8 send_buf[20];
    RADIO_JMP_HDR_T *pstJmpHdr = (RADIO_JMP_HDR_T*)send_buf;
    JMP_MSG_HI_T *pstMsgHi = (JMP_MSG_HI_T *)(pstJmpHdr + 1);

    pstJmpHdr->u8PktType = RADIO_JMP_PT;
    pstJmpHdr->u8MsgType = JMP_MT_HI;
    pstJmpHdr->u16SeqNum = jmp_seqnum++;
    pstJmpHdr->u8Direc = JMP_DIREC_DOWN;
    pstJmpHdr->u8Ttl = 1;
    pstJmpHdr->u16Len = sizeof(JMP_MSG_HI_T);

    pstMsgHi->u8Level = jmpmgr.self.level;
    pstMsgHi->u8Weight = jmpmgr.self.weight;
    pstMsgHi->u16RootAddr = jmpmgr.self.root_addr;

    jmp_tx(JMP_MT_HI,
            send_buf,
            sizeof(RADIO_JMP_HDR_T) + sizeof(JMP_MSG_HI_T),
            NWK_BROADCAST_ADDR,
            READ_STATION_PANID, 0);
}

static void _cn_485_tick(uint32 cur_tick)
{
    if (cur_tick - hi_tick < TIMER_TICK_MAX)
    {
        _node_timeout_del(jmpmgr.child_list, &jmpmgr.self.weight);
        _node_timeout_del(jmpmgr.scan_list, &jmpmgr.scan_num);
        _send_hi();
        vAHI_StartRandomNumberGenerator(E_AHI_RND_SINGLE_SHOT, E_AHI_INTS_DISABLED);
        hi_tick = (u16AHI_ReadRandomNumber() & _15ms);
        hi_tick += cur_tick + JMP_HI_FREQ;
    }

    if (cur_tick - send_tick < TIMER_TICK_MAX)
    {
        _transit_send(cur_tick);
    }
}

static void _cn_jmp_tick(uint32 cur_tick)
{
    _cn_485_tick(cur_tick);

    if (cur_tick - send_topo_tick < TIMER_TICK_MAX)
    {
        jmp_start_topo_proc();
        send_topo_tick = cur_tick + JMP_TOPO_FREQ;
    }

    if (cur_tick - send_ver_tick < TIMER_TICK_MAX)
    {
        uint8 au8verbuf[50];
        uint8 versize;

        versize = app_get_sysver(au8verbuf, 50);
        if (versize > 0)
        {
            uint8 *pbuf = au8verbuf + sizeof(LPBSS_Msg_Header_t);
            versize -= sizeof(LPBSS_Msg_Header_t);
            jmp_data_packed(LPBSS_VERSION, pbuf, versize);
        }

        send_ver_tick = cur_tick + JMP_VER_FREQ;
    }

    _link_update(cur_tick);
}

static void _rq_jmp_tick(uint32 cur_tick)
{
    uint8 idx;

    if (!scan_ok)
        return;

    idx = _request_next();

    if (idx != jmpmgr.scan_num)
    {
        if (jmpmgr.ask_idx != idx)
        {
            jmpmgr.ask_idx = idx;
            _request_start(cur_tick);
        }
    }
    else
    {
        _scan_start(cur_tick);
        return;
    }

    if (cur_tick - rq_last_time < TIMER_TICK_MAX)
    {
        DBG_LOG("join %d failure\n", jmpmgr.scan_list[jmpmgr.ask_idx].src_addr);

        _node_del(&jmpmgr.scan_list[jmpmgr.ask_idx], jmpmgr.scan_list, &jmpmgr.scan_num);
        return;
    }

    if (cur_tick - join_start_time < TIMER_TICK_MAX)
    {
        DBG_LOG("send join %d\n", jmpmgr.scan_list[jmpmgr.ask_idx].src_addr);

        jmp_tx(JMP_MT_JOIN, NULL, 0, jmpmgr.scan_list[jmpmgr.ask_idx].src_addr,
            READ_STATION_PANID, 1);
        join_start_time  = cur_tick + JMP_JOIN_RQ_FREQ;
    }
}

static void _nc_jmp_tick(uint32 cur_tick)
{
    // 当没有子节点，也没有父节点时可以从NC转换到INIT
    if ((cur_tick - bye_start >= JMP_BYE_TIMEOUT)
        || (JMP_INVALID_ADDR == jmpmgr.self.root_addr
            && 0 == jmpmgr.self.weight)
       )
    {
        jmp_state_change(NET_STATE_RQ);
        return;
    }

    if (cur_tick - bye_tick < TIMER_TICK_MAX)
        return;
    bye_tick = cur_tick + JMP_BYE_FREQ;

    if (jmpmgr.self.weight > 0)
    {
        jmp_tx(JMP_MT_BYE, NULL, 0, NWK_BROADCAST_ADDR, READ_STATION_PANID, 0);
    }

    if (JMP_INVALID_ADDR != jmpmgr.self.root_addr)
    {
        DBG_LOG("nc bye\n");
        jmp_tx(JMP_MT_BYE,
                NULL, 0,
                jmpmgr.self.root_addr,
                READ_STATION_PANID, 1);
    }
}

typedef void (*event_tick_proc)(uint32);

static const event_tick_proc const tick_proc[][NET_TYPE_INV] =
{
    [NET_STATE_RQ][NET_TYPE_485] = NULL,
    [NET_STATE_RQ][NET_TYPE_JMP] = _rq_jmp_tick,
    [NET_STATE_CN][NET_TYPE_485] = _cn_485_tick,
    [NET_STATE_CN][NET_TYPE_JMP] = _cn_jmp_tick,
    [NET_STATE_NC][NET_TYPE_485] = NULL,
    [NET_STATE_NC][NET_TYPE_JMP] = _nc_jmp_tick,
};

static void _jmp_event_tick_proc(void)
{
    static uint32 dbg_freq;
    uint32 cur_tick;

    cur_tick = get_system_ms();

    if (tick_proc[jmpmgr.net_state][jmpmgr.net_type])
    {
        tick_proc[jmpmgr.net_state][jmpmgr.net_type](cur_tick);
    }
#ifdef JMP_DUMP
    if (cur_tick - dbg_freq < TIMER_TICK_MAX)
    {
        uint8 i;

        dbg_freq = cur_tick + 1000;
        DBG_LOG("root=%d, level=%i, rssi=%i\n", jmpmgr.self.root_addr,
            jmpmgr.self.level-1, jmpmgr.self.rssi);
        DBG_LOG("state = %d, scan = %d\n", jmpmgr.net_state, jmpmgr.scan_num);

        for (i=0; i<jmpmgr.scan_num; i++)
        {
            DBG_LOG("%d, %i, %d\n", jmpmgr.scan_list[i].src_addr,
                jmpmgr.scan_list[i].rssi, jmpmgr.scan_list[i].level);
        }
    }
#endif
}

// 事件处理主循环
uint32 jmp_event_process(uint32 u32Event)
{
    if (u32Event & JMP_EVENT_TICK)
    {
        _jmp_event_tick_proc();
        EventUtil_vUnsetEvent(JMP_EVENT_TICK);
    }

    return u32Event;
}

uint8 jmp_state_get(void)
{
    return jmpmgr.net_state;
}

// 跳传网络状态
void jmp_state_change(const uint8 u8NetState)
{
    uint32 cur_tick = get_system_ms();

    switch (u8NetState)
    {
    case NET_STATE_NC:
        if (NET_STATE_CN == jmpmgr.net_state)
        {
            bye_start = cur_tick;
            EventUtil_vSetEvent(JMP_EVENT_TICK);
            EventUtil_vSetEvent(CARDREADER_LED_TOGGLE_EVENT);
        }
        else if (NET_STATE_RQ == jmpmgr.net_state)
        {
            return;
        }
        break;

    case NET_STATE_CN:
        send_tick = join_start_time = hi_tick = cur_tick;
        send_ver_tick = send_topo_tick = cur_tick;
        break;

    case NET_STATE_RQ:
        _request_start(cur_tick);
        break;
    }

    DBG_LOG("net state = %d\n", u8NetState);
    jmpmgr.net_state = u8NetState;
}

uint8 jmp_type_get(void)
{
    return jmpmgr.net_type;
}

// 网络工作类型，485还是跳传
void jmp_type_change(const uint8 u8NetType)
{
    if (u8NetType == jmpmgr.net_type)
        return;

    DBG_LOG("net type = %d\n", u8NetType);

    if (NET_TYPE_485 == u8NetType)
    {
        g_u16485Pos = 0;
        jmpmgr.self.root_addr = 0;
        jmpmgr.self.level = 0;
        jmp_state_change(NET_STATE_CN);
    }
    else if (NET_TYPE_485 == jmpmgr.net_type)
    {
        jmp_state_change(NET_STATE_NC);
    }

    jmpmgr.net_type = u8NetType;
}

// 将485命令转发到跳传网络
void jmp_transmit_down(void *buf, uint16 size)
{
    static uint8 au8jmpbuf[JMP_BUFFER_LEN];
    LPBSS_nwk_header_t *pstNwk = (LPBSS_nwk_header_t *)buf;
    RADIO_JMP_HDR_T *pstJmpHdr = (RADIO_JMP_HDR_T *)au8jmpbuf;
    uint16 u16NwkLen;

    ASSERT_RET(pstNwk != NULL && size > sizeof(LPBSS_nwk_header_t));
    u16NwkLen = pstNwk->u16Len;
    CONVERT_ENDIAN(u16NwkLen);
    ASSERT_RET(u16NwkLen <= JMP_APP_LEN && size <= JMP_BUFFER_LEN - sizeof(RADIO_JMP_HDR_T));

    pstJmpHdr->u8PktType = RADIO_JMP_PT;
    pstJmpHdr->u8MsgType = JMP_MT_DATA;
    pstJmpHdr->u16SeqNum = jmp_seqnum++;
    pstJmpHdr->u8Direc = JMP_DIREC_DOWN;
    pstJmpHdr->u8Ttl = 255;
    pstJmpHdr->u16Len = size;

    memcpy((uint8 *)(pstJmpHdr + 1), buf, size);
    _transit_packed(au8jmpbuf, size + sizeof(RADIO_JMP_HDR_T));
}

// 触发topology上报
void jmp_start_topo_proc(void)
{
    static uint8 s_au8TopoBuffer[4 + 2 * JMP_CHILD_MAX];
    uint8 i;
    uint8 u8Pos;

    DBG_LOG("root id = %d\n", jmpmgr.self.root_addr);

    // topo, len and root_addr
    s_au8TopoBuffer[0] = (jmpmgr.self.root_addr & 0xFF);
    s_au8TopoBuffer[1] = ((jmpmgr.self.root_addr >> 8) & 0xFF);
    s_au8TopoBuffer[2] = (g_stSysMgr.u16SysAddr & 0xFF);
    s_au8TopoBuffer[3] = ((g_stSysMgr.u16SysAddr >> 8) & 0xFF);
    u8Pos = 4;

    for (i=0; i<jmpmgr.self.weight; i++)
    {
        s_au8TopoBuffer[u8Pos+0] = (jmpmgr.child_list[i].src_addr & 0xFF);
        s_au8TopoBuffer[u8Pos+1] = ((jmpmgr.child_list[i].src_addr >> 8) & 0xFF);
        u8Pos += 2;
    }

    jmp_data_packed(LPBSS_JMP_TOPO, s_au8TopoBuffer, u8Pos);
}

static uint16 _packet_apphdr_fill(uint16 u16PldLen, uint8 *pu8Poll, uint8 u8Mt)
{
    LPBSS_Msg_Header_t *pstMsgHdr = (LPBSS_Msg_Header_t*)pu8Poll;

    pstMsgHdr->u8Msg = u8Mt;
    pstMsgHdr->u8Padding = 0;
    pstMsgHdr->u16len = u16PldLen;
    CONVERT_ENDIAN(pstMsgHdr->u16len);

    return sizeof(LPBSS_Msg_Header_t);
}

// 封装跳传包
void jmp_data_packed(uint8 u8Mt, uint8 *buf, uint16 u16Len)
{
    DBG_JUDGE_RET(!buf || !u16Len, "buf is invalid\n");
    DBG_JUDGE_RET(u16Len > JMP_PAYLOAD_LEN, "buf full=%d, type=%d\n", u16Len, u8Mt);

    switch (u8Mt)
    {
    case LPBSS_JMP_TOPO:// 拓扑封包
    case LPBSS_VERSION:
    case LPBSS_CARD_VERSION:
    case LPBSS_DEV_INFO:
    case LPBSS_CARD_LOC_DATA:// 卡数据封包
        break;

    default:
        DBG_WARN("msg type not support\n");
        return;
    }

    // trust current poll, ready next poll
    if (JMP_BUFFER_LEN - jmpbuf.pos
        < u16Len + sizeof(LPBSS_Msg_Header_t))
    {
        _next_jmpbuf();
    }

    jmpbuf.hold = 1;
    // each sub packet fill a apphdr
    jmpbuf.pos += _packet_apphdr_fill(u16Len, jmpbuf.buf[jmpbuf.w_idx] + jmpbuf.pos, u8Mt);

    memcpy(jmpbuf.buf[jmpbuf.w_idx] + jmpbuf.pos, buf, u16Len);
    jmpbuf.pos += (uint8)u16Len;
}




