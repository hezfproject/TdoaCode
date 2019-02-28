#ifndef _GROUND_485APP_H_
#define _GROUND_485APP_H_

/* RX state machine */
typedef enum
{
    BUS485_SYNC_IDLE         =   0,       /* IDLE STATE */
    BUS485_SYNC_SYNC1        =   1,       /* SYNCED 'Y', WANT 'I' */
    BUS485_SYNC_SYNC2        =   2,       /* SYNCED 'I', WANT 'R' */
    BUS485_SYNC_CMD          =   3,       /* WANT cmd */
    BUS485_SYNC_SLVID_H      =   4,       /* WANT 'R' */
    BUS485_SYNC_SLVID_L      =   5,       /* WANT 'R' */
    BUS485_SYNC_DATALEN_H    =   6,       /* WANT 'R' */
    BUS485_SYNC_DATALEN_L    =   7,       /* WANT 'R' */
    BUS485_SYNC_DATA         =   8,       /* RECV DATA*/
    BUS485_SYNC_KIND
}BUS485_SYNC_E;

/****************************************************************************
* MICROS
*****************************************************************************/
#define BUS485_MAX_RX_LEN   128
#define BUS485_MAX_TX_LEN   2048
#define BUS485_MAX_LOSE     3
#define BUS485_MAX_CARD_NUM    168

#define BUS485_SUCCESS 0
#define BUS485_INVALIDPARAM 1
#define BUS485_BUSY 2

#ifdef USE_JMP_NET
extern uint16 g_u16485Pos;
extern uint8 g_au8485Poll[];
#endif

/****************************************************************************
* FUNCTIONS
*****************************************************************************/
PUBLIC void bus485_init(void);

PUBLIC uint32 bus485_event_process(uint32 u32Event);

PUBLIC uint8 bus485_tx_start(uint8 *p, uint16 len);

PUBLIC bool_t bus485_is_tx(void);

PUBLIC void bus485_rx_parser(void);

PUBLIC void bus485_setbuaudrate(uint8 u8BaudRate);

#endif

