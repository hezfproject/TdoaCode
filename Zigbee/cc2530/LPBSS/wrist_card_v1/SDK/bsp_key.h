/*******************************************************************************
  Filename:     bsp_key.h
  Revised:        $Date: 11:43 2012年5月10日
  Revision:       $Revision: 1.0 $
  Description:  BSP button library

*******************************************************************************/
#ifndef _BSP_KEY_H_
#define _BSP_KEY_H_

#include "RadioProto.h"

#define LF_RECEIVE_DATA_SEL      P0SEL
#define LF_RECEIVE_DATA_INP      P0INP
#define LF_RECEIVE_DATA_BIT      BV(5)
#define LF_RECEIVE_DATA_IOTIEM   P0


/*******************************************************************************
* GLOBAL FUNCTIONS
*/
extern uint8 LFData[LF_TO_CARD_LEN+1];

/*******************************************************************************
* @fn          BSP_KEY_Init
*
* @brief       配置按键1初始化
*
*
* @param    none
*
* @return     none
*/
VOID BSP_KEY_Init(VOID);
VOID BSP_Wakeup_Init(VOID);

/*******************************************************************************
* @fn          BSP_KEY_IsDown
*
* @brief       判断按键1是否被按下
*
*
* @param    none
*
* @return     none
*/
BOOL BSP_KEY_IsDown(VOID);
void Reset_Wake_Status(void);
BOOL BSP_KEY1_IsDown(VOID);
BOOL BSP_KEY2_IsDown(VOID);

#endif //_BSP_KEY_H_
