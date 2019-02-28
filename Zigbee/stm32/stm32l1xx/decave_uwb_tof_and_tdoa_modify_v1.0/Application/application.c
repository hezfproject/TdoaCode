#include "stm32l1xx.h"
#include <stdlib.h>
#include <string.h>

#include "version.h"
#include "tls_route.h"
#include "NT_protocol.h"

#include "board.h"
#include "timer_event.h"
#include "mac_msg.h"
#include "Mem.h"

#include "app_card_cfg.h"
#include  "instance.h"
#include "port.h"
#include "printf_util.h"
#include "sleep.h"
#include "bsmac_header.h"
#include "crc.h"
#include "nwk_protocol.h"
#include "app_protocol.h"
#include "config.h"
#include "Application.h"



#define MAX_TIMER_TICK  (0xFFFFFFFF/2)

/*****************************************************************************
* CONSTANTS AND DEFINES
*/


// on last sector tail 6byte
#define DEV_ADDRESS (0x0801FF00)
#define DEVTYPE_ADDR  (0x08080FF0)
#define CARD_1S_MOTION_CNT           10        //30minutes 1.2s card 1500
#define CARD_5S_MOTION_CNT            300       //30minutes   6s card 

#define D_VALUE  4
#define DEFF(x,y)   ((x)>=(y)?((x)-(y)):((y)-(x)))

#define TOF_BUZZER_TIMEOUT			200	
#define TOF_MAX_DISTANCE_INLINE     (400)
#define TOF_MIN_RSSI_INLINE         (-100)
#define CARD_RESET_NOTOF_COUNT       10
/*****************************************************************************
* TYPEDEFS
*/

typedef enum
{
    Ranging_Report_TOA          = 0,
    Ranging_Report_Distance     = 1,
    Ranging_Report_Misc_Dist    = 2,
}Ranging_Report_Mode_e;
typedef struct
{
	uint_8 u8Sysmod;
	uint_8 u8IntSrc;
	uint_8 u8FFMtSrc;
	uint_8 u8TransientSrc;
	uint_8 u8XbyteH;
	uint_8 u8YbyteH;
	uint_8 u8ZbyteH;
}teMMA845x_Data;


teMMA845x_Data mma845xData;

/*******************************************************/


//状态优先级: 报警>撤离>低电>其他(无事件)
typedef enum
{
    STATE_URGENT_IDLE = 0,
    STATE_URGENT_RETREAT,
    STATE_URGENT_CNF,    //按键确认后
}Urgent_State_e;

typedef enum
{
    CARD_STATE_NOMAL = 0,
    CARD_STATE_URGENT,      //撤离闪灯时处于改状态
    CARD_STATE_HELP,
}Card_Work_Status_e;

typedef enum
{
    CARD_MOTION_REST = 0,
    CARD_MOTION_MOVE
}Card_Motion_Status_e;


/*
typedef struct
{
    uint16 u16CardNum[MAX_SET_CARD_CNT];
    uint8 u8CardCnt;
} tsCardTypeSet;
*/
extern  slot_msg_t my_cardslotmsg; //
extern  Sub_alarm_msg_t sub_alarmmsg;
uint8 set_helpover_sleep=0;      //when press help button and beep over ,count the sleep tick ,it's diffrent from other times

uint16 u16CardMotionTimeout = CARD_1S_MOTION_CNT;
uint16 u16CardMotionCnt = 0;
extern uint8 u8TdoaCardSendCount;
extern uint16 u16TdoaQuickCardSendCount;
extern uint8 u8TdoaCardSendCount;
extern TOF_INST_CARD_DATA_S gstTofInstCardDate;
extern slot_msg_t my_staslotmsg;
TDOA_UWB_MSG_PACK_SEND_S gTdoaSendPack;


app_UWBVersionReport_t tsVersionReport[APP_TOF_VERSION_MAX_NUM];
uint8					 tsVersionReportLen;

#ifdef DEC_UWB_SUB
extern ts_Car_cardlist Car_revcardlist;
#endif

/*****************************************************************************
*   DECLARATION
*/

static uint_32 eventHold;           //事件保持，以免休眠

//static Card_Work_Status_e cardWorkState = CARD_STATE_NOMAL;
//static Card_Motion_Status_e cardMotionState = CARD_MOTION_MOVE;

// motion detection counter
Bool isMoving = True;

//static uint_32 cardMotionCheckTime;
//static uint_32 cardAdcCheckTime;
//static uint_32 reportVersionTime = 0;


#ifndef DEC_UWB_ANCHOR
uwb_tof_distance_ts distlist[TOF_SLOT_LOC_PERIOD];
#endif

#ifndef DEC_UWB_TAG
uwb_tof_distance_ts distlist[TOF_SLOT_LOC_PERIOD];
#endif

uint8 u8HelpCnt = 0;

uint32 u32CommonBuf[128]={0};
//uint16 u16StationPanId;
uint16 u16ArmId = 0xFFFF;
uint8 u8ReportStatusNum = 0;
uint16 u16Hdl_tx_frame_cnt = 0;
uint16 u16Hdl_rx_frame_cnt;
static uint8 u8cardtype=1;
static uint8 helpask=0;
uint8 cur_slot= 0;
uint8 tof_count=0;

uint8 u8UartSendBuf[300]={0};

uint8 next_retreat =1 ;  //if is the next retreat beep 
uint16 count_tof=0;      //the number of the tof 
uint32 pretick=0;
uint8 quiet_count= 0;    //not moving and wake up time(s), if <10 short sleep ,if >10 long sleep when the card stay qiuet
uint16 u16ShortAddr;
uint8 newstarttype=0;
uint8 avg_tof_tick=0;
uint8 ever_rev_dis=0;   //wether the card have rev distance this time
uint8 bool_check_poll=0;
uint8 tStationStatus=0;
uint8 U8iCount = 0,u8ExciteTure=FALSE ;
uint16 U16ExciterID = 0 ,u16LastExciterID=0;
static uint16 U16TailMs;
static uint8 check_dis_rss=0;
uint8 new_inblink=0;
uint8 is_sendlink=0;

tsCardTypeSet CardType_5s;
tsCardTypeSet CardType_1s;
uint32 last_time=0;
extern uint8 rev_retreat_ack;
static uint16 pre_seqnum=0;
uint16 idle_list_count=0;   //the card Continuous idle count

#define SLOW 0
#define QUICK 1
extern int instance_TxSpeed;

void vWriteData2Stm32(uint8* pbuf,uint16 len);
uint8 bsmac_build_packet( unsigned char * pbuf,
                              const unsigned char * pdata, unsigned short len,
                              const unsigned char frame_type);

//void vReportCardDistance(void);
void uart_rx_callback(unsigned char *pbuf, unsigned char len);

void vCheckBattery(void);
void Appsleepinit(void);
/*****************************************************************************
* FUNCTION
*/
void Write_Devtype(uint16 type)
{
	DATA_EEPROM_Unlock();		 //解锁FLASH 
	DATA_EEPROM_EraseWord(DEVTYPE_ADDR);
	//DATA_EEPROM_ProgramHalfWord(DEVTYPE_ADDR,type);//*RamAdr
	*(uint16*)(DEVTYPE_ADDR)= type;
	DATA_EEPROM_Lock();	
}
uint16 instance_get_cardtype(void)
{
	uint16 cardtype=1,temp=0;
	temp= *(uint16*)(DEVTYPE_ADDR);
	if(temp!=1 && temp!=5)
	{
		temp = *(uint16*)(DEV_ADDRESS+2);
		if(temp==1 || temp ==5)
		{
			cardtype = temp;
			//Write_Devtype(temp);
		}
	}
	else
		cardtype = temp;
	return cardtype;
}


void AppSleepOff(uint_32 u32Event)
{
    eventHold |= u32Event;
}

void AppSleepOn(uint_32 u32Event)
{
    eventHold &= ~u32Event;
}

void AppGreenLedFlash()
{
    LED_Green_On();
    AppSleepOff(EVENT_GREEN_LED_FLASH);
    event_timer_add(EVENT_GREEN_LED_FLASH, LED_FLASH_TIME);
}




void AppRedLedFlash()
{
    LED_Red_On();
    AppSleepOff(EVENT_RED_LED_FLASH);
    event_timer_add(EVENT_RED_LED_FLASH, LED_FLASH_TIME);
}

/*
static void AppRestartRangingEvent(void)
{
    //静止到运动，立即开始上报
 //   cardRangingInfo.rangeind = 0;   //若正在测距，重新开始测距

    event_timer_del(EVENT_RAGING_REPORT);
    event_timer_del(EVENT_GREEN_LED_FLASH);
    event_timer_set(EVENT_RAGING_REPORT);
}

static void AppUpdateCardMotionState(void)
{
//	cardMotionCheckTime = GetSysClock();

	if (isMoving == True)
	{
		if (cardMotionState == CARD_MOTION_REST)
		{
		//    AppCardMotionstateSet(CARD_MOTION_MOVE);
			//静止到运动，立即开始上报
		    AppRestartRangingEvent();
			motion_detect_int_close();
		}
		u16CardMotionCnt =0 ;
	//	PrintfUtil_vPrintf(" ismoving = ture  ;MotionCnt = %d\n" ,u16CardMotionCnt);
	}
	else
	{
		isMoving = False;
		//运动到静止，启用中断
		motion_detect_int_open();
	}
}
*/


void process_deca_irq(void)
{
    do{

    	instance_process_irq(0);
    }while(port_CheckIRQ() == 1); //while IRS line active (ARM can only do edge sensitive interrupts)

}

/*
* 电量检测
*/

uint16 card_clc_count =0;
uint8 motor_count=0;

void vCheckBattery()
{
	uint16 battery=0 ,battery1=0;

	if((card_clc_count-2)%(500/u8cardtype) ==0)
	{
		battery1 = ADC_Get_ADCValue();
		battery = (battery1 +60)/2.9+1;
	//	PrintfUtil_vPrintf(" battery = %d ,battery1 = %d \n" ,battery,battery1);
		if(battery>300)
		{
			if(battery < 380)
				my_cardslotmsg.status |=UWB_CARD_STATUS_NOPWD;
			else
				my_cardslotmsg.status &=~UWB_CARD_STATUS_NOPWD;
			instance_set_vbat(battery);
		}
		else //测量的电压太低则重新测量
			card_clc_count--;
	}
	
}


