/*******************************************************************************
  Filename:     app_card.c

  Description:  应用程序主文件

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <bsp.h>
#include <bsp_led.h>
#include <bsp_key.h>
#include <hal_lcd.h>
#include <hal_adc.h>
#include <ultra_sound.h>
#include <lpbssnwk.h>
#include <hal_button.h>
#include <hal_timer.h>
#include <mma8452q.h>
#include <hal_ultra_sound.h>
#include <string.h>
#include <mem.h>
#include <timer_event.h>
#include <track.h>
#include <hal_mcu.h>
#include "app_flash.h"
#include "version.h"

//tieshan
#include <hal_io_iic.h>

#include "..\..\..\..\..\common\crc.h"


#include "sms.h"
#include "bsp_beep.h"

/*******************************************************************************
 *			                                      MARCO
 */
#define FF_MT_SRC	   0x16
#define TRANSIENT_SRC  0x1E
#define PULCE_SRC      0x22

/*******************************************************************************
* CONSTANTS
*/
#ifdef USE_HEAP
#define HEAP_SIZE       (4 * 1024)
UINT8 heap_pool[HEAP_SIZE];

#define CC2530_HEAP_BEGIN   (void *)(heap_pool)
#define CC2530_HEAP_END     (void *)(&heap_pool[HEAP_SIZE])
#endif

enum status_e {INVALID, LIGHT_ON, LIGHT_CLOSE};

uint16 endDevID = 0xFFFF;   //卡自己的ID
uint16 midDevID = 0xFFFF;   //卡座ID
LPBSS_device_ID_e endDevType;  //卡自己是设备卡还是人员卡

extern uint8 recframe[];//128
extern uint8 sendframe[];
extern uint8 sendframeLen;

uint8 display_delay = 0;
uint8 display_motionCount = 0;

uint8 battery_level = 0;;
uint8 alarm_cnt = 0;

uint8 status = 0;
uint32 light_count = 0;



/*******************************************************************************
* LOCAL FUNCTIONS DECLARATION
*/
static UINT16 app_BCDToDec(UINT8 u8Byte);
static UINT16 app_CheckIEEEInfo(UINT8 *pu8IEEEAddr);
static void   app_get_battery_level(UINT32 u32AdcValue);

/*******************************************************************************
* LOCAL VARIABLES
*/
/*******************************************************************************
* LOCAL FUNCTIONS DEFINED
*/

/*******************************************************************************
* @fn          app_BCDToDec
*
* @brief       将一个字节的整数转换成由这个整数高4位和低四位组成的
*              十进制数
* @param    input-   u8Byte - 将进行转换的数
*
*
* @return      转换结果
*/
static UINT16 app_BCDToDec(UINT8 u8Byte)
{
    UINT16 u16ShortAddr = 0;

    ASSERT(HI_UINT8(u8Byte) < 0xA);
    ASSERT(LO_UINT8(u8Byte) < 0xA);
    u16ShortAddr += HI_UINT8(u8Byte);
    u16ShortAddr *= 10;
    u16ShortAddr += LO_UINT8(u8Byte);

    return u16ShortAddr;
}

