#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "cc_def.h"
#include "CommonTypes.h"
#define xAPP_DBG_LOG
#define xMAC_DBG_LOG
#define xCOM_DBU_LOG
#define xDBG_LOG
/****************************************************************************************
****************************************************************************************/
#if defined(APP_DBG_LOG) || defined(DBG_LOG)
#define APP_DBG(...)                       \
    do{                                    \
       printf(__VA_ARGS__);                \
    }while(0)
#else
#define APP_DBG(...)
#endif

#if defined(MAC_DBG_LOG) || defined(DBG_LOG)
#define MAC_DBG(...)                       \
    do{                                    \
       printf(__VA_ARGS__);                \
    }while(0)
#else
#define MAC_DBG(...)
#endif

#if defined(COM_DBG_LOG) || defined(DBG_LOG)
#define COM_DBG(...)                       \
    do{                                    \
       printf(__VA_ARGS__);                \
    }while(0)
#else
#define COM_DBG(...)
#endif

#if defined(DBG_LOG)
#define ERR_DBG(...)                       \
    do{                                    \
       printf(__VA_ARGS__);                \
    }while(0)
#else
#define ERR_DBG(...)
#endif

#define DBG_INFO_SEND_ADDR 0xFFFF


/**
  * @brief  The assert_param macro is used for function's parameters check.
  * @param  expr: If expr is false, it calls DbgAssertFailed function which reports
  *         the name of the source file and the source line number of the call
  *         that failed. If expr is true, it returns no value.
  * @retval None
  */
#if defined(DBG_LOG)
	#define assert_param(expr) ((expr) ? (void)0 : DbgAssertFailed((uint8 *)__FILE__, __LINE__))
#else
	#define assert_param(expr) ((void)0)
#endif /* USE_FULL_ASSERT */

void DbgAssertFailed(uint_8* file, uint_32 line);

void DbgLedFlashReset(void);

#endif