void Stop_ledvbeep(uint8 type)
{
	LED_Red_Off();
	LED_Green_Off();
	BEEP_Stop();
	if(type==0)
	{
		MOTOR_Stop();
		motor_count=0;
	}
	else
	{
		if(motor_count %2==0&&motor_count %4!=0)
			MOTOR_Stop();
		motor_count++;
	}
}

static void Check_card_lostnum(void)
{

	if(ever_rev_dis ==0 &&my_cardslotmsg.b1Used == USED_TOF)  //if rev poll but not tof seccuss, 4 times continuous than set idle
	{
		my_cardslotmsg.u8LostNum++;
		if(my_cardslotmsg.u8LostNum >=LOST_TOF_MAX_NUM){
			instance_set_idle();
			my_cardslotmsg.sleeptick = CARD_1S_SEC_TIME/4;  //when set idle form tof ,the this slot sleep time is short
			DBG(PrintfUtil_vPrintf("set idle\n");)
		}
	}
	else
	{
	my_cardslotmsg.u8LostNum =0;

	}
	
	PrintfUtil_vPrintf(" ever_rev_dis = %d u8LostNum = %d b1Used = %d Check_card_lostnum\n", 
		ever_rev_dis, my_cardslotmsg.u8LostNum, my_cardslotmsg.b1Used);	
}

int8 card_reset_dw1000()
{
	uint32 devID ;
	instanceConfig_t instConfig;
    int  result;
	
	NVIC_DisableIRQ(EXTI0_IRQn);

	reset_DW1000();
    SPI_ConfigFastRate(SPI_BaudRatePrescaler_8);  //max SPI before PLLs configured is ~4M
	
	//this is called here to wake up the device (i.e. if it was in sleep mode before the restart)
    devID = instancereaddeviceid() ;
    if(DWT_DEVICE_ID != devID) //if the read of devide ID fails, the DW1000 could be asleep
    {
    	port_SPIx_clear_chip_select();	//CS low
    	mSleep(1);	//200 us to wake up then waits 5ms for DW1000 XTAL to stabilise
    	port_SPIx_set_chip_select();  //CS high
    	mSleep(7);

		//add Sleep(50) to stabilise the XTAL
        mSleep(50);

    	devID = instancereaddeviceid() ;
        // SPI not working or Unsupported Device ID
    	if(DWT_DEVICE_ID != devID)
    		return(-1) ;
    	//clear the sleep bit - so that after the hard reset below the DW does not go into sleep
    	dwt_softreset();
    }
	
	reset_DW1000();
	reinit_dw1000();
	//instance_init();
	SPI_ConfigFastRate(SPI_BaudRatePrescaler_4); //increase SPI to max
    devID = instancereaddeviceid() ;

    if (DWT_DEVICE_ID != devID)   // Means it is NOT MP device
    {
        // SPI not working or Unsupported Device ID
		return(-1) ;
    }

    instConfig.channelNumber = chConfig[0].channel ;
    instConfig.preambleCode = chConfig[0].preambleCode ;
    instConfig.pulseRepFreq = chConfig[0].prf ;
    instConfig.pacSize = chConfig[0].pacSize ;
    instConfig.nsSFD = chConfig[0].nsSFD ;

    instConfig.dataRate = chConfig[0].datarate ;
    instConfig.preambleLen = chConfig[0].preambleLength ;

    instance_config(&instConfig) ;                  // Set operating channel etc
    
#if (DR_DISCOVERY == 0)
    addressconfigure() ;                            // set up initial payload configuration
#endif
   // instancesetreplydelay(FIXED_REPLY_DELAY);

	Appsleepinit();
	NVIC_EnableIRQ(EXTI0_IRQn);
	return devID;
}

#ifdef DEC_UWB_ANCHOR

static void Beep_and_redflsh(void)
{
	AppRedLedFlash();
	BEEP_Start();
	event_timer_add(EVENT_LED_HUNGER,TOF_BUZZER_TIMEOUT);
}
static void AppHelpBegin(void)
{
//	cardWorkState = UWB_CARD_STATUS_HELP;
	u8HelpCnt = 0;
	AppSleepOff(EVENT_HELP_MSG);
}



static void AppHelpProc(void)
{
	uint8 status=0;
	status = my_cardslotmsg.status;
	status = instance_get_helpstatus();      //reset when rev help resp
	
	event_timer_unset(EVENT_SLEEP_EVENT);
	helpask =1;
	if(status ==0)
	{
		if(u8HelpCnt++ <= 10)
		{
			
			event_timer_add(EVENT_HELP_MSG,500);
			Beep_and_redflsh();
			DBG(PrintfUtil_vPrintf("Help cnt:%d\n",u8HelpCnt);)
		}
		else
		{
			AppGreenLedFlash();
			BEEP_Stop();
			MOTOR_Stop();
			DBG(PrintfUtil_vPrintf("Help stop:%d\n",u8HelpCnt);)
			event_timer_del(EVENT_HELP_MSG);	
			event_timer_add(EVENT_SLEEP_EVENT,1000); 
		//	instance_set_idle();
			instance_reset_helpstatus();
			u8HelpCnt = 0;
			helpask =0;
			set_helpover_sleep=1;
		}
	}
	else   //1or 2      //if rev help ack ,but not rev counter resp ,beep 5 times at lest
	{
		if(u8HelpCnt++ <6)   
		{
			event_timer_add(EVENT_HELP_MSG,500);
			Beep_and_redflsh();

			DBG(PrintfUtil_vPrintf("*Help cnt:%d\n",u8HelpCnt);)
		}
		else
		{
			AppGreenLedFlash();
			BEEP_Stop();
			MOTOR_Stop();
			DBG(PrintfUtil_vPrintf("Help rev stop:%d\n",u8HelpCnt);)
		//	instance_set_idle();
			instance_reset_helpstatus();
			
			event_timer_del(EVENT_HELP_MSG);			
			event_timer_add(EVENT_SLEEP_EVENT,1000);
			u8HelpCnt = 0;
			helpask =0;
			set_helpover_sleep=1;
		}
	}
	
}



static void AppAlarmProc(void)
{
//	event_timer_del(EVENT_SLEEP_EVENT);
	if(my_cardslotmsg.status &UWB_CARD_STATUS_RETREAT)
	{
		Beep_and_redflsh();
		if(motor_count %4==0)
			MOTOR_Start();
		event_timer_add(EVENT_URGENT_RETREAT, 500); 
		
		helpask =1;
	//	PrintfUtil_vPrintf("<AppAlarmProc>  %d \n",my_cardslotmsg.status);
	}
	else
	{
		helpask =0;
		Stop_ledvbeep(0);
	}
}



/*******************************************************************************
* 处理求救按键的函数
*/
//uint8 alarm_beep_stop=1;

static uint8 AppButtonProc(void)
{
	uint8 key_code=0;

	if ((key_code = BUTTON_KeyCode_Read()) == BTN_IDLE)
	{
		helpask = 0;
		event_timer_del(EVENT_SLEEP_EVENT);
		event_timer_add(EVENT_SLEEP_EVENT,1000);
	    return 0;
	}
//	PrintfUtil_vPrintf("key_code= <%d> <%d>\n",key_code,next_retreat);
	if ((key_code & BTN_HELP) )
	{
		helpask =1;
		event_timer_del(EVENT_SLEEP_EVENT);
		event_timer_add(EVENT_SLEEP_EVENT,1000);
		if(u8HelpCnt<10)
		{
			u8HelpCnt ++;
			set_helpover_sleep=0;
			event_timer_add(EVENT_BUTTON_MSG,200);
			DBG(PrintfUtil_vPrintf("H<%d> \n",u8HelpCnt);)
			return 2;
		}
		else
		{
			u8HelpCnt =0;
			helpask =0;
			my_cardslotmsg.status |= UWB_CARD_STATUS_HELP ;
		//	event_timer_del(EVENT_SLEEP_EVENT);
			event_timer_set(EVENT_HELP_MSG);
			instance_set_helpexcit(1);
		//	MOTOR_Start();
			return 1;
		}
		
	}
	else if ((key_code & BTN_CFRM)&& next_retreat ==1)
	{
		
		if(my_cardslotmsg.status & UWB_CARD_STATUS_RETREAT)
		{
			helpask =1;
			event_timer_del(EVENT_SLEEP_EVENT);
			event_timer_add(EVENT_SLEEP_EVENT,1000);
			if(u8HelpCnt<5)
			{
				u8HelpCnt ++;
			//	alarm_beep_stop =0;   
				set_helpover_sleep=0;
				event_timer_add(EVENT_BUTTON_MSG,200);
				DBG(PrintfUtil_vPrintf("<%d> \n",u8HelpCnt);)
				return 2;
			}
			else
			{
				BEEP_Stop();
				DBG(PrintfUtil_vPrintf("|* %d|",u8HelpCnt);)
				
				u8HelpCnt =0; 
				next_retreat =0 ;   //when this time press stop ,then never beep again before the new retreat come
			//	instance_set_idle();
				if(my_cardslotmsg.status & UWB_CARD_STATUS_RETREAT)
				{
					my_cardslotmsg.status |= UWB_CARD_STATUS_RETREAT_ACK;
					txretreat_ack_send();  //send buttun ack
				
					event_timer_add(EVENT_URGENT_RESET,600000);
					event_timer_del(EVENT_URGENT_RETREAT);
				}
				event_timer_del(EVENT_SLEEP_EVENT);
				event_timer_add(EVENT_SLEEP_EVENT,1000);
				helpask =0;
				set_helpover_sleep =1;
				return 0;
			}
		}
		
	}
}

