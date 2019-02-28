#ifndef _GROUND_JMP_H_
#define _GROUND_JMP_H_

#include "lpbssnwk.h"
#include "RadioProto.h"

// �¼�
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

// �����Ĺ���״̬
typedef enum
{
    NET_STATE_RQ = 0,   // ��������״̬̬
    NET_STATE_CN = 1,   // ������״̬
    NET_STATE_NC = 2,   // δ����״̬
} JMP_NET_STATE_E;

// �������ݰ��ظ�
void jmp_tx(const uint8 u8CmdType, void* buf, const uint8 u8Len,
            const uint16 u16DstAddr, const uint16 u16DstPanId,
            const uint8 u8TxOptions);

uint8 jmp_state_get(void);

// ��������״̬
void jmp_state_change(const uint8 u8NetState);

uint8 jmp_type_get(void);

// ���繤�����ͣ�485��������
void jmp_type_change(const uint8 u8NetType);

// ���������ʼ��, ��ʼ״̬��������������������վ
void jmp_Init(void);

/* ���������� */
bool_t jmp_rx_parser(const MAC_RxFrameData_s* const psFrame);

// ��485����ת������������
void jmp_transmit_down(void *pstNwk, uint16 size);

// ����topology�ϱ�
void jmp_start_topo_proc(void);

// ��װ������
void jmp_data_packed(uint8 u8Mt, uint8 *buf, uint16 u16Len);

// �¼�������ѭ��
uint32 jmp_event_process(uint32 u32Event);

#endif