/*******************************************************************************
* @fn          app_CheckIEEEInfo
*
* @brief       检验读出来的IEEE 地址是否符合规定
*
* @param    input-   pu8IEEEAddr - 指向传过来的IEEE地址
*
*
* @return      如果全部检测通过那么返回一个设备段地址
*/
static UINT16 app_CheckIEEEInfo(UINT8 *pu8IEEEAddr)
{
    UINT32 u32ShortAddr;

    ASSERT(pu8IEEEAddr);
    // ASSERT(EXT_LPBSS_MAC_TYPE_CARD == pu8IEEEAddr[EXT_LPBSS_MAC_TYPE]);
    ASSERT(!pu8IEEEAddr[LPBSS_MAC_CHA]
            || IEEE_MAC_CHA_MIN <= pu8IEEEAddr[LPBSS_MAC_CHA]);
    ASSERT(IEEE_MAC_CHA_MAX >= pu8IEEEAddr[LPBSS_MAC_CHA]);
    ASSERT(pu8IEEEAddr[LPBSS_MAC_CARD_TYPE] == WRIST_CARD_DEVICE_ID);
    endDevType = (LPBSS_device_ID_e)pu8IEEEAddr[LPBSS_MAC_CARD_TYPE];

    // delete HIGH-4 bit of the third Byte
    u32ShortAddr = app_BCDToDec(0x0F & pu8IEEEAddr[LPBSS_MAC_DEVID_H4BIT]);
    u32ShortAddr *= 100;
    u32ShortAddr += app_BCDToDec(pu8IEEEAddr[LPBSS_MAC_DEVID_M8BIT]);
    u32ShortAddr *= 100;
    u32ShortAddr += app_BCDToDec(pu8IEEEAddr[LPBSS_MAC_DEVID_L8BIT]);
    ASSERT(u32ShortAddr >= LPBSS_DEVID_MIN);
    ASSERT(u32ShortAddr <= LPBSS_DEVID_MAX);

    return (UINT16)(u32ShortAddr & 0xFFFF);
}


/*******************************************************************************
* @fn          app_Delay
*
* @brief       毫秒级延时
*
* @param      input - timeout 延时的长度
*
* @return      none
*/
/* timeout is  in ms */
static VOID app_Delay(UINT16 timeout)
{
    uint16 i, j, k;
    uint16 timeBig =  timeout >> 9;
    uint16 timeSmall = timeout - timeBig * 512;

    for (i = 0; i < timeBig; i++)
    {
#ifdef OPEN_WTD
        BSP_WATCHDOG_Feed();
#endif
        for (j = 0; j < 512; j++)
        {
            /* One Nop counts 12/32M, So 889  cyc is a ms*/
            k = 880;//k = 889;
            while (k--)
            {
                asm("NOP");
                asm("NOP");
                asm("NOP");
            }
        }
    }
#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
    for (i = 0; i < timeSmall; i++)
    {
        k = 880;

        while (k--)
        {
            asm("NOP");
            asm("NOP");
            asm("NOP");
        }
    }
}

static void delay_us(unsigned int us)
{
    unsigned int i;
    for (i = 0; i < us; ++i) {
        asm("nop");
        asm("nop");
    }
}

/***********************************************************************************
* @fn          app_TestKeyPress
*
* @brief       毫秒级延时
*
* @param      input - timeout 按键确认的时间，单位MS
*
* @return      如果按键达到确认时间则返回true,否则返回false
*/
/* Test If a key press is a long press */
BOOL app_TestKeyPress(UINT16 TimeOut)
{
    UINT16 testInterval = 300;   // test once each 300 ms
    UINT16 testnum = TimeOut / testInterval;

    for (uint16 i = 0; i < testnum; i++)
    {
        app_Delay(testInterval);

        if(!BSP_KEY_IsDown())   // low voltage when key press
        {
            return false;
        }
    }

    return true;
}

/*
* 电量检测
*/
static void app_AdcCheck(void)
{

}

/*
** 定功耗休眠
*/
void app_savepower(void)
{
}

static void app_event_proc(void)
{
}

static void app_get_battery_level(UINT32 u32AdcValue)
{
}

static void sound_detech(unsigned char slave_id, unsigned char cmd_type)
{
}

static void app_light_handle(void)
{
#define LED_GREEN_PIN  P0_1
#define LED_GREEN_BIT  BV(1)
#define LED_GREEN_DIR  P0DIR
#define LED_GREEN_SEL  P0SEL

    if (LIGHT_ON == status) {
        //let P0.1 LED_GREEN light

        if (0 == light_count++) {
            LED_GREEN_SEL &= ~(LED_GREEN_BIT);
            LED_GREEN_DIR |= LED_GREEN_BIT;    //set p0_1 dir output
            LED_GREEN_PIN  = 0;                // set P0_1 low ttl 
        } else if (light_count > 8000) {
            light_count = 0;
            status = LIGHT_CLOSE;
        }

    } else if (LIGHT_CLOSE == status) {

        LED_GREEN_SEL &= ~(LED_GREEN_BIT);
        LED_GREEN_DIR |= LED_GREEN_BIT;    // set p0_1 dir output
        LED_GREEN_PIN = 1;                 //set P0_1 high ttl 

        status = INVALID;
    }
}