void CardMotionDetect()
{
    static teMMA845x_Data vmma845xData;

    static uint8 mma845x_invalid = FALSE;
    uint8 value;
    uint8 PlStauts;

    if (mma845x_invalid)
    {
    	PrintfUtil_vPrintf("invalid\n");
        return;
    }

    value = mma845xData.u8IntSrc = MMA845x_ReadReg(0x0C);     //INT_SOURCE

 //   if(0xFF == value)
    {
        if(MMA845x_ChipCheck())
        {        
			PrintfUtil_vPrintf(" u16CardMotionCnt = %d MMA845x_ChipCheck\n\n", u16CardMotionCnt);
            mma845x_invalid = TRUE;
            u16CardMotionCnt = 0;
            my_cardslotmsg.status &= ~UWB_CARD_STATUS_IS_WITH_ACCEL;
            return;
        }
        else
        {
            mma845xData.u8IntSrc = MMA845x_ReadReg(0x0C);     //INT_SOURCE
            mma845x_invalid = FALSE;
            my_cardslotmsg.status |= UWB_CARD_STATUS_IS_WITH_ACCEL;
        }
    }

	mma845xData.u8FFMtSrc = MMA845x_ReadReg(0x16);    //FF_MT_SRC
	//mma845xData.u8Sysmod = MMA845x_ReadReg(0x0B);     //SYSMOD
	mma845xData.u8TransientSrc = MMA845x_ReadReg(0x1E);
	mma845xData.u8XbyteH = MMA845x_ReadReg(0x01);
	mma845xData.u8YbyteH = MMA845x_ReadReg(0x03);
	//mma845xData.u8ZbyteH = MMA845x_ReadReg(0x05);

    //DBG(PrintfUtil_vPrintf("FFM %d,Trans %d\n",mma845xData.u8FFMtSrc,mma845xData.u8TransientSrc);)

	PlStauts = MMA845x_ReadReg(0x10);   //PL Status Register

	if((DEFF(mma845xData.u8XbyteH,vmma845xData.u8XbyteH) > D_VALUE)
		|| (DEFF(mma845xData.u8YbyteH,vmma845xData.u8YbyteH) > D_VALUE))
		//|| (DEFF(mma845xData.u8ZbyteH,vmma845xData.u8ZbyteH) > D_VALUE))
	{
		my_cardslotmsg.status |= UWB_CARD_STATUS_ACTIVE;
	}
	//else if((mma845xData.u8TransientSrc & 0x40) || (mma845xData.u8FFMtSrc & 0x80))
    else if((mma845xData.u8TransientSrc & 0x40) || (mma845xData.u8FFMtSrc & 0x80))
	{
		my_cardslotmsg.status|= UWB_CARD_STATUS_ACTIVE;
	}
	else
	{
		my_cardslotmsg.status &= ~UWB_CARD_STATUS_ACTIVE;
	}

	vmma845xData = mma845xData;
	if((PlStauts & 0x04) && (!(PlStauts & 0x40)))
	{
		my_cardslotmsg.status |= UWB_CARD_STATUS_ORIENTATION;
	}
	else
	{
		my_cardslotmsg.status &= ~UWB_CARD_STATUS_ORIENTATION;
	}

    if(UWB_CARD_STATUS_ACTIVE & my_cardslotmsg.status)
    {
	//	PrintfUtil_vPrintf(" u16CardMotionCnt = %d UWB_CARD_STATUS_ACTIVE\n\n", u16CardMotionCnt);
        u16CardMotionCnt = 0;
		quiet_count =0 ;
		isMoving = True;
    }
    else
    {
    
        if(u16CardMotionCnt <u16CardMotionTimeout)
            u16CardMotionCnt++;
		else
		{
			isMoving = False;
		//	cardMotionState = CARD_MOTION_REST;
			motion_detect_int_open();
		}
	//	PrintfUtil_vPrintf("MotionCnt = %d\n" ,u16CardMotionCnt);

    }

//	PrintfUtil_vPrintf("card status %x\n",my_cardslotmsg.status);
}

void vCheck_Devtype_change(void)
{

	if(instance_getchange_devtype())   //change the card's cycle
	{
		if(my_cardslotmsg.u8DeviceType == CARD_1S)
		{
			instance_init_cardslot(CARD_5S); //1:1s card  5:5s card
			u8cardtype = CARD_5S;
		}
		else if(my_cardslotmsg.u8DeviceType == CARD_5S)
		{
			instance_init_cardslot(CARD_1S); //1:1s card  5:5s card
			u8cardtype = CARD_1S;
		}
		instance_set_idle();
		PrintfUtil_vPrintf("--change devtype ok!-- !\n");
		Write_Devtype((uint16)u8cardtype);
	}
}

void Check_used_status(void)
{
	uint8 revpolltype=0;
	revpolltype =instance_get_revpolltype();
	PrintfUtil_vPrintf(" revpolltype = %d b1Used = %d card SeqNum = %d Check_used_status\n", 
		revpolltype, my_cardslotmsg.b1Used, my_cardslotmsg.u16SeqNum);

	if(my_cardslotmsg.b1Used != IDLE)
	{
		if(revpolltype ==0 && helpask==0)  //nothing rev then idle at once 
		{
			instance_set_idle();
			my_cardslotmsg.sleeptick = CARD_1S_SEC_TIME/4;  //when set idle form tof ,the this slot sleep time is short
		}
		else if(revpolltype !=0)   //rev some blink msg but not rev tof msg not idle at once 
			instance_set_AnchorPanid((uint8)ANCHOR_TOF_PRO);
	}
	else
	{
		new_inblink = instance_get_inblinkmsg();
		if(new_inblink)     //new in blink
		{
			instance_set_AnchorPanid((uint8)ANCHOR_TOF_PRO);
			my_cardslotmsg.b1Used = USED_TOF;
			ever_rev_dis =1; //is not a lost
		}
		//tdoa_send();
	}

}

uint8 calcrc_1byte(uint8 abyte)
{
    uint8 i,crc_1byte;
    crc_1byte=0;                //设定crc_1byte初值为0
    for(i = 0; i < 8; i++)
     {
       if(((crc_1byte^abyte)&0x01))
          {
            crc_1byte^=0x18;
            crc_1byte>>=1;
            crc_1byte|=0x80;
           }
        else
           crc_1byte>>=1;
        abyte>>=1;
      }
      return crc_1byte;
 }

// EVENT_EXCIT_EVENT
void ProcessExcite(void)
{
	static uint16 U16HeadMs;
	//static UINT16 U16ExciterID = 0;
	uint16 crcCode,crc;
	uint16 CycleLenth;
	uint8 u8PacketNum=0;
	U16HeadMs = (uint16)GetSysClock();
	if(U8iCount == 0)
	{
		U8iCount++;
		U16TailMs = U16HeadMs;
		U16ExciterID = 0;
		CycleLenth = 0;
	}
	else
	{
		CycleLenth = U16HeadMs - U16TailMs;
		U16TailMs = U16HeadMs;
	}

	if(CycleLenth > 1)  //两次中断之间间隔超过1ms则认为无效了
	{
		U8iCount = 1;
		U16ExciterID = 0;
	}

	uint8 U32SDAdio = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);//PA_10;
	if(U32SDAdio & 0x1)
	{
		U16ExciterID |= 1<<(16-U8iCount);
	}
	U8iCount++;
	AppRedLedFlash();
	BEEP_Start();
	event_timer_add(EVENT_LED_HUNGER,20);
	if(U8iCount == 17)
	{
		uint8 u8HighExciterID=0,u8LowExciterID=0;
		U8iCount = 0;
		u8HighExciterID = (uint8)((U16ExciterID>>8)&0xff);
		u8LowExciterID = (uint8)(U16ExciterID&0xff);

		crcCode =calcrc_1byte(u8HighExciterID);
		crcCode = crcCode & 0x00FF;
		if((u8LowExciterID == crcCode) && (U16ExciterID !=0) && (u8HighExciterID == u16LastExciterID))
		{
			U16ExciterID = (uint16)u8HighExciterID;
			u8ExciteTure = TRUE;
			my_cardslotmsg.status |= UWB_CARD_STATUS_IMPEL;
			event_timer_set(EVENT_EXCIT_EVENT);   //go send excit status to sub
			instance_set_helpexcit(2);
			PrintfUtil_vPrintf("--excit secuss  ! %d -- !\n",U16ExciterID);
		}
		event_timer_add(EVENT_SLEEP_EVENT, 1000);
		u16LastExciterID =(uint16) u8HighExciterID;
	}
}


void card_newslot_init(void)
{
	uint32 time=0;
	clear_inblinkmsg();      //every new slot ,the blink status start from 0	
	newstarttype =1;
	time = pretick - last_time;
	
	if(time>u8cardtype*CARD_1S_SEC_TIME-200)
	{
		my_cardslotmsg.u16SeqNum++;
		last_time = pretick;
		card_clc_count ++;
	}
  	my_cardslotmsg.sleeptick =0;
	my_cardslotmsg.m_distance = 0;
	CardMotionDetect();          //检查卡的运动情况
	new_inblink=0;
	//dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG , 1); 
}


#endif
/*******************************************************************************/
void vProcessCardVersion(uint_16 u16DevId, uint_16 u16OadVersion, uint_16 u16Battery)
{
    uint_8 i;
    uint8 bfind = FALSE;

    /* if already exist */
    for(i=0; i<APP_TOF_VERSION_MAX_NUM; i++)
    {
        if(tsVersionReport[i].devid!=0 && tsVersionReport[i].devid == u16DevId)
        {
            tsVersionReport[i].devid = u16DevId;
            tsVersionReport[i].oad_ver = u16OadVersion;
            tsVersionReport[i].battery = u16Battery;
            bfind = TRUE;
            break;
        }
    }

    /* else fill a new position */
    if(!bfind)
    {
        for(i=0; i<APP_TOF_VERSION_MAX_NUM; i++)
        {
            if(tsVersionReport[i].devid ==0)
            {
                tsVersionReport[i].devid = u16DevId;
                tsVersionReport[i].oad_ver = u16OadVersion;
                tsVersionReport[i].battery = u16Battery;
                tsVersionReportLen++;
                bfind = TRUE;
                break;
            }
        }
    }

    if(tsVersionReportLen >= APP_TOF_VERSION_MAX_NUM)
    {
        //立即发送
        event_timer_unset(EVENT_REPORT_CARDVERSION);
		event_timer_set(EVENT_REPORT_CARDVERSION);
    }
}

//#ifndef DEC_UWB_ANCHOR
#if 1

void vReportStatus(void)
{
    //struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
    is_sendlink =1;
    app_header_t *pHeader = (app_header_t *)u32CommonBuf;
    app_LSrfReport_t *pStationReport = (app_LSrfReport_t *)(pHeader + 1);

    /* app header */
    pHeader->len = sizeof(app_LSrfReport_t)+strlen(VERSION);

    pHeader->msgtype = APP_UWB_MSG_STATION_VER_LINK;
    pHeader->protocoltype = APP_PROTOCOL_TYPE_UWB_CARD;

    /* rf report */
    pStationReport->hdr.dstaddr = u16ArmId;

    pStationReport->hdr.srcaddr = u16ShortAddr;

    pStationReport->len = strlen(VERSION);

    pStationReport->reporttype = APP_LS_REPORT_STATUS_LOC;
    pStationReport->devtype = BSMAC_DEVICE_TYPE_LOC;
    pStationReport->seqnum = u8ReportStatusNum++;

    memcpy((uint8*)(pStationReport+1),VERSION,strlen(VERSION));
    bsmac_build_packet(u8UartSendBuf,(uint8 *)u32CommonBuf,pHeader->len+sizeof(app_header_t),BSMAC_FRAME_TYPE_DATA);

}


