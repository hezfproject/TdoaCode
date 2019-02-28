#include <htc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "spi.h"
#include "afe.h"
#include "timer.h"
#include "PIC639_Type.h"
#include "..\..\common\LPBSS\RadioProto.h"
// 熔丝配置参数

//__CONFIG(INTIO & WDTDIS & PWRTEN & MCLRDIS & UNPROTECT & BORXSLP & IESODIS & FCMDIS & WAKECNT);
__CONFIG(INTIO & WDTDIS & PWRTDIS & MCLRDIS & UNPROTECT & BORDIS & IESODIS & FCMDIS & WAKECNT);
//__CONFIG(LP & WDTDIS & PWRTDIS & MCLRDIS & UNPROTECT & BORDIS & IESODIS & FCMDIS & WAKECNT);


//由于无法控制中断的优先级别，所以同时收发存在问题，且也没有必要同时收发
typedef enum
{
    SYSTEM_IDLE =0,
    SYSTEM_REC  =1,
    SYSTEM_SEND =2,
}system_status;


/*************************************************
 *              EVENT
 */
#define  EVENT_PARSE_LF_BUFF    0x01
#define  EVENT_TO_WAKE_CARD     0x02

/*************************************************
 *              MARCO
 */
//#define LF_DATA_LEN 5
#define LF_DATA_START  1

//#define DEBUG_TEST

#ifdef DEBUG_TEST
#define DEBUG_PIN   RA5
#endif
#define DATA_PIN    RC0
#define SCL_PIN    RA1
#define SCL_H    {SCL_PIN =1;}
#define SCL_L    {SCL_PIN =0;}
#define OUT_H    {DATA_PIN =1;SCL_L;}
#define OUT_L    {DATA_PIN =0;SCL_L;}
#define DATA_OUT_IDLE   {SCL_H;}


/*************************************************
 *              VARIABLE
 */
static uint16 Timer0 = 0;
static uint8  sentFlag = 0;
static uint16 sRecTimer = 0;
static uint16 recBitLen = 0;
static uint16 sendBitLen = 0;
static uint16 sysEvent = 0;
system_status sysStatus = SYSTEM_IDLE;
static uint8 LFRecBuff[LF_DATA_LEN+LF_DATA_START+1];
static uint8 LFData[LF_DATA_LEN+LF_DATA_START+1];
static uint8 LFSendData[LF_DATA_LEN+1];

/*************************************************
 *              FUNCTION
 */
void Init_LFReceiver(void);
void start_LFReceiver(void);
void stop_LFReceiver(void);
void Start_Event(unsigned int event);
void Stop_Event(unsigned int event);
void Parse_LF_Data(void);
void Send_Data_to_Card(void);
void Data_to_Manchester_code(uint8 *pManchCode,const uint8 *pdata,uint16 datalen,uint8 polar);
void LF_Data_Sent(void);
void LF_Data_Rec(void);
void Reset_State_to_Idle(void);

/****************************************************************/
void main()
{
	unsigned int val=0;

	OPENLED;
	OSC_INIT();
	PORT_INIT();
	init_Timer0();

	delay(1);
	CLOSELED;

	SET_AFE();

	OPENLED;
	delay(1);
	CLOSELED;
	delay(1);
	DATA_OUT_IDLE;

    Init_LFReceiver();
    asm("sleep");

#ifdef DEBUG_TEST
    DEBUG_PIN  = 1;
#endif

	while(1)
	{
	    static uint16 i = 0;
	     static unsigned int state = 0;

        if(i >= sizeof(unsigned int))i = 0;

	    state = sysEvent&(0x01<<i++);
        switch(state)
        {
           case EVENT_PARSE_LF_BUFF:
                Parse_LF_Data();
                memset(LFRecBuff,0,sizeof(LFRecBuff));
                break;

           case EVENT_TO_WAKE_CARD:
                Send_Data_to_Card();
                break;

           default:
                break;
        }
        Stop_Event(state);

	}
}

/******************************************************************************
 *   响应低频输入响应(RA2);
 *   上升沿触发中断；
 */
void Init_LFReceiver(void)
{
    RA2 = 0;
    CMCON0 |=  0x07;
    INTEDG = 1;
    GIE = 1;
    start_LFReceiver();
}

