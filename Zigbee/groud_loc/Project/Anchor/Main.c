/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "App.h"
#include "CPU.h"
#include "Console.h"
#include "NtrxDrv.h"
#include "LED.h"
#include "Utility.h"
#include "KeyScan.h"
#include "hal_uart.h"

/* OSAL */
#include "OSAL.h"
#include "OSAL_Tasks.h"

extern void PollApplication( void ) ;
extern void InitApplication( void ) ;

//-----------------------------------------------------------------------------

int	main( void )
{
	CpuInitialize() ;			// CPU��ʼ��
	
	LED1_Off() ;	  			//On��Off��AVR�����෴
	LED2_Off() ;				//1�Ǻ죬2����

	LED1_On() ;
	LED2_On() ;

	HalUARTConfig(App_UartCB);	//UART�Ӵ��ڣ����ڵ��жϣ�

	CpuEnableInterrupt() ;		// �����ж�

	PrintVersionMessage() ;		// ��ض˿�����汾��Ϣ

	NtrxInit(1) ;				// ��ʼ��NanotronоƬ
	
	osal_init_system();

	HalKeyConfig(True, App_KeyCB);	  //�������¼���������ַ������1�룩

	osal_start_system();
}

//-----------------------------------------------------------------------------
// �쳣������
//   0 : δ֪����(�п����Ǳ�̽׶δ���),����ʹ�������¸�λ����
//   1 : оƬ�汾�쳣,CPU�޷�����NanotronоƬ
//   2 : ����Ϊ���ɲ���ģʽȴ�����˲�ຯ��
//   3 : �Ĵ��������,��̽׶δ���
//   4 : ȱʡ���ͽ���ģʽ����,��̽׶δ���

void ErrorHandler( uint8 err )
{
	uint32 start;

	con_PutNumber( CSTR( "ErrorHandler: " ), err ) ;

	if( err == 2 )		// ��ຯ������᷵�ش���ֵ
		return ;

	if( err == 0 )
		(*((void(*)(void))0))() ;	// �쳣����,��������

	con_PutString( CSTR( "System halted !\n" ) ) ;	// ��̴���,��ѭ���ȴ��������

	start = GetSysClock() ;

	while( 1 )
	{
		WatchdogReset() ;
		//ISP_Service() ;
		if( GetSysClock() - start >= 500 )
		{
			start = GetSysClock() ;
			if( err++ & 1 )
			{
				LED1_On() ;
				LED2_Off() ;
			}
			else
			{
				LED1_Off() ;
				LED2_On() ;
			}
		}
	}
}
