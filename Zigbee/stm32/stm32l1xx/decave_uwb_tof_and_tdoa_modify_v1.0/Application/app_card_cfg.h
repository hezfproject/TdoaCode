/*******************************************************************************
    Filename:     app_card_cfg.h

    Description:  地面精确定位系统应用程序头文件

*******************************************************************************/

#ifndef _APP_CARD_CFG_H_
#define _APP_CARD_CFG_H_

/*******************************************************************************
* INCLUDES
*/

/*******************************************************************************
* TYPEDEFS
*/

/*******************************************************************************
* CONSTANTS AND DEFINES
*/
#define OPEN_SLEEP

// 启动后停留一段时间再开始上报，等待电压检测数据
#define BOOT_DURATION_TIME          2000

#define LED_FLASH_TIME              20      //LED灯闪烁时长，单位ms

//定位上报时间
#define LOC_REPORT_HIGH_PERIOD      2000            //范围: 1000~10000
#define LOC_REPORT_LOW_PERIOD       (5L*60*1000)    //5分钟

#define MAX_RANGING_FAIL_COUNT      5
#define CLOSE_SLEEP_FOR_RCV_TIME    10      //收到定位回复带数据标志后，关休眠等待接收的时间

#define BEACON_RCV_DURATION         55     //发beacon后，等待收beacon回复的时间
#define BEACON_SEND_EXPIRE_HIGH_TIME    10000
#define BEACON_SEND_EXPIRE_LOW_TIME     (10L*60*1000)   //10分钟

//版本上报周期
#define VERSION_REPORT_HIGH_PERIOD      (10L*60*1000)   //10分钟
#define VERSION_REPORT_LOW_PERIOD       (60L*60*1000)   //60分钟

//电压检测
#define ADC_CHECK_HIGH_PERIOD           (10L*60*1000)   //10分钟
#define ADC_CHECK_LOW_PERIOD            (60L*60*1000)   //60分钟
//#define ADC_CHECK_TICK              500
//#define ADC_CHECK_CNT               1

//低电
#define VDD_LIMIT                   36          // 门限，0.1v单位
#define HUNGER_FLASH_PERIOD         1000        // 低电，红灯闪烁间隔时间
#define HUNGER_FLASH_TIME           15000       // 低电，红灯闪烁总时间
#define MAX_HUNGER_FLASH_CNT        (HUNGER_FLASH_TIME/HUNGER_FLASH_PERIOD)

// 求救
#define BUTTON_PRESS_HELP_TIME      2500    // 求救按键时长
#define BUTTON_PRESS_TEST_PERIOD    100     // 长按过程检查周期
#define HELP_MSG_REPORT_PERIOD      2000    // 上报求救信息周期
#define HELP_FLASH_RED_PERIOD       500
#define HELP_FLASH_GREEN_PERIOD     500

#define MIN_HELP_TIMES              3       // 响3次
#define MAX_HELP_TIMES              (MIN_HELP_TIMES + 12)      // 响15次
#define HELP_RESPONSED_BEEP_FLASH_TIMES   (MAX_HELP_TIMES + 3)

//撤离
#define MAX_URGENT_ID_NUM           5       // 缓存撤离id的个数

#define URGENT_BEEP_RED_FLASH_PERIOD    200
#define URGENT_CNF_IMMUNITY_NEW_TIME    30000
#define URGENT_CNF_IMMUNITY_OLD_TIME    (15L*60*1000)

#define URGENT_CNF_REPORT_TIMES 3

// motion detection
#define MOTION_DETECT_PERIOD            (1L*30*1000)    //30秒
#define REST_TO_LOW_POWER_TIME          (1L*60*1000)    //1分钟，卡静止后到省电状态的时间
#define MOTION_DETECT_CNT_MAX           (REST_TO_LOW_POWER_TIME/MOTION_DETECT_PERIOD)
#endif

