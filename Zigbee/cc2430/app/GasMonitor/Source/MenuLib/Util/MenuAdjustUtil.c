#include "hal_key_cfg.h"
#include "key.h"
#include "lcd_serial.h"
#include "TimeUtil.h"
#include "MenuAdjustUtil.h"
#include "GasMonitor_MenuLibChinese.h"
#include "GasMenulib_global.h"
#include "GasMenulib_tree.h"
#include "GasMonitor.h"
#include "string.h"
#include "ch4.h"
#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Memory.h"
#include "temperature.h"


#define  AMPM_AM TRUE
#define  AMPM_PM FALSE


/* Adjust Time */
static Time_t             adjusttime_time;
static uint16              adjusttime_invpos;     // 0: minutes       1:seconds
#ifdef CFG_GAS_FIND_APPOINT_CARD
static uint16 appoint_card_number;
bool is_achieve_cardnumber=false;
#endif
static uint8                adjusttime_timeformat;
static bool                 adjusttime_ampm;

/* Adjust date */
static Date_t            adjustdate_date;
static uint16             adjustdate_invpos;   // 0: year       1: month      2: day

static uint16              adjust_num;
static uint8                adjust_numcnt;

/*Adjust volume */
static uint8                adjust_volsetting;

/* pad lock */
static bool                padlock_en = TRUE;
static uint8               padlock_stat = PADLOCK_UNLOCKED;


/* Adjust gasdensity */
static uint8               adjustgas_invpos;     // 0: 	1:	 2:
static uint16		   adjustgas_density = 100;
static bool		   adjustgas_result;

/* Adjust over density alert */
static uint16 		   adjustoveralert_density = 2;    // 2*50: 1%  from 0.5% to 2.5%

/*Adjust temperture */
static int16 		   adjusttemper ;    // 2*50: 1%  from 0.5% to 2.5%

#ifdef CFG_GAS_FIND_APPOINT_CARD
void	 GasMonitor_stationID_fromnum(uint16 *density,uint8 pNum10000,uint8 pNum1000,uint8 pNum100, uint8 pNum10,uint8 pNum1);
#endif

static void  circle_increase(uint16 *p, uint16 min, uint16 max);
static void  circle_decrease(uint16 *p, uint16 min, uint16 max);

static inline void reset_adjustnum(void);

