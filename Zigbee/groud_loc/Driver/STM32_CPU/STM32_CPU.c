/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
 release
******************************************************************************/
//=============================================================================
//stm32 lib header file
#include "stm32f10x.h"
#include "system_stm32f10x.h"

//=============================================================================
//stm32 driver file
#include "CPU.h"
#include "sleep.h"
//#include "hal_uart.h"
//#include "hal_spi.h"
//#include "STM32_CPU\STM32_SPI.c"
//#include "STM32_CPU\hal_uart.c"
//#include "_hal_uart_isr.c"


#include "Config.h"

void CpuResetCheck( void ) ;
void App_RCC_Configuration( void ) ;
void App_IO_Configuration( void ) ;
void App_NVIC_Configuration( void ) ;

typedef void (*__IsrFunction)( void ) ;
extern __IsrFunction __Vectors[] ;		// ��stm32f10x_vector.s�ļ��ж���

//=============================================================================

#define SysTick_ClockSource     SysTick_CLKSource_HCLK_Div8
#define SYSTICK_CLOCK_KHz		( HSE_Value * PLL_MUL / 8 / 1000 )
#define SysTickReloadValue		( SYSTICK_CLOCK_KHz * BASE_TIME_TICK )

//�Զ���Ĳ�����ÿ�������ʱ��RT_TICK_PER_SECOND�ж�
#define RT_TICK_PER_SECOND	(1000 / BASE_TIME_TICK)

//=============================================================================

u32 RCC_GetHSE_Value( void )
{
	return HSE_Value ;
}

void RCC_Configuration( void )
{
	#if 0
	//SystemInit();

	//#else
	ErrorStatus HSEStartUpStatus ;

	// RCC system reset ( for debug purpose )
	RCC_DeInit() ;

	// Enable HSE
	RCC_HSEConfig( RCC_HSE_ON ) ;
	// Wait till HSE is ready
	HSEStartUpStatus = RCC_WaitForHSEStartUp() ;
	if( HSEStartUpStatus == SUCCESS )
	{
		// Enable Prefetch Buffer
		FLASH_PrefetchBufferCmd( FLASH_PrefetchBuffer_Enable ) ;
		// Flash 2 wait state
		FLASH_SetLatency( FLASH_Latency_2 ) ;
		// HCLK = SYSCLK
		RCC_HCLKConfig( RCC_SYSCLK_Div1 ) ;
		// PCLK2 = HCLK
		RCC_PCLK2Config( RCC_HCLK_Div1 ) ;
		// PCLK1 = HCLK/2
		RCC_PCLK1Config( RCC_HCLK_Div2 ) ;
		// ADCCLK = PCLK2/6
//		RCC_ADCCLKConfig( RCC_PCLK2_Div6 ) ;			//δʹ�ã��ر�

		// PLLCLK = HSE_Value * PLL_MUL
		#if ( PLL_MUL >= 2 ) && ( PLL_MUL <= 16 )
			RCC_PLLConfig( RCC_PLLSource_HSE_Div1, (u32)(PLL_MUL-2)<<18 ) ;
		#else
			#error Unsuitable PLL_MUL !
		#endif

		// Enable PLL
		RCC_PLLCmd( ENABLE ) ;
		// Wait till PLL is ready
		while( RCC_GetFlagStatus( RCC_FLAG_PLLRDY ) == RESET ) ;
		// Select PLL as system clock source
		RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK ) ;
		// Wait till PLL is used as system clock source
		while( RCC_GetSYSCLKSource() != 0x08 ) ;
	}
#endif
	con_UART_RCC_Configuration() ;
	Ntrx_SPI_RCC_Configuration() ;
	App_RCC_Configuration() ;
}

//3.5�����Ѿ�û�У��Լ���ԭд����ӡ�
/*******************************************************************************
* Function Name  : NVIC_DeInit
* Description    : Deinitializes the NVIC peripheral registers to their default
*                  reset values.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NVIC_DeInit(void)
{
  u32 index = 0;

  NVIC->ICER[0] = 0xFFFFFFFF;
  NVIC->ICER[1] = 0x000007FF;
  NVIC->ICPR[0] = 0xFFFFFFFF;
  NVIC->ICPR[1] = 0x000007FF;

  for(index = 0; index < 0x0B; index++)
  {
     NVIC->IP[index] = 0x00000000;
  }
}


//PRIMASK: ֻ����NMI��hard fault�жϡ�
//FAULTMASK: ֻ����NMI�жϡ�
void NVIC_Configuration( void )
{
	NVIC_InitTypeDef NVIC_InitStructure ;

	NVIC_DeInit() ;		//������NVIC�Ĵ�������Ϊȱʡֵ����д2.0�⺯����δ��֤��	
	
	//NVIC_SETPRIMASK() ;
	__set_PRIMASK(1);	//�ر����жϡ� ��3.5�̼��⺯������
		
	/* Set the Vector Table base address at 0x08000000 */
	NVIC_SetVectorTable( NVIC_VectTab_FLASH, (uint32_t)(&__Vectors[0]) - NVIC_VectTab_FLASH ) ;
	//NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x00000);		   //Sets the vector table location and Offset.
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

	//NVIC_RESETPRIMASK() ;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;

	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1 ;

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init( &NVIC_InitStructure ) ;	
	
	//App_NVIC_Configuration() ;
	__set_PRIMASK(0);	//�������жϡ� ��3.5�̼��⺯������
}

