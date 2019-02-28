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

#define BROADCAST_ADDR        (0xFFFF)
#define BROADCAST_PANID       (0xFFFF)
#define PACKET_SIZE           sizeof(RADIO_STAFF_CARD_LOC_T)

// Application states
#define IEEE_ADDR_LEN           (8)
#define IEEE_MAC_CHA_MIN        (0x0B)
#define IEEE_MAC_CHA_MAX        (0x1A)
#define IEEE_MAC_CHA_DEFAULT    (0x14)

#define LCD_DISPLAY_OFF         0
#define LCD_DISPLAY_TIME        1
#define LCD_DISPLAY_DATE        2
#define LCD_DISPLAY_ID			3
#define LCD_DISPLAY_SMS		    4
#define LCD_DISPLAY_CHARGE		5
#define LCD_DISPLAY_WEATHER     6
#define LCD_DISPLAY_SOS         7
#define LCD_DISPLAY_TUMBLE      8


#define BAND_CONNECT            0
#define BAND_DISCONNECT         1


#define BATTERYLE_LEVEL_1       36
#define BATTERYLE_LEVEL_2       38
#define BATTERYLE_LEVEL_3       39
#define BATTERYLE_LEVEL_4       40



// 启动后停留时间
#ifndef BOOT_DURATION_TIME
    #define BOOT_DURATION_TIME      (10000)
#endif

// 求救灯闪烁间隔时间
#ifndef HELP_FLASH_TIME
    #define HELP_FLASH_TIME         (500)
#endif

#ifndef RADIO_WAIT_TIME
#define RADIO_WAIT_TIME             (20)
#endif


#ifndef HELP_REPORT
    #define HELP_REPORT             (10)
#endif

// 求救上报次数
#ifndef HELP_REPORT_MAX
    #define HELP_REPORT_MAX     (HELP_REPORT * (LOCATE_TIME / HELP_FLASH_TIME))
#endif

// 低电门限，0.1v单位
#ifndef VDD_LIMITE
    #define VDD_LIMITE              (34)   //3.4v
#endif

// 按键事件定义
#ifndef KEY_HELP_TIME
    #define KEY_HELP_TIME           (2000)
#endif

#define LOCATE_TIME                 (REPORT_DELAY * 1000)           // to sec

#ifndef REPORT_VERINFO_TIME
#define REPORT_VERINFO_TIME        (10L * 60 * 1000)
#endif

#ifndef REPORT_VERINFO_CNT
#define REPORT_VERINFO_CNT          (3)
#endif

#ifndef CHECK_ADC_DELAY_TICK
    #define CHECK_ADC_DELAY_TICK    (600000)                    // 10min
#endif

#define CHECK_ADC_CNT               (5)

#ifndef FLASH_LED_TIME
    #define FLASH_LED_TIME          (6)
#endif

#ifndef SLEEP_TIME_MAX
    #define SLEEP_TIME_MAX          (LOCATE_TIME)
#endif

#ifndef SLEEP_TIME_MIN
    #define SLEEP_TIME_MIN          (2)
#endif

#endif