inline static void reset_adjustnum()
{
    adjust_num = 0;
    adjust_numcnt= 0;
}
void   menu_setadjusttime(void )
{
    adjusttime_time = GetTime();
    adjusttime_timeformat = GetTimeFormat();
    adjusttime_invpos = 0;
    if(adjusttime_timeformat == TIME_FORMAT_12)
    {
        if(adjusttime_time.hour <12)
        {
            adjusttime_ampm = AMPM_AM;
        }
        else
        {
            adjusttime_time.hour -= 12;
            adjusttime_ampm = AMPM_PM;
        }

    }
    reset_adjustnum();

}
void   menu_setadjustdate(void )
{
    adjustdate_date = GetDate();
    adjustdate_invpos = 0;
    reset_adjustnum();
}
void    menu_adjusttime_display(void)
{
    uint8 X_START = 5;
    uint8 Y_START = 2;
    uint8 time_char[TIME_LEN];
    Time_t time = adjusttime_time;

    if(adjusttime_timeformat == TIME_FORMAT_12)
    {
        X_START--;
        if(adjusttime_ampm == AMPM_PM)
        {
            time.hour += 12;
        }
    }
    GetFullCharFromTime(time_char,time);

    LcdClearDisplay();
    LCD_Str_Print(TIME_CHINA, 6, 0, TRUE);
    LCD_Str_Print((uint8 *)time_char, X_START, Y_START, TRUE);
    if(adjusttime_invpos ==0)
    {
        LCD_Char_Inv(X_START, Y_START,2);
    }
    else if(adjusttime_invpos ==1)
    {
        LCD_Char_Inv(X_START+3, Y_START,2);
    }
    else
    {
        LCD_Char_Inv(X_START+6, Y_START,2);
    }

}
void    menu_adjusttime_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
    case HAL_KEY_UP:
        if(adjusttime_invpos == 0)
        {
            if(GetTimeFormat() == TIME_FORMAT_12)
            {
                circle_increase(&adjusttime_time.hour, 0, 11);
            }
            else
            {
                circle_increase(&adjusttime_time.hour, 0, 23);
            }
        }
        else if(adjusttime_invpos == 1)
        {
            circle_increase(&adjusttime_time.min, 0, 59);

        }
        else
        {
            if(adjusttime_timeformat == TIME_FORMAT_12)
            {
                adjusttime_ampm = !adjusttime_ampm;
            }
        }
        menu_display();
        break;
    case HAL_KEY_DOWN:
        if(adjusttime_invpos == 0)
        {
            if(GetTimeFormat() == TIME_FORMAT_12)
            {
                circle_decrease(&adjusttime_time.hour, 0, 11);
            }
            else
            {
                circle_decrease(&adjusttime_time.hour, 0, 23);
            }

        }
        else if(adjusttime_invpos == 1)
        {
            circle_decrease(&adjusttime_time.min, 0, 59);
        }
        else
        {
            if(adjusttime_timeformat == TIME_FORMAT_12)
            {
                adjusttime_ampm = !adjusttime_ampm;
            }
        }
        menu_display();
        break;

    case HAL_KEY_OK:
        if((adjusttime_invpos==2  &&  adjusttime_timeformat == TIME_FORMAT_12)
                || (adjusttime_invpos==1 &&  adjusttime_timeformat == TIME_FORMAT_24))
        {
            if(adjusttime_ampm == AMPM_PM && adjusttime_timeformat == TIME_FORMAT_12)
            {
                adjusttime_time.hour += 12;
            }
            SetTime(adjusttime_time);
            menu_setadjustdate();
            menu_JumptoMenu(MENU_ID_ADJUSTDATE);
            break;
        }

        if(adjusttime_timeformat == TIME_FORMAT_12)
        {
            circle_increase(&adjusttime_invpos, 0, 2);
        }
        else
        {
            circle_increase(&adjusttime_invpos, 0, 1);

        }

        SetTime(adjusttime_time);
        reset_adjustnum();
        menu_display();
        break;


    case HAL_KEY_MODE:
        menu_JumpBackWithMark();
        break;
    default:
        break;
    }

}
#ifdef CFG_GAS_FIND_APPOINT_CARD
void	 GasMonitor_stationID_fromdensity(uint8* pNum10000,uint8* pNum1000,uint8* pNum100, uint8* pNum10,uint8* pNum1,uint16 density)
{
    *pNum10000= density/10000;
    density -= (*pNum10000)*10000;
    *pNum10000 = (*pNum10000)%10;

    *pNum1000= density/1000;
    density -= (*pNum1000)*1000;

    *pNum100= density/100;
    density -= (*pNum100)*100;

    *pNum10 = density/10;
    density -= (*pNum10)*10;

    *pNum1 = density;
}
void	 GasMonitor_stationID_fromnum(uint16 *density,uint8 pNum10000,uint8 pNum1000,uint8 pNum100, uint8 pNum10,uint8 pNum1)
{
    *density=10000*pNum10000+1000*pNum1000+100*pNum100+10*pNum10+1*pNum1;
}
uint8 Get_adjustgas_invpos(void)
{
    return adjustgas_invpos;
}
void menu_find_appoint_card_onkey(uint8 keys, uint8 status)
{
    uint16 n1=0,n2=0,n3=0,n4=0,n5=0;
    uint16 max;
    GasMonitor_stationID_fromdensity((uint8*)&n1,(uint8*)&n2,(uint8*)&n3,(uint8*)&n4,(uint8*)&n5,appoint_card_number);
    switch(keys)
    {
    case HAL_KEY_UP:
    {
        if(adjustgas_invpos == 0)
        {
            circle_increase(&n1, 0, 6);
        }
        else if(adjustgas_invpos == 1)
        {
            if(n1==6)max=5;
            else max=9;
            circle_increase(&n2, 0, max);
        }
        else if(adjustgas_invpos == 2)
        {
            if((n1==6)&&(n2>4))max=5;
            else max=9;
            circle_increase(&n3, 0, max);
        }
        else if(adjustgas_invpos == 3)
        {
            if((n1==6)&&(n2>4)&&(n3>4))max=3;
            else max=9;
            circle_increase(&n4, 0, max);
        }
        else if(adjustgas_invpos == 4)
        {
            if((n1==6)&&(n2>4)&&(n3>4)&&(n4>2))max=5;
            else max=9;
            circle_increase(&n5, 0, max);
        }

        GasMonitor_stationID_fromnum(&appoint_card_number, n1,n2,n3,n4,n5);
        NearLastNodeID = CurrentNodeID;
        menu_display();
        break;
    }
    case HAL_KEY_DOWN:
    {
        if(adjustgas_invpos == 0)
        {
            circle_decrease(&n1, 0, 6);
        }
        else if(adjustgas_invpos == 1)
        {
            if(n1==6)max=5;
            else max=9;
            circle_decrease(&n2, 0, max);
        }
        else if(adjustgas_invpos == 2)
        {
            if((n1==6)&&(n2>4))max=5;
            else max=9;
            circle_decrease(&n3, 0, max);
        }
        else if(adjustgas_invpos == 3)
        {
            if((n1==6)&&(n2>4)&&(n3>4))max=3;
            else max=9;
            circle_decrease(&n4, 0, max);
        }
        else if(adjustgas_invpos == 4)
        {
            if((n1==6)&&(n2>4)&&(n3>4)&&(n4>2))max=5;
            else max=9;
            circle_decrease(&n5, 0, max);
        }
        GasMonitor_stationID_fromnum(&appoint_card_number, n1,n2,n3,n4,n5);
        NearLastNodeID = CurrentNodeID;
        menu_display();

        break;
    }
    case HAL_KEY_OK:
    {
        if(adjustgas_invpos < 4)
        {
            adjustgas_invpos++;
            NearLastNodeID = CurrentNodeID;
            menu_display();
        }
        else
        {
            adjustgas_invpos=0;
            is_achieve_cardnumber=true;
            //strcpy((char *)g_jump_buf,(char *)CALIBRATIONING_CHINA);
            //menu_JumptoMenu(MENU_ID_MAIN);
        }
        break;
    }
    case HAL_KEY_MODE:
    {
        if(adjustgas_invpos == 0)
        {
            adjustgas_invpos=4;
            //menu_JumpBackMarkParent();
        }
        else
        {
            adjustgas_invpos--;
        }
        NearLastNodeID = CurrentNodeID;
        menu_display();

        is_achieve_cardnumber=false;
        break;
    }
    }
}
uint16 Get_appoint_card_number(void)
{
    return appoint_card_number;
}
void Clear_appoint_card_number(void)
{
    appoint_card_number=0;
    adjustdate_invpos=0;
}
#endif
void    menu_adjustdate_display(void)
{
    const uint8 X_START = 3;
    const uint8 Y_START = 2;

    const uint8  year_pos = X_START;
    const uint8  month_pos = year_pos + 5;
    const uint8  day_pos = month_pos + 3;


    uint8 p[DATE_LEN];

    GetCharFromDate(p,adjustdate_date);
    LcdClearDisplay();
    LCD_Str_Print(DATE_CHINA, 6, 0, TRUE);
    LCD_Str_Print((uint8 *)p, X_START, Y_START, TRUE);

    if(adjustdate_invpos ==0)  // year
    {
        LCD_Char_Inv(year_pos, Y_START,4);
    }
    else if(adjustdate_invpos ==1)
    {
        LCD_Char_Inv(month_pos, Y_START,2);
    }
    else if(adjustdate_invpos ==2)
    {
        LCD_Char_Inv(day_pos, Y_START,2);
    }

}
void    menu_adjustdate_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {

    case HAL_KEY_UP:
    {
        if(adjustdate_invpos == 0)
        {
            circle_increase(&adjustdate_date.year, TIME_MIN_YEAR, TIME_MAX_YEAR);
        }
        else if(adjustdate_invpos == 1)
        {
            circle_increase(&adjustdate_date.mon, 1, 12);
        }
        else
        {
            circle_increase(&adjustdate_date.day, 1, GetMaxDayofMonth(adjustdate_date.year,adjustdate_date.mon));
        }
        menu_display();
        break;
    }
    case HAL_KEY_DOWN:
        if(adjustdate_invpos == 0)
        {
            circle_decrease(&adjustdate_date.year, TIME_MIN_YEAR, TIME_MAX_YEAR);
        }
        else if(adjustdate_invpos ==1)
        {
            circle_decrease(&adjustdate_date.mon, 1, 12);
        }
        else
        {
            circle_decrease(&adjustdate_date.day, 1, GetMaxDayofMonth(adjustdate_date.year, adjustdate_date.mon));
        }
        menu_display();
        break;

    case HAL_KEY_OK:
        if(adjustdate_invpos == 0)
        {
            if(adjustdate_date.year > TIME_MAX_YEAR)
                adjustdate_date.year = TIME_MAX_YEAR;
            if(adjustdate_date.year < TIME_MIN_YEAR)
                adjustdate_date.year = TIME_MIN_YEAR;
        }

        else if(adjustdate_invpos==2)
        {
            SetDate(adjustdate_date);
            strcpy((char *)g_jump_buf,SETTED_CHINA);
            menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
            break;

        }
        circle_increase(&adjustdate_invpos, 0, 2);
        reset_adjustnum();
        menu_display();
        break;

    case HAL_KEY_MODE:
        menu_JumpBackWithMark();
        break;
    default:
        break;
    }

}