/******************************************************************************
 *   打开响应低频输入响应(RA2);
 *   上升沿触发中断；
 */
void start_LFReceiver(void)
{
    INTE = 1;

}
/******************************************************************************
 *   关闭响应低频输入响应(RA2);
 *   上升沿触发中断；
 */
void stop_LFReceiver(void)
{
    INTE = 0;

}


/******************************************************************************
 *   中断服务函数;
 *   处理LF输入中断服务和定时器0溢出中断服务；
 */
void interrupt INT_SERVICE(void)
{


    if(T0IF)
    {
        T0IF = 0;
        TMR0 = TIMER0FOOD;

        LF_Data_Sent();



        /*
         *  Make sure that the timer0 will stop ;
         */
    #if (RECEIVE_FRE == 50000)
        /* samping timer,decide by samping frequency;
         * *8->bits*4->1010000bits*5->the max count
         */
        if(++Timer0 > (LF_DATA_LEN*8*4*5-10))
    #else
        "you need set a value as sampling frequency"
    #endif
        {
            if(sysStatus == SYSTEM_REC)
            {
                if(40 <= recBitLen)
                {
                    LFRecBuff[LF_DATA_LENORMATCH] = LF_DATA_LEN;
                    Start_Event(EVENT_PARSE_LF_BUFF);
                #ifdef DEBUG_TEST
                    DEBUG_PIN  = 0;
                #endif
                }
                else
                {
                    asm("sleep");
                    asm("nop");
                    asm("nop");
                }
            }
            Reset_State_to_Idle();
        }
    }

    if(INTF)
    {
        INTF = 0;

        if(sysStatus == SYSTEM_IDLE)
        {
            sRecTimer = 0;
            recBitLen = 0;
            Timer0 = 0;
            sysStatus = SYSTEM_REC;
            start_Timer0();
        }

        LF_Data_Rec();
    }

}

/******************************************************************************
 *   设置系统EVENT;
 *   由于是简单应用，没有提供时间支持；
 */
void Start_Event(unsigned int event)
{
    if(event)
    {
        sysEvent |= event;
    }
}

/******************************************************************************
 *   清除系统EVENT;
 *   由于是简单应用，没有提供时间支持；
 */
void Stop_Event(unsigned int event)
{
    if(event)
    {
        sysEvent &=~ event;
    }
}

/******************************************************************************
 *   处理低频接收到的数据;
 *
 */
void Parse_LF_Data(void)
{
    static uint16 sDevID  = 0x0000;
    static uint16 sCardID = 0xffff;
    static uint8 sCmdandType = 0;
    static uint8 changeCardCount = 1;
    static uint8 changeDevCount = 1;
    static uint8 changeCmdCount = 1;

    if(sCardID != (LFRecBuff[LF_DATA_CARDID_H]<<8|LFRecBuff[LF_DATA_CARDID_L]))
    {
        changeCardCount++;
    }

    if(sCardID != (LFRecBuff[LF_DATA_DEVID_H]<<8|LFRecBuff[LF_DATA_DEVID_L]))
    {
        changeDevCount++;
    }

    if(sCmdandType != LFRecBuff[LF_DATA_CMDANDTYPE])
    {
        changeCmdCount++;
    }

    //if((changeCardCount > 1)||(changeDevCount > 1)||(changeCmdCount > 1))
    {
        changeCardCount = 0;
        changeDevCount  = 0;
        changeCmdCount  = 0;
        memcpy(LFData,&LFRecBuff,LFRecBuff[LF_DATA_LENORMATCH]+1);
        Start_Event(EVENT_TO_WAKE_CARD);
    }
}
/******************************************************************************
 *   向CC2530转发LF数据;
 *
 */
void Send_Data_to_Card(void)
{
    if((LFData[LF_DATA_LENORMATCH] <= 0)||(sysStatus != SYSTEM_IDLE))
    {
        return;
    }
    if(sysStatus == SYSTEM_IDLE)
    {
        stop_Timer0();
        stop_LFReceiver();
        memset(LFSendData,0,sizeof(LFSendData));
        LFData[0]= RECEIVE_MATCH_BYTE;
        memcpy(&LFSendData,&LFData[0],LFData[LF_DATA_LENORMATCH]+1);
        //Data_to_Manchester_code(&LFSendData,&LFData[LF_DATA_START],LFData[LF_DATA_LENORMATCH],1);
        sysStatus = SYSTEM_SEND;
        Timer0 = 0;
        start_Timer0();
    }
}