//л����
void  SysTick_Configuration(const u32 cnts)
{
	RCC_ClocksTypeDef  rcc_clocks;
//	u32 cnts;
//	cnts = (u32)rcc_clocks.HCLK_Frequency / RT_TICK_PER_SECOND;

	RCC_GetClocksFreq(&rcc_clocks);

	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK); 				//ϵͳʱ��Ƶ��	

	SysTick_Config(cnts);											//counterֵ���ʱΪ10ms
}

/*3.5����,core_cm3.h���ṩ�ĺ���*/
//static __INLINE uint32_t SysTick_Config(uint32_t ticks)
//{ 
//  if (ticks > SysTick_LOAD_RELOAD_Msk)  return (1);            /* Reload value impossible */
//                                                               
//  SysTick->LOAD  = (ticks & SysTick_LOAD_RELOAD_Msk) - 1;      /* set reload register */
//  NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1);  /* set Priority for Cortex-M0 System Interrupts */
//  SysTick->VAL   = 0;                                          /* Load the SysTick Counter Value */
//  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | 
//                   SysTick_CTRL_TICKINT_Msk   | 
//                   SysTick_CTRL_ENABLE_Msk;                    /* Enable SysTick IRQ and SysTick Timer */
//  return (0);                                                  /* Function successful */
//}

//����
// void SysTick_Configuration(void)
// {
//    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);			//ʱ�ӳ�8
//    SysTick_SetReload(250000);     //                            //�������ڳ���
//    SysTick_CounterCmd(SysTick_Counter_Enable);                //������ʱ��
//    SysTick_ITConfig(ENABLE);                                  //���ж�
// }

// void SysTick_Configuration( void )
// {
// 	SysTick_CLKSourceConfig( SysTick_ClockSource ) ;
// 	NVIC_SystemHandlerPriorityConfig( SysTick_IRQn, 3, 0 ) ;
// 	SysTick_SetReload( SysTickReloadValue ) ;
// 	SysTick_ITConfig( ENABLE ) ;
// 	SysTick_CounterCmd( SysTick_Counter_Enable ) ;
// }

void IWDG_Configuration( void )
{
	#ifdef WATCHDOG_ENABLE
		IWDG_WriteAccessCmd( IWDG_WriteAccess_Enable ) ;
		IWDG_SetPrescaler( IWDG_Prescaler_32 ) ;
		IWDG_SetReload( 1000 ) ;
		IWDG_ReloadCounter() ;
		IWDG_Enable() ;
	#endif
}

void CpuInitialize( void )
{
	CpuResetCheck() ;
	RCC_Configuration() ;
	NVIC_Configuration() ;

	SysTick_Configuration(SystemCoreClock*BASE_TIME_TICK/1000) ;	//ÿ10msһ���жϣ�ÿ��100���ж�

	//ʹ��3.5�Ŀ⺯����core_cm3.h��.
	//�ú����������Զ������������(LOAD)��ֵ��SysTick IRQ�����ȼ�����λ�˼�����(VAL)��ֵ����ʼ��������SysTick IRQ�жϡ�
	//SysTickʱ��Ĭ��ʹ��ϵͳʱ�ӡ�
	 /* SystemCoreClock is defined in ��system_stm32f10x.h�� and equal to HCLK frequency */
	//SysTick_Config(SystemCoreClock*BASE_TIME_TICK/1000);

	IWDG_Configuration() ;
	con_UART_Configuration() ;	//
	
	Ntrx_SPI_Configuration() ;	  //
	App_IO_Configuration() ;
	
	SleepInit();	
}

void CpuEnableInterrupt( void )
{
	__set_PRIMASK(0);		//�������жϡ� ��3.5�̼��⺯������
	__set_FAULTMASK(0);
}
  
void CpuDisableInterrupt( void )
{
	__set_PRIMASK(1);		//�ر����жϡ� ��3.5�̼��⺯������
	__set_FAULTMASK(1);
}

void WatchdogReset( void )
{
	#ifdef WATCHDOG_ENABLE
		IWDG_ReloadCounter() ;
	#endif
}

//=============================================================================
//static uint16 _SysCountDownTimer = 0 ;
//static uint32 _Jiffies = 0 ;

