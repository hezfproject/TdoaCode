/*******************************************************************************
  Filename:     bsp_key.c
  Revised:        $Date: 10:59 2012年5月10日
  Revision:       $Revision: 1.0 $
  Description:  BSP button library

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <hal_button.h>
#include <types.h>
#include <hal_mcu.h>
#include <timer_event.h>
#include "bsp_key.h"
#include "app_card_wireless.h"
#include <string.h>

#define  WAKE_TIMEOUT_CELL  (1)

/*******************************************************************************
* LOCAL DATA
*/
static BOOL s_bInitOk;

//#ifdef STAFF_CARD
static BOOL s_bWakeed = false;
static uint16 recIdx = 0;
static uint8 LFRecData[LF_TO_CARD_LEN+1];

/*******************************************************************************
* GLOBAL FUNCTIONS
*/
uint8 LFData[LF_TO_CARD_LEN+1];
/*************************************************************************/
static VOID bsp_LF_Data_Receive(VOID);
static void halLFWakeIO(void);

#ifdef _DEBUG_TICK 
uint32 U32tick = 0;
uint32 sU32tick2 = 0;
uint32 sU32tick3 = 0;
uint32 sU32tick4 = 0;
uint32 sU32tick5 = 0;
#endif
static void Wakeup_interrupt(void)
{
    static uint8 bitsCount = 0;
    volatile bool flag= false;
    
    if(!s_bWakeed)
    {   
    #ifdef _DEBUG_TICK     
        U32tick = BSP_GetSysTick();
    #endif     
        if(bitsCount++ < 45)
        {
    #ifdef _DEBUG_TICK      
            DEBUG_PIN_L;
    #endif        
            flag =event_timer_add(EVENT_WAKE_TIMEOUT_MSG, (80-bitsCount)*WAKE_TIMEOUT_CELL);
            if(flag)
            {   
                bitsCount = 0;
                s_bWakeed = true;
            }
        }
        else
        {
            bitsCount = 0;
        }
    }
    
    if((s_bWakeed)||(bitsCount > 1))
    {
        bsp_LF_Data_Receive();
    }
}

/*******************************************************************************
* @fn          BSP_Wakeup_Init
*
* @brief       wake初始化
*
*
* @param    none
*
* @return     none
*/
VOID BSP_Wakeup_Init(VOID)
{
    halLFWakeIO();
    HAL_Wakeup_Init(Wakeup_interrupt);

}

/*******************************************************************************
* @fn          bsp_LF_Data_Receive
*
* @brief
*
* @param     none
*
*
* @return     none
*/
static VOID bsp_LF_Data_Receive(VOID)
{
    if(recIdx >= LF_TO_CARD_LEN*8)
    {
        return;
    }

    if(LF_RECEIVE_DATA_IOTIEM&LF_RECEIVE_DATA_BIT)
    {
        LFRecData[recIdx/8] |= BV(7-(recIdx%8));
    }
    else
    {        
        LFRecData[recIdx/8] &=~ BV(7-(recIdx%8));        
    }
    
    recIdx++;

    if(recIdx >= (LF_TO_CARD_LEN*8))
    {        
        memcpy(LFData,LFRecData,LF_TO_CARD_LEN);
        memset(LFRecData,0,LF_TO_CARD_LEN+1);
        event_timer_set(EVENT_RESPONCELF_MSG);
        s_bWakeed = false;
        recIdx = 0;  
        event_timer_del(EVENT_WAKE_TIMEOUT_MSG);   
    }
}

/*
 * 初始化WAKE(SLC)引脚
 */
static void halLFWakeIO(void)
{
    LF_RECEIVE_DATA_SEL &=~ LF_RECEIVE_DATA_BIT;
    LF_RECEIVE_DATA_INP |= LF_RECEIVE_DATA_BIT;
}

/*
 * 回到接收准备状态
 */
void Reset_Wake_Status(void)
{  
  #ifdef _DEBUG_TICK 
    sU32tick4 = BSP_GetSysTick();  
    DEBUG_PIN_H;
  #endif  
    
    if(recIdx >= (LF_TO_CARD_LEN*8))
    {
        memcpy(LFData,LFRecData,LF_TO_CARD_LEN);
        memset(LFRecData,0,LF_TO_CARD_LEN+1);
        event_timer_set(EVENT_RESPONCELF_MSG);
    }

    s_bWakeed = false;
    recIdx = 0;
}


static void key_interrupt(void)
{
    event_timer_set(EVENT_KEY_MSG);
}

/*******************************************************************************
* @fn          BSP_KEY_Init
*
* @brief      配置按键1初始化
*
*
* @param    none
*
* @return     none
*/
VOID BSP_KEY_Init(VOID)
{
    if (!s_bInitOk)
    {
        s_bInitOk = true;

        HAL_BUTTON_Init(key_interrupt);
    }
}

/*******************************************************************************
* @fn          BSP_KEY_IsDown
*
* @brief        判断按键1是否被按下
*
*
* @param    none
*
* @return     none
*/
BOOL BSP_KEY_IsDown(VOID)
{
    if (HAL_BUTTON_1_ISDOWN())
    {
    #ifdef OPEN_WTD
        HAL_WATCHDOG_Feed();
    #endif
        HAL_WaitUs(5000);

        if (HAL_BUTTON_1_ISDOWN())
            return true;
    }

    return false;
}

