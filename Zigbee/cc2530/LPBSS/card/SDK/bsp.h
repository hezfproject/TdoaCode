/*******************************************************************************
  Filename:       bsp.h
  Revised:        $Date: 18:21 2012年5月7日
  Revision:       $Revision: 1.0 $

  Description:    BSP library

*******************************************************************************/
#ifndef _BSP_H_
#define _BSP_H_

#include <types.h>

/*******************************************************************************
*                                             Macros
*/

#define BSP_SYSTEM_REBOOT() HAL_SYSTEM_RESET()

/*******************************************************************************
* GLOBAL FUNCTIONS DECLARATION
*/

UINT16 BSP_GetRandom(VOID);


/*******************************************************************************
* @fn          BSP_BoardInit
*
* @brief       硬件初始化接口
*
* @param       none
*
*
* @return      none
*/
VOID BSP_BoardInit(VOID);

#ifdef OPEN_WTD
/*******************************************************************************
* @fn          BSP_WATCHDOG_Feed
*
* @brief       喂狗操作
*
* @param       none
*
*
* @return      none
*/
VOID BSP_WATCHDOG_Feed(VOID);
#endif

/*******************************************************************************
* @fn          BSP_SLEEP_Enter
*
* @brief       进入休眠
*
* @param     input - u32Ms 休眠的时间长度
*
*
* @return     none
*/
VOID BSP_SLEEP_Enter(UINT32 u32Ms);

/*******************************************************************************
* @fn          BSP_GetSysTick
*
* @brief       获取当前系统TICK
*
* @param     none
*
*
* @return     当前系统的Tick数
*/
UINT32 BSP_GetSysTick(VOID);

/*******************************************************************************
* @fn          BSP_GetExIEEEInfo
*
* @brief       获取IEEE地址参数
*
* @param      output- pu8Buffer 存放IEEE地址参数的buffer
*
*             input- u8Len 存放参数的长度
* @return      none
*/
UINT8 BSP_GetExIEEEInfo(UINT8 *pu8Buffer, UINT8 u8Len);

/*******************************************************************************
* @fn          BSP_ADC_GetVdd
*
* @brief       获取当前系统的电量，单位0.1v
*
* @param      none
*
*
* @return      电量值
*/
UINT8 BSP_ADC_GetVdd(VOID);

#endif