void vReportCardDistance(void)
{
	// 本节第一个slot
	//uint16 u16IntBeginSlot = (u16SlotIndex/TOF_SLOT_LOC_INT)*TOF_SLOT_LOC_INT;

	//uint16 u16CardBeginSlot = ((u16IntBeginSlot + TOF_SLOT_NUM - TOF_SLOT_LOC_INT*2) % TOF_SLOT_NUM);
	//uint16 u16CardNum = 0;
	uint16 i,j,m=0,n=0,count;
	app_uwb_tof_distance_ts app_uwb_tof_distance_data;
	app_uwb_tof_distance_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_UWB_CARD;
	app_uwb_tof_distance_data.app_tof_head.msgtype = APP_UWB_MSG_DISTANCE;
	count = tof_count;
	while(count >10)
	{
		n = m*10;
		for(i=n,j=0;i<n+10&&i<tof_count;i++,j++)
		{
			//tof_new_distance_ts  *distance_data = &app_tof_distance_data.tof_distance[u16CardNum];
			app_uwb_tof_distance_data.tof_distance[j].u16ShortAddr  = distlist[i].u16ShortAddr;
			app_uwb_tof_distance_data.tof_distance[j].u16SeqNum     = distlist[i].u16SeqNum;
			app_uwb_tof_distance_data.tof_distance[j].u32StationDistance = distlist[i].u32StationDistance;
			app_uwb_tof_distance_data.tof_distance[j].u32LocDistance = distlist[i].u32LocDistance;
			app_uwb_tof_distance_data.tof_distance[j].u8DevType = distlist[i].u8DevType;
			app_uwb_tof_distance_data.tof_distance[j].u8Status = distlist[i].u8Status;
			app_uwb_tof_distance_data.tof_distance[j].i8Rssi = distlist[i].i8Rssi;
			app_uwb_tof_distance_data.tof_distance[j].u8Reserved= 0;
		}
		m++;
		app_uwb_tof_distance_data.app_tof_head.len =  10*sizeof(uwb_tof_distance_ts);
		bsmac_build_packet(u8UartSendBuf,(uint8 *)(&app_uwb_tof_distance_data), sizeof(app_header_t)+app_uwb_tof_distance_data.app_tof_head.len,BSMAC_FRAME_TYPE_DATA);
		count -= 10;
	}
	if(count == 0)
		return;
	n = m*10;
	for(i=n,j=0;i<tof_count;i++,j++)
	{
		//tof_new_distance_ts  *distance_data = &app_tof_distance_data.tof_distance[u16CardNum];
		app_uwb_tof_distance_data.tof_distance[j].u16ShortAddr  = distlist[i].u16ShortAddr;
		app_uwb_tof_distance_data.tof_distance[j].u16SeqNum     = distlist[i].u16SeqNum;
		app_uwb_tof_distance_data.tof_distance[j].u32StationDistance = distlist[i].u32StationDistance;
		app_uwb_tof_distance_data.tof_distance[j].u32LocDistance = distlist[i].u32LocDistance;
		app_uwb_tof_distance_data.tof_distance[j].u8DevType = distlist[i].u8DevType;
		app_uwb_tof_distance_data.tof_distance[j].u8Status = distlist[i].u8Status;
		app_uwb_tof_distance_data.tof_distance[j].i8Rssi = distlist[i].i8Rssi;
		app_uwb_tof_distance_data.tof_distance[j].u8Reserved= 0;
	}
	app_uwb_tof_distance_data.app_tof_head.len =  count *sizeof(uwb_tof_distance_ts);
	bsmac_build_packet(u8UartSendBuf,(uint8 *)(&app_uwb_tof_distance_data), sizeof(app_header_t)+app_uwb_tof_distance_data.app_tof_head.len,BSMAC_FRAME_TYPE_DATA);
}

void vReportCardAlarm(uint16 addr,uint16 exciterid,uint8 status)
{
	app_uwb_alarm_ts app_uwb_alarm_data;
	app_uwb_alarm_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_UWB_CARD;
	app_uwb_alarm_data.app_tof_head.msgtype = APP_UWB_MSG_ALARM ;
	app_uwb_alarm_data.u16ShortAddr = addr;
	app_uwb_alarm_data.u8Status = status;
	app_uwb_alarm_data.u8ExciterID = exciterid;

	app_uwb_alarm_data.app_tof_head.len = 4;
	bsmac_build_packet(u8UartSendBuf,(uint8 *)(&app_uwb_alarm_data), sizeof(app_header_t)+app_uwb_alarm_data.app_tof_head.len,BSMAC_FRAME_TYPE_DATA);

}



uint8 bsmac_build_packet( unsigned char * pbuf,
                              const unsigned char * pdata, unsigned short len,
                              const unsigned char frame_type)
{
    unsigned short tx_len;
    unsigned short crc;
    bsmac_header_t *ph;
	
    struct nwkhdr *pnwkhdr;

    // add mac header
    if (pbuf == NULL || len > 512 )
    {
        EDBG(PrintfUtil_vPrintf("Build Failed pbuf %X len%d\n", pbuf, len);)
        return 0;
    }


    ph = (bsmac_header_t *) pbuf;

    pnwkhdr = (struct nwkhdr *)(ph+1); 

    pnwkhdr->type = NWK_DATA;
    pnwkhdr->ttl = 1;
    pnwkhdr->src = u16ShortAddr;
	if(instancegetrole() == SUB_STA && !is_sendlink)
    	pnwkhdr->src = u16ShortAddr-10000;
	is_sendlink =0;
    pnwkhdr->dst = u16ArmId;
	pnwkhdr->len = len;
	

    ph->preamble_H = BSMAC_PREAMBLE_H;
    ph->preamble_L = BSMAC_PREAMBLE_L;

    BSMAC_SET_DEVICETYPE(ph->frame_control, BSMAC_DEVICE_TYPE_LOC);    // I am location module
    BSMAC_SET_RDY(ph->frame_control, 1);           							// always ready
    BSMAC_SET_FRAMETYPE(ph->frame_control, frame_type);
    BSMAC_SET_PRIORITY(ph->frame_control, 1);

	

    if (frame_type == BSMAC_FRAME_TYPE_ACK) // for ack, use recieved frame_cnt
    {
        ph->frame_count_H = (u16Hdl_rx_frame_cnt & 0xff00) >> 8;
        ph->frame_count_L = u16Hdl_rx_frame_cnt  & 0xff;
    }
    else
    {
        ph->frame_count_H = ( u16Hdl_tx_frame_cnt & 0xff00) >> 8; // framecnt_h
        ph->frame_count_L =  u16Hdl_tx_frame_cnt& 0xff; // framecnt_l
        u16Hdl_tx_frame_cnt++;
    }

    ph->src_addr_H = (u16ShortAddr >> 8) & 0xff;
    ph->src_addr_L = (u16ShortAddr) & 0xff;            // source mac address
    ph->dst_addr_H = 0;                                                     // dst address is useless
    ph->dst_addr_L = 0;
    //ph->reserverd = ;

    /* ack do not need payload, Live may have payload */
    if (len != 0 && pdata && frame_type != BSMAC_FRAME_TYPE_ACK)
    {
        memcpy((void*) (pbuf + BSMAC_HEADER_LEN+sizeof(struct nwkhdr)), pdata, len);
    }

    //LIVE packet needs to be a long frame
    if (frame_type == BSMAC_FRAME_TYPE_LIVE)
    {
        len = BSMAC_MAX_TX_PAYLOAD_LEN;
    }
    else if(frame_type == BSMAC_FRAME_TYPE_ACK)
    {
        len = 0;
    }

    tx_len = len + BSMAC_FOOTER_LEN+sizeof(struct nwkhdr); // length = payload+footer
    ph->data_len_H = (tx_len >> 8) & 0xff; //
    ph->data_len_L = tx_len & 0xff; //

    crc = CRC16((unsigned char *)(pbuf+2), len+BSMAC_HEADER_LEN+sizeof(struct nwkhdr)-2, 0xffff);   // caculate header and payload
    // padding footer
    pbuf[len+BSMAC_HEADER_LEN+sizeof(struct nwkhdr)] = (crc >> 8) & 0xff;
    pbuf[len+BSMAC_HEADER_LEN+sizeof(struct nwkhdr)+1] = crc & 0xff;
	
    vWriteData2Stm32(u8UartSendBuf,sizeof(bsmac_header_t) + tx_len);
    return sizeof(bsmac_header_t) + tx_len;
}

void vWriteData2Stm32(uint8* pbuf,uint16 len)
{
	uint8 i;
	if(pbuf == NULL || len == 0)
	{
		return;
	}
	for(i=0;i<len;i++)
	{
		USART_SendData(USART1,(unsigned char)( *(pbuf+i))); /* Loop until the end of transmission */

		//PrintfUtil_vPrintf(" write \n");
		while (USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET);
	}
}
void check_cardtype_buff(uint8 type)
{
	uint8 i=0,j=0;
	if(type == APP_LS_TLV_TYPE_UWB_CARD_1)
	{
		if(CardType_5s.u8CardCnt >0)
		{
			for(i=0;i<CardType_1s.u8CardCnt;i++)
			{
				for(j=0;i<CardType_5s.u8CardCnt;j++)
				{
					if(CardType_1s.u16CardNum[i] == CardType_5s.u16CardNum[j])
					{
						CardType_5s.u16CardNum[j]=0;
						CardType_5s.u8CardCnt--;
					}
				}
			}
			for(i=0,j=1;i<MAX_SET_CARD_CNT-1,j<MAX_SET_CARD_CNT;i++)
			{
				while(CardType_5s.u16CardNum[j]==0)
					j++;
				if(CardType_5s.u16CardNum[i]==0)
				{
					CardType_5s.u16CardNum[i] = CardType_5s.u16CardNum[j];
					CardType_5s.u16CardNum[j] =0;
				}
				while(CardType_5s.u16CardNum[j]==0)
					j++;
			}
		}
	}
	else if(type == APP_LS_TLV_TYPE_UWB_CARD_5)
	{
		if(CardType_1s.u8CardCnt >0)
		{
			for(i=0;i<CardType_5s.u8CardCnt;i++)
			{
				for(j=0;i<CardType_1s.u8CardCnt;j++)
				{
					if(CardType_1s.u16CardNum[i] == CardType_5s.u16CardNum[j])
					{
						CardType_1s.u16CardNum[j]=0;
						CardType_1s.u8CardCnt--;
					}
				}
			}
			for(i=0,j=1;i<MAX_SET_CARD_CNT-1,j<MAX_SET_CARD_CNT;i++)
			{
				while(CardType_1s.u16CardNum[j]==0)
					j++;
				if(CardType_1s.u16CardNum[i]==0)
				{
					CardType_1s.u16CardNum[i] = CardType_1s.u16CardNum[j];
					CardType_1s.u16CardNum[j] =0;
				}
				while(CardType_1s.u16CardNum[j]==0)
					j++;
			}
			
		}
	}
}

