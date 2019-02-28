#include "CC_DEF.h"


void SleepInit(void);


void Sleep(uint32 ms);


/**
  * @brief  Configures EXTI Line17(RTC Alarm).
  * @param  None
  * @retval : None
  */
void EXTI_Configuration(void);

/**
  * @brief  Configures RTC clock source and prescaler.
  * @param  None
  * @retval : None
  */
void RTC_Configuration(void);