#if 0
void menu_adjustvol_display(void)
{
    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        LCD_Str_Print((uint8 *)VOLUME_CHINA, (LCD_LINE_WIDTH-osal_strlen(VOLUME_CHINA))/2, 0, TRUE);
        adjust_volsetting = AudioGetOutputGain();
    }
    LCD_BigAscii_Print(0x90, 2, 2);
    LCD_BigAscii_Print(0x8B, 3, 2);

    for(uint8 i=0; i < adjust_volsetting; i++)
    {
        LCD_BigAscii_Print(0x0D, 5+2*i, 2);
        LCD_BigAscii_Print(0x0D, 5+2*i+1, 2);
    }
    for(uint8 i = adjust_volsetting; i<4; i++)
    {
        LCD_BigAscii_Print(0x00, 5+2*i, 2);
        LCD_BigAscii_Print(0x00, 5+2*i+1, 2);
    }
}
void menu_adjustvol_onkey(uint8 keys, uint8 status)
{
    switch(keys)
    {
        /*
        case HAL_KEY_SELECT:
        AudioSetOutputGain(adjust_volsetting);
        HalRingSetGain(RING_TYPE_SOUND, adjust_volsetting);
        menu_JumpBackWithMark();
        break;
        case HAL_KEY_BACKSPACE:
        menu_JumpBackWithMark();
        break;
        */
    case HAL_KEY_RIGHT:
        if(adjust_volsetting < MAX_GAIN_LEVEL-1)
        {
            adjust_volsetting++;
            AudioSetOutputGain(adjust_volsetting);
            HalRingSetGain(RING_TYPE_SOUND, adjust_volsetting);
        }
        NearLastNodeID = CurrentNodeID;
        menu_display();
        break;

    case HAL_KEY_LEFT:
        if(adjust_volsetting > 0)
        {
            adjust_volsetting--;
            AudioSetOutputGain(adjust_volsetting);
            HalRingSetGain(RING_TYPE_SOUND, adjust_volsetting);
        }
        NearLastNodeID = CurrentNodeID;
        menu_display();
        break;
    default:
        break;
    }
    // osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT, 1000);
    MineApp_StartMenuLibEvt (1000);
}