void card_retreat(uint8 *revdata,uint8 revlen)
{
	uint8 count,i;
	uint16 addr;
	if(revlen ==0)
		return;
	count = revlen/2;
	instance_set_alarmlist(addr,0xFF,0); //clear all pre alarm buff
	for(i=0;i<count;i++)
	{
		memcpy(&addr,&revdata[2*i],2);
		instance_set_alarmlist(addr,0,1);
	}
	event_timer_del(EVENT_URGENT_RESET);
	event_timer_add(EVENT_URGENT_RESET, 600000); 
}

void uart_rx_callback(unsigned char *pbuf, unsigned char len)
{
	uint8 *revdata;
//	uint8 i=0;
	uint16 count=0,addr;
//	PrintfUtil_vPrintf("uart rev data!\n");
	RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(pbuf+sizeof(bsmac_header_t) + sizeof(struct nwkhdr));

	if(len <sizeof(bsmac_header_t)+ sizeof(struct nwkhdr) + sizeof(app_header_t))
	{
		EDBG(PrintfUtil_vPrintf("err0 len =%d pt=%x  ;mt=%d\n",len,psAppPkt->tof_head.protocoltype,psAppPkt->tof_head.msgtype);)
			
		return;
	}
	
	
//	PrintfUtil_vPrintf("msgtype= %d  ; protype= %d\n",psAppPkt->tof_head.msgtype,psAppPkt->tof_head.protocoltype);
	//not for me
	if(APP_PROTOCOL_TYPE_UWB_CARD != psAppPkt->tof_head.protocoltype)
		return;
	revdata = (uint8*)(psAppPkt+1);
	switch(psAppPkt->tof_head.msgtype)
	{
	case  APP_UWB_MSG_RETREAT://撤离 5
		instance_set_alarmlist(0,0xFF,1);
/*		if(revdata[0]==0xFF) //all start alam
		{
			instance_set_alarmlist(0,0xFF,1);
		}
		else
		{
			count = revdata[1];
			for(i=0;i<count;i++)
			{
				memcpy(&addr,&revdata[2],2);
				instance_set_alarmlist(addr,0,1);
			}
		}
*/			
		break;

	case APP_UWB_MSG_CANCEL_RETREAT:    //取消撤离6
		instance_set_alarmlist(0,0xFF,0);
/*		if(revdata[0]==0x00)     //all stop alam
		{
			instance_set_alarmlist(0,0xFF,0);
		}
		else   //some card stop retreat alarm
		{
	//		count = revdata[2];
		//	for(i=0;i<count;i++)
			{
				memcpy(&addr,&revdata[0],2);
				instance_set_alarmlist(addr,0,0);
			}
		}
*/		
		break;

	case APP_UWB_MSG_ALARM_ACK:     //求救返回ack
		//int i=0;
		//for(i=0;i<psAppPkt->tof_head.len/2;i++)
		if(psAppPkt->tof_head.len >2)
		{
			memcpy(&addr,&revdata[0],2);
			instance_set_helpstatus(addr);
		}
		break;

	case APP_UWB_MSG_REQ_LOC_DISTANCE:
		event_timer_set(EVENT_UART_SEND);
		break;

	case APP_UWB_MSG_SET:
		 app_header_t * pheader = (app_header_t *)psAppPkt;
        	 app_LSrfSet_t * prfSet = (app_LSrfSet_t *)(pheader + 1);
		 if((prfSet->hdr.dstaddr == u16ShortAddr || prfSet->hdr.dstaddr == 0xFFFF)
                && pheader->len  >=  (sizeof(app_LSrfSet_t) + sizeof(app_rfTlv_t))) //at least one tlv
		{
			uint_16 crc;
			crc = CRC16((uint8*)(prfSet + 1),pheader->len-sizeof(app_LSrfSet_t) , 0xFFFF);
			if(crc == prfSet->crc)
			{
				
				uint16 len = pheader->len - sizeof(app_LSrfSet_t);  // sum len of all tlvs
				app_rfTlv_t *pTlv = (app_rfTlv_t *)(prfSet+1);
			//	EDBG(PrintfUtil_vPrintf("Card dev change rx  len =%d  pheader->len=%d \n",len,pheader->len );)
				while(len >= (sizeof(app_rfTlv_t) + pTlv->len))
				{
					uint8 *pdata = (uint8*)(pTlv+1);
					if(pTlv->type == APP_LS_TLV_TYPE_UWB_CARD_1)
					{
						if(pTlv->len < MAX_SET_CARD_CNT*2)
						{
							//memcpy(&status,pdata,2);
							memcpy(&CardType_1s.u16CardNum[0],pdata,2);
							if(CardType_1s.u16CardNum[0]== 0xFFFF)
							{
								instance_set_sta_status(STATION_TYPE_DEVCH_1S);
								instance_reset_sta_status(STATION_TYPE_DEVCH_5S);
								memset(&CardType_5s,0,sizeof(tsCardTypeSet));
								CardType_5s.u8CardCnt =0;
								
							}
							else
							{
								//instance_set_sta_status(STATION_TYPE_DEVCH_ANY);
								instance_reset_sta_status(STATION_TYPE_DEVCH_1S |STATION_TYPE_DEVCH_5S);
								memcpy(&CardType_1s.u16CardNum,pdata,pTlv->len);
								CardType_1s.u8CardCnt = pTlv->len / 2;
								check_cardtype_buff(APP_LS_TLV_TYPE_UWB_CARD_1);
							}
						}
					}

					else if(pTlv->type == APP_LS_TLV_TYPE_UWB_CARD_5)
					{
						if(pTlv->len < MAX_SET_CARD_CNT*2)
						{
							memcpy(&CardType_5s.u16CardNum[0],pdata,2);
							if(CardType_5s.u16CardNum[0]== 0xFFFF)
							{
								instance_set_sta_status(STATION_TYPE_DEVCH_5S);
								instance_reset_sta_status(STATION_TYPE_DEVCH_1S);
								memset(&CardType_1s,0,sizeof(tsCardTypeSet));
								CardType_1s.u8CardCnt =0;
							}
							else
							{
								//instance_set_sta_status(STATION_TYPE_DEVCH_ANY);
								instance_reset_sta_status(STATION_TYPE_DEVCH_1S |STATION_TYPE_DEVCH_5S);
								memcpy(&CardType_5s.u16CardNum,pdata,pTlv->len);
								CardType_5s.u8CardCnt = pTlv->len / 2;
								check_cardtype_buff(APP_LS_TLV_TYPE_UWB_CARD_5);
							}
						}
					}
					else if(pTlv->type == APP_LS_TLV_TYPE_UWB_CARD_RETREAT)
					{
						card_retreat(pdata,pTlv->len);
					}
					len -= (sizeof(app_rfTlv_t)  + pTlv->len);
					pTlv = (app_rfTlv_t *)(pdata  + pTlv->len);
				}

			}
			else
			{
				EDBG(PrintfUtil_vPrintf("CRC failed %d %d\n", prfSet->crc, crc);)
			}
			event_timer_add(EVENT_DEVTYPE_RESET, 180000); 
		}
			
		break;
		case APP_UWB_MSG_LINK:
			vReportStatus();
			break;
		
		case APP_UWB_MSG_GAIN_SETTING:
			app_header_t * pheader1 = (app_header_t *)psAppPkt;
			 uint8* rssi = (uint8*)(pheader1+1);
			if(pheader1->len >1)
				return;
			PrintfUtil_vPrintf("----rev power data =%d | %d \n", pheader1->len,*rssi );
			instance_setpower_rssi(*rssi);
			if(instancegetrole() == SUB_STA)
			{
				instance_change_channel(ANCHOR_TOF_CHANNEL);
			}
			break;
			
	}
}

void Appclear_distancelist(void)
{
	int i=0;
	for(i=0;i<TOF_SLOT_LOC_PERIOD;i++) 
	{
		distlist[i].u16ShortAddr = 0x0;
		distlist[i].u16SeqNum = 0;
		distlist[i].u32StationDistance =0;
		distlist[i].u32LocDistance =0;
		distlist[i].u8Status =0;
		distlist[i].u8DevType=0;	
	}
	tof_count =0;
}


void vReportCardVersion(void)
{
    if(tsVersionReportLen>0 && tsVersionReportLen<=APP_TOF_VERSION_MAX_NUM)
    {
        app_header_t *pHeader = (app_header_t *)u32CommonBuf;
        //app_LSrfReport_t *pStationReport = (app_LSrfReport_t *)(pHeader + 1);
        app_UWBVersionReport_t *pVersionReport = (app_UWBVersionReport_t *)(pHeader + 1);

        /*app header */
        pHeader->msgtype = APP_UWB_MSG_CARD_VER_BATTRY;
        pHeader->protocoltype = APP_PROTOCOL_TYPE_UWB_CARD;
        pHeader->len = tsVersionReportLen*sizeof(app_UWBVersionReport_t);

		for(int i=0;i<tsVersionReportLen;i++)
		{
			memcpy(pVersionReport + i,&tsVersionReport[i],sizeof(app_UWBVersionReport_t));
		}
       // memcpy((uint8*)pVersionReport, (uint8*)tsVersionReport, tsVersionReportLen*sizeof(app_UWBVersionReport_t));
		bsmac_build_packet(u8UartSendBuf,(uint8 *)(u32CommonBuf), sizeof(app_header_t)+pHeader->len,BSMAC_FRAME_TYPE_DATA);
		
        /* clear the buffer after send */
        memset((uint8*)tsVersionReport, 0, APP_TOF_VERSION_MAX_NUM*sizeof(app_UWBVersionReport_t));
        tsVersionReportLen = 0;
    }
    else
    {
        tsVersionReportLen = 0;
    }

}

