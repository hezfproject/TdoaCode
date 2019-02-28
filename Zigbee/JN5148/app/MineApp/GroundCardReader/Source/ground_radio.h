#ifndef _GROUND_RADIO_H_
#define _GROUND_RADIO_H_

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "lpbssnwk.h"
#include "MbusProto.h"
#include "RadioProto.h"
#include "ground_485.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define CARDINFO_TIMEOUT                    (40000)
#define SEARCH_TIMEOUT                      (40000)

// sizeof(LPBSS_card_data_t) * BUS485_MAX_CARD_NUM
#define CARD_LOC_BUFF_LEN   (BUS485_MAX_CARD_NUM * 12)
#define CARD_INFO_BUFF_LEN  (1024)
#define CARD_VER_BUFF_LEN   (256)

#define SET_CARD_INFO_NUM    16

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/*******************************************************************************
*****ÉÏÐÐ»º³åÇø
*******************************************************************************/
typedef struct
{
    uint16 u16CardVerLen;
    const uint16 u16LimitLen;
    uint8* const u8CardVerBuf;
}card_ver_t;

typedef struct
{
    uint16 u16CardInfoLen;
    const uint16 u16LimitLen;
    uint8* const u8CardInfoBuf;
}card_info_t;

typedef struct
{
    uint16 u16CardLocDataLen;
    const uint16 u16LimitLen;
    uint8* const u8CardLocBuf;
}card_loc_t;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern card_loc_t g_st485LocBuf;
extern card_info_t g_st485InfoBuf;
extern card_ver_t g_st485VerBuf;

#ifdef USE_JMP_NET
extern card_info_t g_stJmpInfoBuf;
extern card_ver_t g_stJmpVerBuf;
#endif

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
extern void radio_cast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId,
                        uint8 u8TxOptions, uint8* buf);

extern void radio_search_timeout(uint32 u32CurMs);

extern void radio_cardinfo_timeout(uint32 u32CurMs);

extern void radio_extaddr_init(void);

extern void radio_mac_init(void);
extern void radio_incoming_MLME(MAC_MlmeDcfmInd_s const * const psMlmeInd);
extern void radio_incoming_MCPS(MAC_McpsDcfmInd_s const * const psMcpsInd);
extern void radio_start_coordinator(void);
extern void radio_stack_check(void);
extern void radio_card_sms_pool(void);
extern void radio_wrist_card_rsp(void);

#endif