#endif

void HalSetPadLockStat(uint8 val)
{
    padlock_stat = val;
}
uint8  HalGetPadLockStat(void)
{
    return padlock_stat;
}
void HalSetPadLockEnable(bool val)
{
    padlock_en = val;
}
bool HalGetPadLockEnable(void )
{
    return padlock_en;
}

/* static functions */
static void circle_increase(uint16 *p, uint16 min, uint16 max)
{
    if(*p < max)
    {
        ++*p;
    }
    else
    {
        *p  =  min;
    }
}
static void circle_decrease(uint16 *p, uint16 min, uint16 max)
{
    if(*p > min)
    {
        --*p;
    }
    else
    {
        *p = max;
    }
}

static void temper_increase(int16 *p, int16 min, int16 max)
{
    if(*p < max)
    {
        *p+= 5;
    }
    else
    {
        *p  =  min;
    }
    *p = (*p/5)*5;
}
static void temper_decrease(int16 *p, int16 min, int16 max)
{
    if(*p > min)
    {
        *p-= 5;
    }
    else
    {
        *p = max;
    }
    *p = (*p/5)*5;
}


void    menu_calibration_setdensity_display(void)
{
    uint8 x,y;
    uint8 n1,n2,n3;

    /*
    ch4_getnum_fromdensity(&n1,&n2,&n3, adjustgas_density);
    ch4_getstr_fromdensity(densityStr,n1,n2,n3);
    LCD_Str_Print((uint8 *)densityStr, X_START, Y_START, TRUE);
    */

    ch4_getnum_fromdensity(&n1,&n2,&n3, adjustgas_density);
    menu_GasPrint(n1,n2,n3,true);

    if(FIRSTTIME_INTO_NODE())
    {
        char buf[] = "CH4";
        adjustgas_invpos=0;
        LcdClearDisplay();
        LCD_Str_Print(GASDENSITY_CHINA, 4, 0, TRUE);
        LCD_BigCharPrint('.',GASDISP_START_X+2, GASDISP_START_Y*LCD_LINE_HIGH);
        LCD_BigCharPrint('%', GASDISP_START_X+7, GASDISP_START_Y*LCD_LINE_HIGH);
        LCD_Str_Print_Pixel((uint8 *)buf, GASDISP_START_X+10, 1.8*LCD_LINE_HIGH);
        ch4_getnum_fromdensity(&n1,&n2,&n3, adjustgas_density);
        menu_GasPrint(n1,n2,n3,true);
    }

    if(adjustgas_invpos ==0)
    {
        x = GASDISP_START_X;
        y = GASDISP_START_Y;
        LCD_Area_Inv(x,x+1, 1.2*y*LCD_LINE_HIGH,(y+1.8)*LCD_LINE_HIGH);
    }
    else if(adjustgas_invpos ==1)
    {
        x = GASDISP_START_X+3;
        y = GASDISP_START_Y;
        LCD_Area_Inv(x,x+1, 1.2*y*LCD_LINE_HIGH,(y+1.8)*LCD_LINE_HIGH);
    }
    else if(adjustgas_invpos == 2)
    {
        x = GASDISP_START_X+5;
        y = GASDISP_START_Y;
        LCD_Area_Inv(x,x+1, 1.2*y*LCD_LINE_HIGH,(y+1.8)*LCD_LINE_HIGH);

    }

}
void    menu_calibration_setdensity_onkey(uint8 keys, uint8 status)
{
    uint16 n1=0,n2=0,n3=0;
    ch4_getnum_fromdensity((uint8*)&n1,(uint8*)&n2,(uint8*)&n3,adjustgas_density);
    switch(keys)
    {
    case HAL_KEY_UP:
    {
        if(adjustgas_invpos == 0)
        {
            circle_increase(&n1, 0, 3);
        }
        else if(adjustgas_invpos == 1)
        {
            circle_increase(&n2, 0, 9);
        }
        else if(adjustgas_invpos == 2)
        {
            circle_increase(&n3, 0, 9);
        }
        ch4_getdensity_fromnum(&adjustgas_density, n1,n2,n3);
        NearLastNodeID = CurrentNodeID;
        menu_display();
        break;
    }
    case HAL_KEY_DOWN:
    {
        if(adjustgas_invpos == 0)
        {
            circle_decrease(&n1, 0, 3);
        }
        else if(adjustgas_invpos == 1)
        {
            circle_decrease(&n2, 0, 9);
        }
        else if(adjustgas_invpos == 2)
        {
            circle_decrease(&n3, 0, 9);
        }
        ch4_getdensity_fromnum(&adjustgas_density, n1,n2,n3);
        NearLastNodeID = CurrentNodeID;
        menu_display();

        break;
    }
    case HAL_KEY_OK:
    {
        if(adjustgas_invpos < 2)
        {
            adjustgas_invpos++;
            NearLastNodeID = CurrentNodeID;
            menu_display();
        }
        else
        {
            if( (ch4_set_cal_value(adjustgas_density) == 0)
                    && (ch4_calibration()==0) )
            {
                adjustgas_result = true;
            }
            else
            {
                adjustgas_result = false;
            }

            strcpy((char *)g_jump_buf,(char *)CALIBRATIONING_CHINA);
            menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
        }
        break;
    }
    case HAL_KEY_MODE:
    {
        if(adjustgas_invpos == 0)
        {
            menu_JumpBackMarkParent();
        }
        else
        {
            adjustgas_invpos--;
            NearLastNodeID = CurrentNodeID;
            menu_display();
        }
        break;
    }
    }
}
void    menu_settings_overdensity_display(void)
{
    uint8 n1,n2,n3;

    if(FIRSTTIME_INTO_NODE())
    {
        char buf[] = "CH4";
        adjustgas_invpos=0;
        LcdClearDisplay();
        LCD_Str_Print(OVERDENSITY_CHINA, 2, 0, TRUE);
        LCD_BigCharPrint('.',GASDISP_START_X+2, GASDISP_START_Y*LCD_LINE_HIGH);
        LCD_BigCharPrint('%', GASDISP_START_X+7, GASDISP_START_Y*LCD_LINE_HIGH);
        LCD_Str_Print_Pixel((uint8 *)buf, GASDISP_START_X+10, 1.8*LCD_LINE_HIGH);
        ch4_getnum_fromdensity(&n1,&n2,&n3, adjustoveralert_density*50);
        menu_GasPrint(n1,n2,n3,true);
    }
    else
    {
        ch4_getnum_fromdensity(&n1,&n2,&n3, adjustoveralert_density*50);
        menu_GasPrint(n1,n2,n3,true);

    }
    LCD_Area_Inv(GASDISP_START_X,GASDISP_START_X+6, 1.2*GASDISP_START_Y*LCD_LINE_HIGH,(GASDISP_START_Y+1.8)*LCD_LINE_HIGH);
}
void    menu_settings_overdensity_onkey(uint8 keys, uint8 status)
{
    uint16 n1=0,n2=0,n3=0;
    ch4_getnum_fromdensity((uint8*)&n1,(uint8*)&n2,(uint8*)&n3,adjustoveralert_density*50);
    switch(keys)
    {
    case HAL_KEY_UP:
    {
        circle_increase(&adjustoveralert_density, 1, 5);
        NearLastNodeID = CurrentNodeID;
        menu_display();
        break;
    }
    case HAL_KEY_DOWN:
    {
        circle_decrease(&adjustoveralert_density, 1, 5);
        NearLastNodeID = CurrentNodeID;
        menu_display();
        break;
    }
    case HAL_KEY_OK:
    {
        //menu_steptoparent();
        strcpy((char *)g_jump_buf,SETTING_CHINA);
        menu_JumpandMark(MENU_ID_SHOWMESSAGE);
        break;
    }
    case HAL_KEY_MODE:
    {
        menu_steptoparent();
    }
    }
}