#endif

#ifdef DEC_UWB_SUB
void clear_car_cardmsg(uint8 start)
{
	int i=0;
	for(i=start;i<MAX_CAR_CARD_CNT;i++)
	{
		Car_revcardlist.cardmsg[i].devtype=0;
		Car_revcardlist.cardmsg[i].status =0;
		Car_revcardlist.cardmsg[i].u8cardaddr[0] =0;
		Car_revcardlist.cardmsg[i].u8cardaddr[1] =0;
	}
	if(start ==0)
	{
		Car_revcardlist.u8CardCnt =0;
		Car_revcardlist.m_distance=0;
		Car_revcardlist.s_distance=0;
	}
	else
	{
		Car_revcardlist.u8CardCnt =start;
	}	
}
#endif

//return 1 :cardtype have change ;return 0:no change
uint8 check_cardtype(uint16 cardid, uint8 cardtype)
{
	uint8 i=0;
	if(CardType_5s.u8CardCnt == 0&& CardType_1s.u8CardCnt==0)
		return 0;
	if(cardtype == CARD_1S)
	{
		for(i=0;i<CardType_5s.u8CardCnt;i++)
		{
			if(CardType_5s.u16CardNum[i] == cardid)
				return 1;
		}
	}
	else if(cardtype == CARD_5S)
	{
		for(i=0;i<CardType_1s.u8CardCnt;i++)
		{
			if(CardType_1s.u16CardNum[i] == cardid)
				return 1;
		}
	}
	return 0;
}


void reset_cardtype_list()
{
	memset(&CardType_5s,0,sizeof(tsCardTypeSet));
	memset(&CardType_1s,0,sizeof(tsCardTypeSet));
	CardType_5s.u8CardCnt =0;
	CardType_1s.u8CardCnt =0;
}
/*******************************************************************************/
void vInitParameter()
{
	memset((uint8*)tsVersionReport, 0, APP_TOF_VERSION_MAX_NUM*sizeof(app_UWBVersionReport_t));
}

static uint8 check_chang_ST(void)
{
	int8 rssi;
	rssi= instance_get_rssi();
	if(my_cardslotmsg.m_distance > TOF_MAX_DISTANCE_INLINE || rssi <TOF_MIN_RSSI_INLINE)
	{
		event_timer_add(EVENT_CHECK_IDLE, 25); 
		check_dis_rss = 1;      //goto rev new blink process
	}
	return 1;
}

static void App_SleepProc(void)
{
	int sleeptime=0;
	uint16 uptick=0,wakeuptime=CARD_WAKE_UP_MAX;
	uint32 temp=0,temp1=0;

	uptick = instance_get_uptick();
	
	dwt_setleds(0);
	//PrintfUtil_vPrintf(" helpask = %d App_SleepProc\n", helpask);
	if(helpask==0)     //if the button have down, not sleep
	{
		Stop_ledvbeep(0);
	//	dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG   | DWT_INT_RFTO, 0); 
		if((quiet_count <=5 && isMoving==False) || isMoving == True)
		{
			quiet_count ++;
			if(uptick ==0 && my_cardslotmsg.b1Used == IDLE)  //not rev anything and in idle status
			{
				if(u8cardtype ==1)
					sleeptime = CARD_1S_SEC_TIME;
				else if(u8cardtype ==5)
					sleeptime = CARD_5S_SEC_TIME;
				if(my_cardslotmsg.sleeptick == CARD_1S_SEC_TIME/4)
				{
					sleeptime = my_cardslotmsg.sleeptick;
				}
			}
			else    //have been rev blink or poll ,or in tof status
			{
				Check_card_lostnum();
				if(uptick <2)
					uptick=5;    //uptick is the tick should be wake up before of the card

				temp = (uint32)(GetSysClock()- instance_get_slot_starttick());
				if(set_helpover_sleep ==0) //求救过程信号未停止
				{
				/*	if(temp>15 && temp<(70 +(u8cardtype -1)*10) )   //store the normal value
					{
						avg_tof_tick = temp;
					}
					else
					{
						EDBG(PrintfUtil_vPrintf(" sleep ,temp =%d \n",temp);)
						temp =avg_tof_tick;
					}*/
					temp1 =(uint32)( temp-uptick) ;
					if(temp1>0xfffff000)
						temp1 = 0xffffffff- temp1;

					if(my_cardslotmsg.sleeptick >(temp1+11))
						sleeptime  = my_cardslotmsg.sleeptick - temp1-11;
					else
						sleeptime =0;
				}
				else   //just help beep over then enter sleep ,but it's have a different count
				{
					temp1 =(uint32)( temp-uptick) ;
					while(my_cardslotmsg.sleeptick <(temp1+11)){
						if(u8cardtype ==1)
							my_cardslotmsg.sleeptick += CARD_1S_SEC_TIME;
						else if(u8cardtype ==5)
							my_cardslotmsg.sleeptick += CARD_5S_SEC_TIME;
					}
					sleeptime  = my_cardslotmsg.sleeptick - temp1-11;
					set_helpover_sleep =0;
				}
				if(u8cardtype ==CARD_1S&& sleeptime >=2000)
					sleeptime = CARD_1S_SEC_TIME-CARD_WAKE_UP_MAX;
				else if(u8cardtype ==CARD_5S&& sleeptime >=7000)
					sleeptime = CARD_5S_SEC_TIME-CARD_WAKE_UP_MAX*2;
				
			}
			wakeuptime = CARD_WAKE_UP_MAX;	
			
	//		PrintfUtil_vPrintf(" stime = %d helpover = %d wtime = %d \n", sleeptime, set_helpover_sleep, wakeuptime);
		}
		else
		{
			DBG(PrintfUtil_vPrintf(" ***********  long sleep :: isMoving =%d >> quiet_count =%d\n",isMoving,quiet_count);)
			quiet_count =0 ;
			sleeptime = 120*1000;  //2minute
			instance_set_idle();
			wakeuptime = CARD_WAKE_UP_MAX+30;  //long sleep need more time to rev blink 
		}
		ever_rev_dis =0;
		
		if(u8cardtype ==CARD_5S)
		{
		//	sleeptime += 30;  //20ms more ,becuse the time is not synchronization ,not *0.88 anymore 22ms less    6S card is 30ms
			wakeuptime = CARD_WAKE_UP_MAX+20;
		}

		if(sleeptime >15)
		{
			system_sleep((uint32)sleeptime);
			dec_sleep_wakeup();
		}
	//	dwt_setrxtimeout(0);
	//	instancerxon(0,0);
		
		
		//PrintfUtil_vPrintf(" out sleep ,time=%d | %d|%d |%d |%d |%i\n\n",
		//                    sleeptime,temp,uptick,my_cardslotmsg.sleeptick,u16CardMotionCnt,instance_get_powerlever());//
			PrintfUtil_vPrintf(" out sleep ,time=%d | %d|%d |%d |%d |%d |%i isMoving = %d quiet_count = %d \n\n",
			sleeptime, //休眠时间
			temp,
			uptick,
			my_cardslotmsg.sleeptick,
			u16CardMotionCnt,		  //运动计数，达到超时值说明人卡稳定，修改为静止状态
			u16CardMotionTimeout,	  //运动计数超时值
			instance_get_powerlever(),//RSSI值
			isMoving,				  //运动状态
			quiet_count);			  //运动状态的计数

		if(my_cardslotmsg.status &UWB_CARD_STATUS_NOPWD)
			LED_Red_On();
		else
			LED_Green_On();
		event_timer_add(EVENT_RED_LED_FLASH,10);
	}
	else
	{
		DBG(PrintfUtil_vPrintf(" help not sleep \n");)
		wakeuptime = CARD_1S_SEC_TIME+100;
		PrintfUtil_vPrintf(" help not sleep \n");
		
		Check_card_lostnum();
		set_helpover_sleep =1;
		system_sleep(25);
		dec_sleep_wakeup();
		if(my_cardslotmsg.u16SeqNum - pre_seqnum>=1)
			ever_rev_dis =0;
		pre_seqnum = my_cardslotmsg.u16SeqNum;
	}
	
	// if idle  to much ,then reset the dw100
	pretick = GetSysClock();
	instance_set_slot_starttick(pretick);
	
	event_timer_add(EVENT_SLEEP_EVENT,wakeuptime );
	event_timer_set(EVENT_RAGING_REPORT);
	if((idle_list_count)>=CARD_RESET_NOTOF_COUNT && !new_inblink)//*u8cardtype
	{
		if(card_reset_dw1000() ==-1)  //reset the dw1000
			NVIC_SystemReset();
		idle_list_count=0;
		//instance_set_idle();
		my_cardslotmsg.b1Used = IDLE;
		PrintfUtil_vPrintf("--------reset----- -!\n");
	}
	
	idle_list_count++;
	
	
	if(my_cardslotmsg.b1Used != IDLE && helpask ==0)
	{
		event_timer_add(EVENT_CHECK_IDLE, wakeuptime-20); 
		instance_set_AnchorPanid((uint8)ANCHOR_TOF_PRO);
	//new 123	instance_set_AnchorPanid((uint8)ANCHOR_BLINK_PRO);

	}
	else if(my_cardslotmsg.b1Used == IDLE )
	{
		event_timer_add(EVENT_CHECK_IDLE, wakeuptime-5); 
		instance_set_AnchorPanid((uint8)ANCHOR_BLINK_PRO);
	}
	reset_appstate();

}