/******************************************************************************
 *   将数据转化成曼切斯特码;
 *
 */
void Data_to_Manchester_code(uint8 *pManchCode,const uint8 *pdata,uint16 datalen,uint8 polar)
{
    uint8 sData[LF_DATA_LEN+1];

    if((pdata == NULL)||(pManchCode == NULL)||(datalen == 0)||(datalen > LF_DATA_LEN))
    {
        return;
    }
    memset(&sData,0,LF_DATA_LEN+1);
    memcpy(&sData,pdata,datalen);

    for(uint8 idx =0; idx < datalen; idx++)
    {
        uint8 i=0;
        for(i=0; i<4; i++)
        {
            if(sData[idx]&(0x80>>i))
            {
                //pManchCode[idx*2]&=~(3<<(6-2*i));
                pManchCode[idx*2]|= (polar?2:1)<<(6-2*i);

            }
            else
            {
                //pManchCode[idx*2]&=~ (3<<(6-2*i));
                pManchCode[idx*2]|= (polar?1:2)<<(6-2*i);
            }
        }

        for(i=4; i<8; i++)
        {
            if(sData[idx]&(0x80>>i))
            {
                /*
                 *   6-2*(i-4)=14-2*i;
                 */
                //pManchCode[idx*2+1] &=~ (3<<(14-2*i));
                pManchCode[idx*2+1]|= (polar?2:1)<<(14-2*i);//(6-2*(i-4));
            }
            else
            {
                //pManchCode[idx*2+1] &=~ (3<<(14-2*i));
                pManchCode[idx*2+1]|= (polar?1:2)<<(14-2*i);
            }
        }
    }
}

/******************************************************************************
 *   向卡射频部分发送低频数据
 *
 */
void LF_Data_Sent(void)
{
    static uint16 i = 0;

    if(sysStatus != SYSTEM_SEND)
    {
        return;
    }

    DATA_OUT_IDLE;
    if(i++%2)
    {
        return;
    }


    if(sysStatus == SYSTEM_SEND)
    {
        if(((LF_DATA_LEN+1)*8) <= sendBitLen)
        {
            i = 0;
            sysStatus = SYSTEM_IDLE;
            sendBitLen  = 0;
            stop_Timer0();
            start_LFReceiver();
            DATA_OUT_IDLE;
            asm("sleep");
            asm("nop");
            asm("nop");
            return;
        }

        if(LFSendData[sendBitLen/8]&(0x80>>(sendBitLen%8)))
        {
             OUT_H;
        }
        else
        {
             OUT_L;
        }
        sendBitLen++;
   }
}

/******************************************************************************
 *   接收低频数据
 *
 */
void LF_Data_Rec(void)
{

    if(sysStatus == SYSTEM_REC)
    {
    #ifdef DEBUG_TEST
        DEBUG_PIN  = 1;
    #endif

        if(Timer0 > sRecTimer)
        {
            unsigned int vTimer =0;

            vTimer = Timer0-sRecTimer;
    #if (RECEIVE_FRE == 50000)
            /* decide by samping frequency,
             * but Ideal are very plentiful, reality is very bone
             */
            if(vTimer > 4)
    #else
            "error"
    #endif
            {
                LFRecBuff[LF_DATA_START+recBitLen/8]|= 0x01<<(7-recBitLen%8);
            }
            else
            {
                LFRecBuff[LF_DATA_START+recBitLen/8]&=~ 0x01<<(7-recBitLen%8);
            }
            recBitLen++;
            sRecTimer = Timer0;

        }

        if(recBitLen >= LF_DATA_LEN*8)
        {
        #ifdef DEBUG_TEST
            DEBUG_PIN  = 0;
        #endif
            LFRecBuff[LF_DATA_LENORMATCH] = LF_DATA_LEN;
            Start_Event(EVENT_PARSE_LF_BUFF);
            Reset_State_to_Idle();
        }
    }
}

void Reset_State_to_Idle(void)
{
    stop_Timer0();
    DATA_OUT_IDLE;
    sysStatus = SYSTEM_IDLE;
    recBitLen = 0;
    sendBitLen =0;
    Timer0 = 0;
    sRecTimer = 0;
}

