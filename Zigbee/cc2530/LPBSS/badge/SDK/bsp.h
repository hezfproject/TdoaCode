/*******************************************************************************
  Filename:       bsp.h
  Revised:        $Date: 18:21 2012��5��7��
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
* @brief       Ӳ����ʼ���ӿ�
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
* @brief       ι������
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
* @brief       ��������
*
* @param     input - u32Ms ���ߵ�ʱ�䳤��
*
*
* @return     none
*/
VOID BSP_SLEEP_Enter(UINT32 u32Ms);

/*******************************************************************************
* @fn          BSP_GetSysTick
*
* @brief       ��ȡ��ǰϵͳTICK
*
* @param     none
*
*
* @return     ��ǰϵͳ��Tick��
*/
UINT32 BSP_GetSysTick(VOID);

/*******************************************************************************
* @fn          BSP_GetExIEEEInfo
*
* @brief       ��ȡIEEE��ַ����
*
* @param      output- pu8Buffer ���IEEE��ַ������buffer
*
*             input- u8Len ��Ų����ĳ���
* @return      none
*/
UINT8 BSP_GetExIEEEInfo(UINT8 *pu8Buffer, UINT8 u8Len);

/*******************************************************************************
* @fn          BSP_ADC_GetVdd
*
* @brief       ��ȡ��ǰϵͳ�ĵ�������λ0.1v
*
* @param      none
*
*
* @return      ����ֵ
*/
UINT8 BSP_ADC_GetVdd(VOID);

#endif