bool menu_get_calibration_result(void)
{
    return adjustgas_result;
}

void menu_set_adjustdensity(uint16 adjustdensity)
{
    adjustgas_density = adjustdensity;
    return ;
}
uint16 menu_get_adjustdensity(void)
{
    return adjustgas_density;
}

uint16 menu_get_overalert_density(void)
{
    return adjustoveralert_density*50;
}

uint8   menu_set_overalert_density(uint16 density)
{
    if(density>=OVERDENSITY_MIN  && density<=OVERDENSITY_MAX)
    {
        adjustoveralert_density = density/50;
        return ZSuccess;
    }
    return ZInvalidParameter;
}

void    menu_settings_temperature_display(void)
{
    if(FIRSTTIME_INTO_NODE())
    {
        LcdClearDisplay();
        LCD_Str_Print((uint8 *)TEMPERATURE_CALIB_CHINA, (LCD_LINE_WIDTH-osal_strlen((char *)TEMPERATURE_CALIB_CHINA))/2, 0, TRUE);
        LCD_BigCharPrint('.',TEMPERDISP_START_X+6, TEMPERDISP_START_Y*LCD_LINE_HIGH);
        LCD_BigCharPrint('C', TEMPERDISP_START_X+9, TEMPERDISP_START_Y*LCD_LINE_HIGH);
        menu_TemperPrint(adjusttemper, true);
    }
    else
    {
        menu_TemperPrint(adjusttemper, false);
    }
    if(adjusttemper>=0 &&adjusttemper<100)
    {
        LCD_Area_Inv(TEMPERDISP_START_X+4,TEMPERDISP_START_X+8, 1.2*TEMPERDISP_START_Y*LCD_LINE_HIGH,(TEMPERDISP_START_Y+1.8)*LCD_LINE_HIGH);
    }
    else if(adjusttemper>-100 &&adjusttemper<0 || adjusttemper>=100)
    {
        LCD_Area_Inv(TEMPERDISP_START_X+2,TEMPERDISP_START_X+8, 1.2*TEMPERDISP_START_Y*LCD_LINE_HIGH,(TEMPERDISP_START_Y+1.8)*LCD_LINE_HIGH);
    }
    else if(adjusttemper<=-100)
    {
        LCD_Area_Inv(TEMPERDISP_START_X,TEMPERDISP_START_X+8, 1.2*TEMPERDISP_START_Y*LCD_LINE_HIGH,(TEMPERDISP_START_Y+1.8)*LCD_LINE_HIGH);
    }
}