static void app_red_light_flash(int distance)
{
#define LED_RED_PIN  P0_1
#define LED_RED_BIT  BV(1)
#define LED_RED_DIR  P0DIR
#define LED_RED_SEL  P0SEL

    int count = 0, level = 0;
    static int cur_count = 0;

    //count = (distance / 100 + 1) * 2;
    level = distance / 500;
    count = 4 + 2*level - 1;

    LED_RED_SEL &= ~(LED_RED_BIT);
    LED_RED_DIR |= LED_RED_BIT;    //set p0_0 dir output

    if (0 == cur_count++) {
        LED_RED_PIN = 0;              // set P0_0 low ttl 
    } else if (cur_count == count / 2 + 1) {
        LED_RED_PIN = 1;              // set P0_0 high ttl
    } else if (cur_count >= count) {
        cur_count = 0;
    }
}


static void app_create_40khz_square(void)
{
//#define ULTRA_PWM      P1_6
//#define ULTRA_PWM_SEL  P1SEL
//#define ULTRA_PWM_DIR  P1DIR
//#define ULTRA_PWM_BIT  BV(6)

#define ULTRA_PWM      P2_0
#define ULTRA_PWM_SEL  P2SEL
#define ULTRA_PWM_DIR  P2DIR
#define ULTRA_PWM_BIT  BV(0)

    unsigned char old_IEN0;

    old_IEN0 = IEN0;

    ULTRA_PWM_SEL &= ~(ULTRA_PWM_BIT); //set PWM(P1_6) is gpio
    ULTRA_PWM_DIR |=  (ULTRA_PWM_BIT); //set PWM(P1_6) is output status


    old_IEN0 = IEN0;
    IEN0 = 0; //关中断

    ULTRA_PWM = 1;
    delay_us(7);
    ULTRA_PWM = 0;
    delay_us(7);
    ULTRA_PWM = 1;
    delay_us(7);
    ULTRA_PWM = 0;
    delay_us(7);

    ULTRA_PWM = 1;
    delay_us(7);
    ULTRA_PWM = 0;
    delay_us(7);
    ULTRA_PWM = 1;
    delay_us(7);
    ULTRA_PWM = 0;
    delay_us(7);

    IEN0 = old_IEN0; //恢复中断
}

static void app_hot_view_interrupt(void)
{
    if (INVALID == status)
        status = LIGHT_ON;
}


/*******************************************************************************
* @fn          main
* @brief       This is the main entry of the  application.
* @param       none
* @return      none
*/
void main (void)
{
    BSP_BoardInit();

#ifdef USE_HEAP
    rt_system_heap_init(CC2530_HEAP_BEGIN, CC2530_HEAP_END);
#endif

    //event_timer_init();
    //HalAdcInit();
    //BSP_LED_Init();
    //HalLcdInit();
    //BSP_KEY_Init();

#ifdef _DEBUG_TICK
    /*
    DEBUG_PIN_SEL &= ~0x01;
    DEBUG_PIN_DIR |=  0x01;

    DEBUG_PIN_H;
    */
#endif

    //tieshan
    HAL_I2C_Init();
    HAL_HotView_Init(app_hot_view_interrupt);

    //app_create_40khz_square();

    HAL_Timer4_Start(hal_timer4_interrupt);
    //app_red_light_flash(1000);

    while (1)
    {
        UINT16 value = 0xFFFF; 
        UINT16 err_value = 0;

        //if (LIGHT_ON == status)
        value = HAL_I2C_ReadValue(0xe8, 2, 0xb4);

        if (0XFFFF != value && 0 != value) {
            //HAL_Timer4_Stop();
            //app_light_handle();
            if (value < 1000)
                app_red_light_flash(value);
        }

        //delay_us(5);
#ifdef OPEN_WTD
        BSP_WATCHDOG_Feed();
#endif

#ifdef OPEN_SLEEP
        //if(display_status == LCD_DISPLAY_OFF)
        {
            app_savepower();
        }
#endif
    }
}