void AppEventsProcess(void)
{
	uint8 u8TimeOfSleep = 0;

  	uint32 events = event_timer_take_all();
#ifdef DEC_UWB_ANCHOR	
	if (events & EVENT_MMA8452Q_EVENT)
	{
		PrintfUtil_vPrintf(" u16CardMotionCnt = %d EVENT_MMA8452Q_EVENT\n\n", u16CardMotionCnt);
		motion_detect_int_close();
		isMoving = True;
		u16CardMotionCnt =0 ;
	    CardMotionDetect();          //检查卡的运动情况    
	//    AppUpdateCardMotionState(); //更新卡是否运动，改变卡的工作周期
	//	 DBG(PrintfUtil_vPrintf("----------------------------\n");)
	}
#endif
	/************************************ TDOA部分处理 start *************************************/
	//TDOA流程 只进行十次TDOA测距
	if(((events &(EVENT_TDOA_CARD_BOARDCAST)) && (u8TdoaCardSendCount <= TDOA_SLOW_CARD_SEND_COUNT) && 
		(u16TdoaQuickCardSendCount <= TDOA_QUICK_CARD_SEND_COUNT)))
	{		
		//关闭DW1000发送器 清除所有事件 包括所有的接收消息 基站需要再次打开接收
		dwt_forcetrxoff();
		//根据设备类型进行处理
		if (instancegetrole() == ANCHOR) //卡
		{		
			//若为待测卡则设置一个随机的睡眠时间，防止后期待测卡太多导致发送冲突
			u8TimeOfSleep = rand()%10;
			if (u8TimeOfSleep > 0)
			{
				Sleep((uint32)u8TimeOfSleep);
			}
			//进行发送帧封装并且启动DW1000的发送
			TdoaSendTagPollInTof();
			//只进行十次发送测距
			u8TdoaCardSendCount ++;
			
			if (u8TdoaCardSendCount == (TDOA_SLOW_CARD_SEND_COUNT + 1) )
			{
				//若TDOA部分测距结束，则触发TOF测距启动
				gstTofInstCardDate.u16CarsTofStart = TRUE;
				event_timer_set(EVENT_SEND_TOF_POLL);
			}
		}
		else if (instancegetrole() == TDOA_INST_TAG_STANDARD) //快发卡不用进行事件发送的随机延时
		{
			//进行发送帧封装并且启动DW1000的发送
			TdoaSendTagPollInTof();
			u16TdoaQuickCardSendCount ++;
		}
		else
		{
			//设置实例进行数据校验事件
			event_timer_unset(EVENT_RAGING_REPORT);
			event_timer_set(EVENT_RAGING_REPORT);
		}
		
	}
	
	//进行接收数据校验
	if (events & (EVENT_CHECKTDOA_REVMSG_EVENT))
	{
		//基站接收到标签消息后进行接收消息的检查和组包处理
		TdoaRxCardMsgProc();
		event_timer_set(EVENT_RAGING_REPORT);
		//执行完事件则进行事件标志清除，防止事件误入
		event_timer_unset(EVENT_CHECKTDOA_REVMSG_EVENT);
	}

	//进行串口数据发送
	if (events & (EVENT_UART_SEND_TDOA))
	{
		// PrintfUtil_vPrintf("u32Events = %d	EVENT_UART_SEND\n", u32Events);
		TdoaSendCardReportToUart();
		//instance_data[0].testAppState = TA_RX_WAIT_DATA;
		event_timer_unset(EVENT_UART_SEND); //清除串口发送事件，等待下一次数据到来的再此设置串口数据发送事件
		event_timer_set(EVENT_RAGING_REPORT);
	}
	
	//完成TDOA测距后进行TOF测距消息发送
	if ((events & (EVENT_SEND_TOF_POLL)) && (gstTofInstCardDate.u16CarsTofStart == TRUE))
	{	
		//确保收到最后一帧后才进行下一个基站的测距
		CardSendPollToStation();
		event_timer_set(EVENT_RAGING_REPORT);
	}

#ifndef DEC_UWB_TAG  //基站通过串口发送测距结果
	if (events & (EVENT_UART_SEND))
	{
		dwt_forcetrxoff();
		led_station_off();
		if(instancegetrole() == TAG)  //distance send
		{
		    vReportCardDistance();
			Appclear_distancelist();
			WatchdogReset();
		}

		dwt_setrxtimeout(0);
		instancerxon(0,0);
	}
#endif

	/************************************ TDOA部分处理 end *************************************/	
	
	if(events &(EVENT_NEWSLOT_EVENT))
	{
		uint16 cardid;
		uint8 type;
		pretick = GetSysClock();
		dwt_forcetrxoff();
		instance_set_slot_starttick(pretick);
		newstarttype =0;
		cur_slot =cur_slot%SUM_SLOT_COUNT;
		type = instance_get_listslotmsg(cur_slot);
		instance_clear_substa();
		if(type !=3 && type !=0)  //==1,2
		{
			newstarttype =0;
		//	if(ispower_newon==0)  
			{	
				if(type==1 || type==4)
				{
					instance_change_channel(ANCHOR_BLINK_CHANNEL);
					instance_set_event(0);   //blink 要求入网
				}
				else if(type ==2)
                {
					type = get_curslot_cardmsg(&cardid);
					if(check_cardtype(cardid,type))     //change the card's devtype or not
						instance_change_devtype();
					instance_change_channel(ANCHOR_TOF_CHANNEL);
					instance_set_event(1);   //poll  直接tof
				//	DBG(PrintfUtil_vPrintf("\n evnet :slot = %d , addr=%d\n",cur_slot,get_curslot_destaddr(cur_slot));)
				}
			}

			instance_set_seqnum();
			instancerxon(0,0);
			event_timer_unset(EVENT_RAGING_REPORT);			
			event_timer_set(EVENT_RAGING_REPORT);
		}
		else if(type ==3)    //uart send
		{
			instance_set_event(0);
			event_timer_unset(EVENT_UART_SEND);			
			event_timer_set(EVENT_UART_SEND);
		}
		else
			EDBG(PrintfUtil_vPrintf(" ***** Erro: slotmsgtype erro!***** \n ");)
		cur_slot++;
		
	}
	
	if (events & (EVENT_RAGING_REPORT))
	{	
			
	    instance_run(newstarttype);
		if((instancegetrole() == ANCHOR || instancegetrole() == SUB_STA )&&newstarttype ==1)
			newstarttype=0;

	}

	if (events & (EVENT_END_RCV_BEACON))
	{
	//	instance_set_alarmlist(0,0,0);                 //测试取消撤离过程	
	}
#ifdef DEC_UWB_ANCHOR
	if (events & EVENT_BUTTON_MSG)
	{
		uint8 type;
		type = AppButtonProc();
	    if(type==1)       //help ask
		{
			dwt_forcetrxoff();
			instance_set_AnchorPanid((uint8)ANCHOR_TOF_PRO); //tof pandi ask help
			helpask =1;
		}

	
	}
	if (events & EVENT_EXCIT_EVENT)
	{
		if(u8ExciteTure != TRUE)      //excit interrupt
			ProcessExcite();
		
		if(u8ExciteTure == TRUE) 
		{
			txexcit_wait_send(U16ExciterID);    //send to sub station
			u8ExciteTure = FALSE;
		}
	}
	if (events & EVENT_LED_HUNGER)
	{
		Stop_ledvbeep(1);
	}
	
	if (events & EVENT_HELP_MSG)
	{

	    AppHelpProc();
		if(instance_get_helpstatus()== 0 &&helpask ==1)
		{
			txhelp_call_send();
			PrintfUtil_vPrintf("begain send help !\n");
		}	
	}

	if (events & EVENT_URGENT_RETREAT)
	{
		uint8 temp = instance_get_retreatstatus() ;
		if(temp && (next_retreat == 1) )
		{
	    	//PrintfUtil_vPrintf("card set reteat beep !\n");      // 卡测试撤离过程
			AppAlarmProc();
		}
		else if(!temp)
		{
			PrintfUtil_vPrintf("1-retreat : %d | %d!\n",instance_get_retreatstatus(),next_retreat); 
			next_retreat = 1;
			helpask =0;
			Stop_ledvbeep(0);
		}
		else
			PrintfUtil_vPrintf("2-retreat : %d | %d!\n",instance_get_retreatstatus(),next_retreat); 
	}
#endif

	if (events & EVENT_URGENT_RESET)
	{

#ifndef DEC_UWB_ANCHOR	
		instance_set_alarmlist(0,0xFF,0);
#else
		my_cardslotmsg.status &= ~UWB_CARD_STATUS_RETREAT_ACK;
		next_retreat = 1;    //be ready for the next one to beep

#endif
	}

	if (events & EVENT_DEVTYPE_RESET)
	{
		reset_cardtype_list();
	}

	if (events & (EVENT_GREEN_LED_FLASH))
	{
	    LED_Green_Off();
	}

	if (events & (EVENT_RED_LED_FLASH))
	{
	    LED_Red_Off();
		LED_Green_Off();
		
	}

#if 0
#ifndef DEC_UWB_ANCHOR

	if (events & (EVENT_UART_SEND))
	{
		dwt_forcetrxoff();
		led_station_off();
		if(instancegetrole() == TAG ||(instancegetrole() == SUB_STA&& tof_count >0))  //distance send
		{
		    vReportCardDistance();
			Appclear_distancelist();
			WatchdogReset();
		}
		if(instancegetrole() == SUB_STA)      //sub no data then reset but not M sta
		{
			newstarttype =1;
			event_timer_set(EVENT_RAGING_REPORT);
		}

		if((sub_alarmmsg.alarmstatus & UWB_CARD_STATUS_HELP && instance_get_sta_status(STATION_TYPE_ALARM))
			|| (sub_alarmmsg.alarmstatus & UWB_CARD_STATUS_RETREAT_ACK &&rev_retreat_ack ==1))	//help msg send
		{
			rev_retreat_ack =0;
			vReportCardAlarm(sub_alarmmsg.alarmaddr,0xff,sub_alarmmsg.alarmstatus);
		}
		else if(sub_alarmmsg.alarmstatus & UWB_CARD_STATUS_IMPEL && instance_get_sta_status(STATION_TYPE_EXCIT))
		{
			vReportCardAlarm(sub_alarmmsg.alarmaddr,sub_alarmmsg.excitid,sub_alarmmsg.alarmstatus);
		}
		dwt_setrxtimeout(0);
		instancerxon(0,0);
	}

	if(events & (EVENT_REPORT_CARDVERSION))
	{
        static uint8 i;
        vReportCardVersion();
        
        if(++i % 2)
		    vReportStatus();
        
		event_timer_add(EVENT_REPORT_CARDVERSION, 60000); 
	}

#else
	if (events & (EVENT_CHECK_IDLE))
	{
		uint16 tick = instance_get_ifrevsig();
		if(my_cardslotmsg.b1Used == USED_TOF)
		{
			uint8 revpolltype = instance_get_revpolltype();
			PrintfUtil_vPrintf(" revpolltype = %d b1Used = %d helpask = %d check_dis_rss = %d  card SeqNum = %dEVENT_CHECK_IDLE\n", 
				revpolltype, my_cardslotmsg.b1Used, helpask, check_dis_rss, my_cardslotmsg.u16SeqNum);	
			if(!revpolltype)
			{
				if(helpask==0 && !check_dis_rss)
					instance_set_AnchorPanid((uint8)ANCHOR_BLINK_PRO);
			}
			else
				instance_set_revpolltype(1);
			bool_check_poll=1;
			/////////////////// 数据构造开始 ///////////////////
			 instance_set_AnchorPanid((uint8)ANCHOR_BLINK_PRO); //构造只发送blink包
			/////////////////// 数据构造结束 ///////////////////
			
		}
		else if(my_cardslotmsg.b1Used == IDLE)
		{
			tdoa_send();
		}
	}
	if (events & (EVENT_SLEEP_EVENT))
	{
		if(U8iCount >0) //excit
		{
			uint16 temp = (uint16)GetSysClock();
			if(temp -U16TailMs >100)
				U8iCount =0;
			else
			{
				event_timer_add(EVENT_SLEEP_EVENT, 50); 
				return;
			}
		}
		bool_check_poll=0;
		Check_used_status();
		dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG , 0); 	//after send the tdoa msg
		dwt_forcetrxoff();
		vCheck_Devtype_change();
		App_SleepProc();
		WatchdogReset();
		card_newslot_init();
		//dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG , 1); 	
	}
	
	
