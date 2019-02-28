#include "debug.h"
#include "led.h"
#include "NtrxDrv.h"
#include "cc_def.h"
#include "CommonTypes.h"
#include "stm32l1xx.h"

//重启前闪三下红灯
void DbgLedFlashReset(void)
{
    while (1)
    {
        LED_Red_On();
        LED_Green_On();
        Delay_ms(200);
        LED_Red_Off();
        LED_Green_Off();
        Delay_ms(200);
    }
}

/*******************************************************************************
* Function Name  : DbgAssertFailed
* Description    : Reports the name of the source file and the source line number
*                  where the assert error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void DbgAssertFailed(uint_8 *file, uint_32 line)
{
    ERR_DBG("\nWrong parameter value detected on\n");
    ERR_DBG("       file  %s\r\n", file);
    ERR_DBG("       line  %d\r\n", line);

    DbgLedFlashReset();
}

