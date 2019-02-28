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
#include "config.h"
#include "Application.h"



extern TDOA_INSTANCE_DATA_S gstTdoaInstData;
TDOA_UWB_MSG_PACK_SEND_S gTdoaSendPack;
//uint16 u16HdlTxFrameCnt = 0;

#define MAX_TIMER_TICK  (0xFFFFFFFF/2)

/*****************************************************************************
* CONSTANTS AND DEFINES
*/

/*****************************************************************************
* TYPEDEFS
*/

/*****************************************************************************
*   DECLARATION
*/
Bool isMoving = True;


#ifdef TOA_RESULT
static Toa_Result_t toaFrm;
#endif


/*****************************************************************************
* FUNCTION
*/

void vCheckBattery()
{
	
}


void process_deca_irq(void)
{
    do{

    	instance_process_irq(0);
    }while(port_CheckIRQ() == 1); //while IRS line active (ARM can only do edge sensitive interrupts)

}


/*************************************************************
函数名称:AppEventsProcess
函数描述:程序中产生的事件处理
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void AppEventsProcess(void)
{
	int iTdoaInstMode;
	uint8 u8TimeOfSleep = 0;
  	uint32 u32Events = event_timer_take_all();
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	
	if (u32Events & (EVENT_NEWSLOT_EVENT))
	{
		//参数校验
		//Tdoa_check_data(u32Events);
		
		//关闭DW1000发送器 清除所有事件 包括所有的接收消息 基站需要再次打开接收
		dwt_forcetrxoff();
		//根据设备类型进行处理
		if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_TEST)
		{		
			//若为待测卡则设置一个随机的睡眠时间，防止后期待测卡太多导致发送冲突
			u8TimeOfSleep = rand()%10;
			if (u8TimeOfSleep > 0)
			{
				Sleep((uint32)u8TimeOfSleep);
			}
			//进行发送帧封装并且启动DW1000的发送
			TdoaSendTagPoll();
		}
		else if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_STANDARD) //快发卡不用进行事件发送的随机延时
		{
			//若为待测卡则设置一个随机的睡眠时间，防止后期待测卡太多导致发送冲突
			u8TimeOfSleep = rand()%10;
			if (u8TimeOfSleep > 0)
			{
	//			Sleep((uint32)u8TimeOfSleep);
			}
			//进行发送帧封装并且启动DW1000的发送
			TdoaSendTagPoll();
            
            //快发卡发送信号后 接收待测卡数据
            dwt_rxenable(0); 
		}
		else if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_ANCHOR)
		{
			//设置实例进行数据校验事件
			event_timer_unset(EVENT_RAGING_REPORT);
			event_timer_set(EVENT_RAGING_REPORT);
		}		
	}
	
	if (u32Events & (EVENT_RAGING_REPORT))
	{
		TdoaInstRunState();
	    event_timer_unset(EVENT_RAGING_REPORT);
		//丢包数达到五次则进行复位DW1000
		if (pstInstMsg->stInstBaseData.i8LostPollPackCount == TDOA_LOST_POLL_PACK_COUNT)
		{
			PrintfUtil_vPrintf("LostPollPackCount = %d TdoaInstMode = %d\n", 
							pstInstMsg->stInstBaseData.i8LostPollPackCount, 
							pstInstMsg->stInstBaseData.eTdoaInstMode);
			iTdoaInstMode = pstInstMsg->stInstBaseData.eTdoaInstMode;
			AppInstanceInit(iTdoaInstMode);	
			pstInstMsg->stInstBaseData.i8LostPollPackCount = 0;
		}
	}
	
	//进行接收数据校验
	if (u32Events & (EVENT_CHECKTDOA_REVMSG_EVENT))
	{
		//基站接收到标签消息后进行接收消息的检查和组包处理
		TdoaRxCardMsgProc();
		//event_timer_set(EVENT_RAGING_REPORT);
		//执行完事件则进行事件标志清除，防止事件误入
		event_timer_unset(EVENT_CHECKTDOA_REVMSG_EVENT);
	}

	//进行串口数据发送
	if (u32Events & (EVENT_UART_SEND))
	{
		// PrintfUtil_vPrintf("u32Events = %d	EVENT_UART_SEND\n", u32Events);
		TdoaSendCardReportToUart();
		pstInstMsg->stInstBaseData.eTdoaInstState = TDOA_RX_WAIT_DATA;
		event_timer_unset(EVENT_UART_SEND); //清除串口发送事件，等待下一次数据到来的再此设置串口数据发送事件
		//event_timer_set(EVENT_RAGING_REPORT);
	}

	return;
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

/*************************************************************
函数名称:DecaTdoaEquipEventSet
函数描述:设置卡的发送周期，开启消息轮询事件

参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
void DecaTdoaEquipEventSet(void)
{
	uint32 u32RloadTime = 0;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

	//根据是设备类型设置对应的轮询周期
	if(pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_TEST) //检查是否是慢发卡
	{
		event_timer_set(EVENT_NEWSLOT_EVENT);
		u32RloadTime = SLOW_SPEED_TAG_SEND_TIME; //待测卡发送周期
		event_timer_add_reload(EVENT_NEWSLOT_EVENT, u32RloadTime);
	}
	else if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_STANDARD)
	{
		event_timer_set(EVENT_NEWSLOT_EVENT);
		u32RloadTime = QUICK_SPEED_TAG_SEND_TIME; //快发卡发送周期
		event_timer_add_reload(EVENT_NEWSLOT_EVENT, u32RloadTime);
	}
	else
	{
		event_timer_set(EVENT_NEWSLOT_EVENT);
		event_timer_add_reload(EVENT_UART_SEND, DEVICE_UART_SEND_TIME);	//not big than a card's cycle time
		event_timer_add(EVENT_REPORT_CARDVERSION, 1000); 
		event_timer_add(EVENT_CARD_VER_BATTERY_EVENT,1000);
	}
	
	return;
}

/*************************************************************
函数名称:Application
函数描述:系统应用执行入口，主要进行以下功能操作
		1.系统应用初始化
		2.进行系统内存初始化
		3.进行tick值复位
		4.开启中断
		5.进行硬件初始化
		6.进行应用程序处理

参数说明:uint8 usModeNum 模式类型
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
void Application(void)
{
	PrintfUtil_vPrintf("\n/********************** main start **********************/\n");
	
	int iTdoaInstMode = TDOA_INST_MODE_TYPE; 	
	
	AppInstanceInit(iTdoaInstMode);	
	NVIC_EnableIRQ(EXTI0_IRQn);

    while (1)
    {
		WatchdogReset();
		event_timer_update();
		AppEventsProcess();
	}	
	
}