#endif
#endif
}

/**********************用于人卡与基站进行tof测距发送前数据组装*****************************/
void TofCardOfStation(uwb_tof_distance_ts *distance)
{
	//distlist[tof_count] 
	instance_data_t* pstInstMsg = TdoaGetOldLocalInstStructurePtr();
		
	uint16 destaddr;
	memcpy(&destaddr,&my_staslotmsg.dest_addr[0],ADDR_BYTE_SIZE);	
	distance->i8Rssi = pstInstMsg->i8rssi;
	
	if(pstInstMsg->mode == TAG)
	{
		distance->u16ShortAddr= destaddr;
		distance->u16SeqNum = my_staslotmsg.u16SeqNum;
		distance->u8Status = my_staslotmsg.status;
		distance->u8DevType = my_staslotmsg.u8DeviceType;
		distance->u32StationDistance =my_staslotmsg.m_distance ;
		
		distance->u32LocDistance = 0; 
	}

	return;
}


void led_tofresult(void)
{

#ifndef DEC_UWB_ANCHOR
	if( instanceoldrange()&& instancegetrole() == SUB_STA) //if no (&& instancegetrole() == SUB_STA) ,the main station can report the last distance
	{
		int precount=0;
		if(tof_count >0)
			precount = tof_count-1;
		else
			precount =0;
		instance_get_distancelist(&distlist[tof_count],&distlist[precount],1); //inset the pre seq distance
		tof_count ++;
	}
#endif
	
	if(instancenewrange())
	{
		uwb_tof_distance_ts *distance;
		int seqnum = 0 ,precount=0;
		ever_rev_dis =1;
		if(tof_count >0)
			precount = tof_count-1;
		else
			precount =0;
		seqnum = instance_get_seqnum();
#ifdef DEC_UWB_ANCHOR
		idle_list_count=0;
		if(instancegetrole() == ANCHOR)
		{
			
			event_timer_del(EVENT_SLEEP_EVENT);			
			event_timer_set(EVENT_SLEEP_EVENT);
			PrintfUtil_vPrintf("\n------------dist= %d cm  ; seq = %d \n",my_cardslotmsg.m_distance,seqnum);
		}
#else
		led_station_off();
		
		if (!instance_get_distancelist(&distlist[tof_count],&distlist[precount],0)&& instancegetrole() == SUB_STA)
			tof_count= precount;
		distance = (uwb_tof_distance_ts *)&distlist[tof_count];
		//else
			tof_count++;
		
		DBG(PrintfUtil_vPrintf("\n------------M_dist= %d cm  ; seq = %d  |%x\n",distance->u32StationDistance,seqnum,distance->u8Status);)

		count_tof++;
		if(instancegetrole() == SUB_STA)
		{
			
			DBG(PrintfUtil_vPrintf("------------S_dist= %d cm  cardid = %d \n\n",distance->u32LocDistance,distance->u16ShortAddr);)
			instance_clear_substa();
		}
		
		//tof_count++;
		if(tof_count >=18)
		{	
			event_timer_set(EVENT_UART_SEND);
		}
		
#endif

	}
#ifdef DEC_UWB_SUB
	if(instancenewCar())
	{
		int i=0,j=0;
		for(i=0;i<Car_revcardlist.u8CardCnt;i++)
		{
			if(instance_get_car_cardlist(&distlist[tof_count],&Car_revcardlist.cardmsg[i]))
				tof_count++;
			//PrintfUtil_vPrintf("--id = %d  |%d  |%d",distlist[tof_count-1].u16ShortAddr,Car_revcardlist.u8CardCnt,tof_count);
			if(tof_count >=TOF_SLOT_LOC_PERIOD)  //the list have been full
			{	
				event_timer_set(EVENT_UART_SEND);
				if(i<Car_revcardlist.u8CardCnt-1)
				{
					for(j=0;i<Car_revcardlist.u8CardCnt;i++,j++)
					{
						memcpy(&Car_revcardlist.cardmsg[j],&Car_revcardlist.cardmsg[i],sizeof(Car_cardsmsg_t));
					}
					clear_car_cardmsg(j);
					instance_set_car_rev();  //wait for the next time after the dislist have been send
				}
				return;
			}
				

		}
		clear_car_cardmsg(0);
		//PrintfUtil_vPrintf("********\n ");
	}
#endif
}


/*
* 系统入口
*/

void Appsleepinit(void)
{

	dwt_setdblrxbuffmode(0); //disable double RX buffer
	dwt_enableautoack(ACK_RESPONSE_TIME); //wait for 5 symbols before replying with the ACK

#if (DEEP_SLEEP == 1)
#if (DEEP_SLEEP_AUTOWAKEUP == 1)
	dwt_configuresleep(DWT_LOADUCODE|DWT_PRESRV_SLEEP|DWT_CONFIG|DWT_TANDV, DWT_WAKE_SLPCNT|DWT_WAKE_CS|DWT_SLP_EN); //configure the on wake parameters (upload the IC config settings)
#else
	//NOTE: on the EVK1000 the DEEPSLEEP is not actually putting the DW1000 into full DEEPSLEEP mode as XTAL is kept on
#if (DEEP_SLEEP_XTAL_ON == 1)
	dwt_configuresleep(DWT_LOADUCODE|DWT_PRESRV_SLEEP|DWT_CONFIG|DWT_TANDV, DWT_WAKE_CS|DWT_SLP_EN|DWT_XTAL_EN); //configure the on wake parameters (upload the IC config settings)
#else
	dwt_configuresleep(DWT_LOADUCODE|DWT_PRESRV_SLEEP|DWT_CONFIG|DWT_TANDV, DWT_WAKE_CS|DWT_SLP_EN); //configure the on wake parameters (upload the IC config settings)
#endif
#endif
#endif
}
void Application1(void)
{
	PrintfUtil_vPrintf("\n ************** main start 111*************\n");
}

void Application(void)
{
	PrintfUtil_vPrintf("\n ************** main start*************\n");

	if ((instancegetrole() == ANCHOR) && (instance_TxSpeed == QUICK))
	{
		instancesetrole(TDOA_INST_TAG_STANDARD);
	}

    Appsleepinit();
#ifndef DEC_UWB_ANCHOR
	
	msg_analyser_register(uart_rx_callback);
	vReportStatus();

#endif
    LED_Red_Off();
    LED_Green_Off();
	vInitParameter();
	reset_cardtype_list();

	//初始化人卡进行tof所用的结构体
	TofInstanceCardDataInit();
	
	//设置TDOA事件
	if(instancegetrole() == ANCHOR)
	{
		event_timer_set(EVENT_TDOA_CARD_BOARDCAST);
		//event_timer_add(EVENT_TDOA_CARD_BOARDCAST, 50); //不需要重复加载只需要延时启动
		event_timer_add_reload(EVENT_TDOA_CARD_BOARDCAST, SLOW_SPEED_TAG_SEND_TIME); //待测卡发送为45ms
	}
	else if (instancegetrole() == TDOA_INST_TAG_STANDARD)
	{
		event_timer_set(EVENT_TDOA_CARD_BOARDCAST);
		event_timer_add_reload(EVENT_TDOA_CARD_BOARDCAST, QUICK_SPEED_TAG_SEND_TIME); //快发卡发送为45ms
	}
	
	//原有
	if(instancegetrole() == TAG)//(mode==1)    //M_station
	{
//new123		event_timer_set(EVENT_NEWSLOT_EVENT);
		event_timer_add(EVENT_REPORT_CARDVERSION, 10); 
		
		instance_init_slotlist();
//new123		event_timer_add_reload(EVENT_NEWSLOT_EVENT, EVERY_SLOT_TIME);   
	}
	else
	{
		newstarttype =1;  //first power on ,the  anchor wake up enter the init status		
		event_timer_set(EVENT_RAGING_REPORT);
		if(instancegetrole() == ANCHOR)
			event_timer_add(EVENT_SLEEP_EVENT, CARD_WAKE_UP_MAX+20); 
        
		if(instancegetrole() == ANCHOR)
		{
			u8cardtype = instance_get_cardtype();
			instance_init_cardslot(u8cardtype); //1:1s card  5:5s card
			instance_set_idle();
		}
		else if(instancegetrole() == SUB_STA)
		{
			instance_clear_substa();
			event_timer_add(EVENT_REPORT_CARDVERSION, 10); 
			instance_change_channel(ANCHOR_TOF_CHANNEL);
		}
	}
    while (1)
    {

		if(instance_get_status()==1)
		{
			event_timer_unset(EVENT_RAGING_REPORT);
			event_timer_set(EVENT_RAGING_REPORT);
		}
		event_timer_update();
		AppEventsProcess();
//new123		led_tofresult();
	//	AppSavePower();

	}
}

