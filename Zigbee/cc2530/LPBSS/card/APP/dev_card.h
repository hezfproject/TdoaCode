/*******************************************************************************
    Filename:     app_card.h

    Description:  地面系统应用程序头文件

*******************************************************************************/

#ifndef _APP_CARD_H_
#define _APP_CARD_H_

/*******************************************************************************
* INCLUDES
*/
#include <types.h>
#include "app_flash.h"

/*******************************************************************************
* TYPEDEFS
*/

/*******************************************************************************
* CONSTANTS AND DEFINES
*/
#define BROADCAST_ADDR        (0xFFFF)
#define BROADCAST_PANID       (0xFFFF)
#define PACKET_SIZE           sizeof(RADIO_DEV_CARD_LOC_T)

// Application states
#define IEEE_ADDR_LEN           (8)
#define IEEE_MAC_CHA_MIN        (0x0B)
#define IEEE_MAC_CHA_MAX        (0x1A)
#define IEEE_MAC_CHA_DEFAULT    (0x14)

// 启动后停留时间
#ifndef BOOT_DURATION_TIME
    #define BOOT_DURATION_TIME      (10000)
#endif

// 求救灯闪烁间隔时间
#ifndef HELP_FLASH_TIME
    #define HELP_FLASH_TIME         (500)
#endif

#ifndef HELP_REPORT
    #define HELP_REPORT             (10)
#endif

// 求救上报次数
#ifndef HELP_REPORT_MAX
    #define HELP_REPORT_MAX     (HELP_REPORT * (LOCATE_INTERVAL_TIME / HELP_FLASH_TIME))
#endif

// 低电门限，0.1v单位
#ifndef VDD_LIMITE
    #define VDD_LIMITE              (23)
#endif

// 按键事件定义
#ifndef KEY_HELP_TIME
    #define KEY_HELP_TIME           (2500)
#endif

#ifndef LOCATE_INTERVAL
#define LOCATE_INTERVAL             (20)
#endif

#define LOCATE_INTERVAL_TIME        (LOCATE_INTERVAL * 1000)           // to sec

#ifndef CHECK_ADC_DELAY_TICK
    #define CHECK_ADC_DELAY_TICK    (600000)                    // 10min
#endif

#define CHECK_ADC_CNT               (5)

#ifndef FLASH_LED_TIME
    #define FLASH_LED_TIME          (6)
#endif

#ifndef RADIO_WAIT_TIME
#define RADIO_WAIT_TIME             (20)
#endif

#ifndef KEY_PRESS_LONGLONG
#define KEY_PRESS_LONGLONG          (50)
#endif

#ifndef KEY_PRESS_LONG
#define KEY_PRESS_LONG              (10)
#endif

#ifndef KEY_PRESS_SHORT
#define KEY_PRESS_SHORT             (1)
#endif

#ifndef DISPLAY_STAGE_TIME
#define DISPLAY_STAGE_TIME          (1000)
#endif

#ifndef INFO_INTERVAL_TIME
#define INFO_INTERVAL_TIME          (10UL * 60 * 1000)
#endif

#ifndef INFO_REPORT_TIME
#define INFO_REPORT_TIME            (5000)
#endif

#endif
