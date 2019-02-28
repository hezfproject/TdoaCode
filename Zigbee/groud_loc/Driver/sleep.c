#include "sleep.h"
#include "CPU.h"
#include "stm32f10x.h"

#define SLP_SEC	(ms)

/* fTR_CLK = fRTCCLK / (RTC_PRESCALE + 1)*/
#define RTC_PRESCALE    (31)		//32·ÖÆµ		32767

//#define USE_LSE

#define WFE
//#define STOP_WFE

void SleepInit(void)
{
    /* Enable PWR and BKP clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    
    /* Configure EXTI Line9 to generate an interrupt on falling edge */
    EXTI_Configuration();
    
    /* Configure RTC clock source and prescaler */
    RTC_Configuration();
}

void Sleep(uint32 ms)
{
    /* Set the RTC Alarm after SLP_SEC */
    RTC_SetAlarm(RTC_GetCounter()+ ms);	//#define SLP_SEC	(ms)SLP_SEC
    
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
    
#ifdef WFE
    __WFE(); 
#else 
#ifdef STOP_WFE
#ifdef STOP_FAST
    /* Request to enter STOP mode with regulator ON */
    PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFE);
#else
    /* Request to enter STOP mode with regulator in Low Power */
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFE);
#endif
    SystemInit();
#endif
#endif
}

/**
  * @brief  Configures EXTI Line17(RTC Alarm).
  * @param  None
  * @retval : None
  */
void EXTI_Configuration(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    /* Configure EXTI Line17(RTC Alarm) to generate an interrupt on rising edge */
    EXTI_ClearITPendingBit(EXTI_Line17);
    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Event;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd =ENABLE;
    EXTI_Init(&EXTI_InitStructure);    
}

/**
  * @brief  Configures RTC clock source and prescaler.
  * @param  None
  * @retval : None
  */
void RTC_Configuration(void)
{
    /* RTC clock source configuration ------------------------------------------*/
    /* Allow access to BKP Domain */
    PWR_BackupAccessCmd(ENABLE);

    /* Reset Backup Domain */
    BKP_DeInit();

#ifdef USE_LSE
    /* Enable the LSE OSC */
    RCC_LSEConfig(RCC_LSE_ON);
    /* Wait till LSE is ready */
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
    {
    }

    /* Select the RTC Clock Source */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
#else
    RCC_LSICmd(ENABLE);
    /* Wait till LSE is ready */
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
    {
    }

    /* Select the RTC Clock Source */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
#endif
    /* Enable the RTC Clock */
    RCC_RTCCLKCmd(ENABLE);

    /* RTC configuration -------------------------------------------------------*/
    /* Wait for RTC APB registers synchronisation */
    RTC_WaitForSynchro();

    /* Set the RTC time base to  us ( = n/32768) */
    RTC_SetPrescaler(RTC_PRESCALE);

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();

    /* Enable the RTC Alarm interrupt */
    RTC_ITConfig(RTC_IT_ALR, ENABLE);
    
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
}