void    menu_settings_temperature_onkey(uint8 keys, uint8 status)
{
    //tem_getnum_fromtemper(str,adjusttemper);

    switch(keys)
    {
    case HAL_KEY_UP:
    {
        temper_increase(&adjusttemper, TEM_CALIBTEMP_MIN, TEM_CALIBTEMP_MAX);
        NearLastNodeID = CurrentNodeID;
        menu_display();
        break;
    }
    case HAL_KEY_DOWN:
    {
        temper_decrease(&adjusttemper, TEM_CALIBTEMP_MIN, TEM_CALIBTEMP_MAX);
        NearLastNodeID = CurrentNodeID;
        menu_display();
        break;
    }
    case HAL_KEY_OK:
    {
        //menu_steptoparent();
        tem_calibration(adjusttemper);
        strcpy((char *)g_jump_buf,SETTING_CHINA);
        menu_JumpandMark(MENU_ID_SHOWMESSAGE);
        break;
    }
    case HAL_KEY_MODE:
    {
        menu_steptoparent();
    }
    }
}

bool menu_set_temperature(int16 temper)
{
    if(temper<TEM_CALIBTEMP_MIN ||temper>TEM_CALIBTEMP_MAX )
    {
        return false;
    }
    adjusttemper = temper;
    return true;
}

