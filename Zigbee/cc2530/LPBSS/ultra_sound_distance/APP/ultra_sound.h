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

/*******************************************************************************
* TYPEDEFS
*/

/*******************************************************************************
* CONSTANTS AND DEFINES
*/

// Application states
#define IEEE_ADDR_LEN           (8)
#define IEEE_MAC_CHA_MIN        (0x0B)
#define IEEE_MAC_CHA_MAX        (0x1A)
#define IEEE_MAC_CHA_DEFAULT    (0x14)

#define BATTERYLE_LEVEL_1       36
#define BATTERYLE_LEVEL_2       38
#define BATTERYLE_LEVEL_3       39
#define BATTERYLE_LEVEL_4       40


// 启动后停留时间
#ifndef BOOT_DURATION_TIME
    #define BOOT_DURATION_TIME      (10000)
#endif

// 低电门限，0.1v单位
#ifndef VDD_LIMITE
    #define VDD_LIMITE              (34)   //3.4v
#endif

// 按键事件定义
#ifndef KEY_HELP_TIME
    #define KEY_HELP_TIME           (2000)
#endif

#ifndef CHECK_ADC_DELAY_TICK
    #define CHECK_ADC_DELAY_TICK    (600000)                    // 10min
#endif

#define CHECK_ADC_CNT               (5)

#ifndef SLEEP_TIME_MAX
    #define SLEEP_TIME_MAX          (LOCATE_TIME)
#endif

#ifndef SLEEP_TIME_MIN
    #define SLEEP_TIME_MIN          (2)
#endif

#endif