//static uint16 _irqCount = 0;
volatile uint8 systicks = 0;

void SysTick_Handler( void )			//�жϴ�����
{
//	if( _SysCountDownTimer >= BASE_TIME_TICK )
//		_SysCountDownTimer -= BASE_TIME_TICK ;
//	else
//		_SysCountDownTimer = 0 ;
	systicks += BASE_TIME_TICK ;    //ms��
	AppTimerTickLoop() ;

	//systicks++;				 //���irq��������
}

//irq����ֵ����
void RestartSysTickIrqCounter(void)
{  	
	systicks = 0;
}

//��ȡirq����ֵ
uint16 GetSysTickIrqCounter(void)
{
	return systicks;
}

//void SetCountDownTimer( uint16 t )
//{
//	_SysCountDownTimer = t ;
//}

//uint16 GetCountDownTimer( void )
//{
//	return _SysCountDownTimer ;
//}

uint32 GetSysClock( void )
{
	//return _Jiffies ;
	return systicks;
}

//=============================================================================


/*******************************************************************************
* Function Name  : SysTick_GetCounter
* Description    : Gets SysTick counter value.
* Input          : None
* Output         : None
* Return         : SysTick current value

˵������ȡ  < Systick ��ʱ���ĵļ���ֵ >
*******************************************************************************/
u32 SysTick_GetCounter(void)	//���2.0��ĺ�����2.5��
{
  return(SysTick->VAL);
}

void ElapsedTime(uint32 ms)
{
	CpuDisableInterrupt();
	//_Jiffies += ms;
    systicks += (ms+BASE_TIME_TICK-1)/BASE_TIME_TICK;
	CpuEnableInterrupt();
}

static void _delay_clock( uint32 StartSysTickValue, uint32 DelayValue )
{
	uint32 t ;
	// ��ʱ�ӳ���,������BASE_TIME_TICKʱ��
	while( 1 )
	{
		t = SysTick_GetCounter() ;	//3.5�������Ѿ�û���ˡ�
		//t = SystemCoreClockUpdate();
		
		if( t < StartSysTickValue )
			t = StartSysTickValue - t ;
		else
			t = SysTickReloadValue - t + StartSysTickValue ;
		if( t >= DelayValue )
			return ;
	}
}

static void _delay_us( uint32 t )
{
	uint32 s ;
	s = SysTick_GetCounter() ;
	t *= ( SYSTICK_CLOCK_KHz / 1000 ) ;
	while( t > SysTickReloadValue / 2 )
	{
		_delay_clock( s, SysTickReloadValue / 2 ) ;
		t -= ( SysTickReloadValue / 2 ) ;
		if( s > SysTickReloadValue / 2 )
			s -= ( SysTickReloadValue / 2 ) ;
		else
			s += ( SysTickReloadValue / 2 ) ;
		WatchdogReset() ;
	}
	_delay_clock( s, t ) ;
}

void Delay_ms( uint16 ms )
{
	_delay_us( (uint32)ms * 1000 ) ;
}

void Delay_us(__IO uint16 us )
{
	_delay_us( (uint32)us ) ;
//	while (us--)
//	{
//		
//	}
}

//=============================================================================
#if 1
const uint8 AT_CommandKey[] = "\7AT\r" ;
const uint8 AT_AnswerKey[] = "STM32IAP\r\n"; 
const uint8 IAP_EnterKey[] = "?��??IAP\r" ;
static uint8 AT_CommandIndex = 0;
static uint8 IAP_CommandIndex = 0;
void ISP_Service( uint8* buf, uint8 len )
{
	uint8 iapkeysize = sizeof(IAP_EnterKey) - 1;
	uint8 atkeysize = sizeof(AT_CommandKey) - 1;
	uint8 i;
	for(i = 0; i < len; i++){
		if(AT_CommandIndex < atkeysize){
			if(buf[i] == AT_CommandKey[AT_CommandIndex]){
				AT_CommandIndex++;
			}else{
				AT_CommandIndex = 0;
			}
		}
		if(AT_CommandIndex >= atkeysize){
			uint8 k;
			for(k = 0; k < sizeof(AT_AnswerKey) - 1; k++){
				con_putchar(AT_AnswerKey[k]);
			}
		}
		
		if(IAP_CommandIndex < iapkeysize){
			if(buf[i] == IAP_EnterKey[IAP_CommandIndex]){
				IAP_CommandIndex++;
			}else{
				IAP_CommandIndex = 0;
			}
		}
		if(IAP_CommandIndex >= iapkeysize){
			//NVIC_SETFAULTMASK() ;
			//NVIC_GenerateSystemReset() ;

			//__set_PRIMASK(0);		
			__set_FAULTMASK(0);			//�������жϡ� ��3.5�̼��⺯������
			NVIC_SystemReset();
			while( 1 ) ;
		}
	}
}

#endif

