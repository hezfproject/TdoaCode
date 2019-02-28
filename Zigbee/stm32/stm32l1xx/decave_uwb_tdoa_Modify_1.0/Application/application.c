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
��������:AppEventsProcess
��������:�����в������¼�����
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void AppEventsProcess(void)
{
	int iTdoaInstMode;
	uint8 u8TimeOfSleep = 0;
  	uint32 u32Events = event_timer_take_all();
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	
	if (u32Events & (EVENT_NEWSLOT_EVENT))
	{
		//����У��
		//Tdoa_check_data(u32Events);
		
		//�ر�DW1000������ ��������¼� �������еĽ�����Ϣ ��վ��Ҫ�ٴδ򿪽���
		dwt_forcetrxoff();
		//�����豸���ͽ��д���
		if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_TEST)
		{		
			//��Ϊ���⿨������һ�������˯��ʱ�䣬��ֹ���ڴ��⿨̫�ർ�·��ͳ�ͻ
			u8TimeOfSleep = rand()%10;
			if (u8TimeOfSleep > 0)
			{
				Sleep((uint32)u8TimeOfSleep);
			}
			//���з���֡��װ��������DW1000�ķ���
			TdoaSendTagPoll();
		}
		else if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_STANDARD) //�췢�����ý����¼����͵������ʱ
		{
			//��Ϊ���⿨������һ�������˯��ʱ�䣬��ֹ���ڴ��⿨̫�ർ�·��ͳ�ͻ
			u8TimeOfSleep = rand()%10;
			if (u8TimeOfSleep > 0)
			{
	//			Sleep((uint32)u8TimeOfSleep);
			}
			//���з���֡��װ��������DW1000�ķ���
			TdoaSendTagPoll();
            
            //�췢�������źź� ���մ��⿨����
            dwt_rxenable(0); 
		}
		else if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_ANCHOR)
		{
			//����ʵ����������У���¼�
			event_timer_unset(EVENT_RAGING_REPORT);
			event_timer_set(EVENT_RAGING_REPORT);
		}		
	}
	
	if (u32Events & (EVENT_RAGING_REPORT))
	{
		TdoaInstRunState();
	    event_timer_unset(EVENT_RAGING_REPORT);
		//�������ﵽ�������и�λDW1000
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
	
	//���н�������У��
	if (u32Events & (EVENT_CHECKTDOA_REVMSG_EVENT))
	{
		//��վ���յ���ǩ��Ϣ����н�����Ϣ�ļ����������
		TdoaRxCardMsgProc();
		//event_timer_set(EVENT_RAGING_REPORT);
		//ִ�����¼�������¼���־�������ֹ�¼�����
		event_timer_unset(EVENT_CHECKTDOA_REVMSG_EVENT);
	}

	//���д������ݷ���
	if (u32Events & (EVENT_UART_SEND))
	{
		// PrintfUtil_vPrintf("u32Events = %d	EVENT_UART_SEND\n", u32Events);
		TdoaSendCardReportToUart();
		pstInstMsg->stInstBaseData.eTdoaInstState = TDOA_RX_WAIT_DATA;
		event_timer_unset(EVENT_UART_SEND); //������ڷ����¼����ȴ���һ�����ݵ������ٴ����ô������ݷ����¼�
		//event_timer_set(EVENT_RAGING_REPORT);
	}

	return;
}


/*
* ϵͳ���
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
��������:DecaTdoaEquipEventSet
��������:���ÿ��ķ������ڣ�������Ϣ��ѯ�¼�

����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void DecaTdoaEquipEventSet(void)
{
	uint32 u32RloadTime = 0;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

	//�������豸�������ö�Ӧ����ѯ����
	if(pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_TEST) //����Ƿ���������
	{
		event_timer_set(EVENT_NEWSLOT_EVENT);
		u32RloadTime = SLOW_SPEED_TAG_SEND_TIME; //���⿨��������
		event_timer_add_reload(EVENT_NEWSLOT_EVENT, u32RloadTime);
	}
	else if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_STANDARD)
	{
		event_timer_set(EVENT_NEWSLOT_EVENT);
		u32RloadTime = QUICK_SPEED_TAG_SEND_TIME; //�췢����������
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
��������:Application
��������:ϵͳӦ��ִ����ڣ���Ҫ�������¹��ܲ���
		1.ϵͳӦ�ó�ʼ��
		2.����ϵͳ�ڴ��ʼ��
		3.����tickֵ��λ
		4.�����ж�
		5.����Ӳ����ʼ��
		6.����Ӧ�ó�����

����˵��:uint8 usModeNum ģʽ����
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

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

