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
	CpuInitialize() ;			// CPU初始化
	
	LED1_Off() ;	  			//On和Off与AVR板上相反
	LED2_Off() ;				//1是红，2是绿

	LED1_On() ;
	LED2_On() ;

	HalUARTConfig(App_UartCB);	//UART接串口，串口的中断？

	CpuEnableInterrupt() ;		// 启动中断

	PrintVersionMessage() ;		// 监控端口输出版本信息

	NtrxInit(1) ;				// 初始化Nanotron芯片
	
	osal_init_system();

	HalKeyConfig(True, App_KeyCB);	  //处理按键事件（增减地址，休眠1秒）

	osal_start_system();
}

//-----------------------------------------------------------------------------
// 异常错误处理
//   0 : 未知错误(有可能是编程阶段错误),可以使程序重新复位运行
//   1 : 芯片版本异常,CPU无法控制Nanotron芯片
//   2 : 设置为不可测距的模式却调用了测距函数
//   3 : 寄存器表错误,编程阶段错误
//   4 : 缺省发送接收模式错误,编程阶段错误

void ErrorHandler( uint8 err )
{
	uint32 start;

	con_PutNumber( CSTR( "ErrorHandler: " ), err ) ;

	if( err == 2 )		// 测距函数本身会返回错误值
		return ;

	if( err == 0 )
		(*((void(*)(void))0))() ;	// 异常错误,重新运行

	con_PutString( CSTR( "System halted !\n" ) ) ;	// 编程错误,死循环等待软件升级

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
