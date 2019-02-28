#ifndef _GROUND_JMP_H_
#define _GROUND_JMP_H_

#include "lpbssnwk.h"
#include "RadioProto.h"

// 事件
#define JMP_EVENT_TICK      (JMP_TICK_EVENT)

/*  (sizeof(RADIO_JMP_HDR_T)        \
    + sizeof(LPBSS_nwk_header_t)    \
    )
*/
#define JMP_PLD_OFFSET      (20)

#define JMP_BUFFER_NUM      (20)
#define JMP_BUFFER_LEN      (114)          // 114-8-12-4 = 90
#define JMP_APP_LEN         (JMP_BUFFER_LEN - JMP_PLD_OFFSET)

/* JMP_APP_LEN - sizeof(LPBSS_Msg_Header_t) */
#define JMP_PAYLOAD_LEN     (JMP_APP_LEN - 4)

typedef enum
{
    NET_TYPE_JMP = 0,
    NET_TYPE_485 = 1,
    NET_TYPE_INV
} JMP_NET_TYPE_E;

// 跳传的工作状态
typedef enum
{
    NET_STATE_RQ = 0,   // 请求连接状态态
    NET_STATE_CN = 1,   // 已连接状态
    NET_STATE_NC = 2,   // 未连接状态
} JMP_NET_STATE_E;

// 跳传数据包回复
void jmp_tx(const uint8 u8CmdType, void* buf, const uint8 u8Len,
            const uint16 u16DstAddr, const uint16 u16DstPanId,
            const uint8 u8TxOptions);

uint8 jmp_state_get(void);

// 跳传网络状态
void jmp_state_change(const uint8 u8NetState);

uint8 jmp_type_get(void);

// 网络工作类型，485还是跳传
void jmp_type_change(const uint8 u8NetType);

// 跳传网络初始化, 初始状态可以申请入网，侦听基站
void jmp_Init(void);

/* 跳传包解析 */
bool_t jmp_rx_parser(const MAC_RxFrameData_s* const psFrame);

// 将485命令转发到跳传网络
void jmp_transmit_down(void *pstNwk, uint16 size);

// 触发topology上报
void jmp_start_topo_proc(void);

// 封装跳传包
void jmp_data_packed(uint8 u8Mt, uint8 *buf, uint16 u16Len);

// 事件处理主循环
uint32 jmp_event_process(uint32 u32Event);

#endif

