/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "App.h"
#include "KeyScan.h"
#include "LED.h"
#include "OLED.h"
#include "BoardConfig.h"

//=============================================================================

#include "CPU.h"

//=============================================================================
// IO��������

void App_RCC_Configuration( void )
{
	// Enable LED Control GPIO clock
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIO_LED, ENABLE ) ;
	// Enable KEY-SW GPIO clock
	#ifdef ENABLE_KEY
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIO_KEY, ENABLE ) ;
	#endif
	// Enable OLED GPIO clock
	#ifdef ENABLE_OLED
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIO_OLED, ENABLE ) ;
	#endif
}

void App_IO_Configuration( void )
{
	GPIO_InitTypeDef GPIO_InitStructure ;

	// Configure LED_A Port
	GPIO_InitStructure.GPIO_Pin = LED_A_PIN ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;
	GPIO_Init( LED_A_PORT, &GPIO_InitStructure ) ;

	// Configure LED_B Port
	GPIO_InitStructure.GPIO_Pin = LED_B_PIN ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;
	GPIO_Init( LED_B_PORT, &GPIO_InitStructure ) ;

	// Configure KEY-SW Port
	#ifdef ENABLE_KEY
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU ;
	GPIO_InitStructure.GPIO_Pin = KEY_SW1_PIN ;
	GPIO_Init( KEY_SW1_PORT, &GPIO_InitStructure ) ;
	GPIO_InitStructure.GPIO_Pin = KEY_SW2_PIN ;
	GPIO_Init( KEY_SW2_PORT, &GPIO_InitStructure ) ;
	GPIO_InitStructure.GPIO_Pin = KEY_SW3_PIN ;
	GPIO_Init( KEY_SW3_PORT, &GPIO_InitStructure ) ;
	GPIO_InitStructure.GPIO_Pin = KEY_SW4_PIN ;
	GPIO_Init( KEY_SW4_PORT, &GPIO_InitStructure ) ;
	GPIO_InitStructure.GPIO_Pin = KEY_SW5_PIN ;
	GPIO_Init( KEY_SW5_PORT, &GPIO_InitStructure ) ;
	#endif

	// Configure OLED Ctrl Port
	#ifdef ENABLE_OLED
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;
	GPIO_InitStructure.GPIO_Pin = OLED_SCL_PIN ;
	GPIO_Init( OLED_SCL_PORT, &GPIO_InitStructure ) ;
	GPIO_InitStructure.GPIO_Pin = OLED_SI_PIN ;
	GPIO_Init( OLED_SI_PORT, &GPIO_InitStructure ) ;
	GPIO_InitStructure.GPIO_Pin = OLED_CS_PIN ;
	GPIO_Init( OLED_CS_PORT, &GPIO_InitStructure ) ;
	GPIO_InitStructure.GPIO_Pin = OLED_RES_PIN ;
	GPIO_Init( OLED_RES_PORT, &GPIO_InitStructure ) ;
	GPIO_InitStructure.GPIO_Pin = OLED_A0_PIN ;
	GPIO_Init( OLED_A0_PORT, &GPIO_InitStructure ) ;
	#endif
}

void App_NVIC_Configuration( void )
{
	NVIC_InitTypeDef NVIC_InitStructure ;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1 ;
	NVIC_Init( &NVIC_InitStructure ) ;
}

//-----------------------------------------------------------------------------
// ������״̬

uint8 GetKeyStatus( void )
{
	uint8 k = 0 ;
	if( ! ( GPIO_ReadInputData( KEY_SW1_PORT ) & KEY_SW1_PIN ) )
		k |= KEY_SW1 ;
	if( ! ( GPIO_ReadInputData( KEY_SW2_PORT ) & KEY_SW2_PIN ) )
		k |= KEY_SW2 ;
	if( ! ( GPIO_ReadInputData( KEY_SW3_PORT ) & KEY_SW3_PIN ) )
		k |= KEY_SW3 ;
	if( ! ( GPIO_ReadInputData( KEY_SW4_PORT ) & KEY_SW4_PIN ) )
		k |= KEY_SW4 ;
	if( ! ( GPIO_ReadInputData( KEY_SW5_PORT ) & KEY_SW5_PIN ) )
		k |= KEY_SW5 ;
	return k ;
}

//-----------------------------------------------------------------------------

void NtrxSSN_Lo( void )	  
{
	//Clears the selected data port bits.
	GPIO_ResetBits( NTRX_SSN_PORT, NTRX_SSN_PIN ) ;
}

void NtrxSSN_Hi( void )
{
	//GPIO_SetBits( NTRX_SSN_PORT, NTRX_SSN_PIN ) ;
}

void NtrxResetOn( void )
{
	GPIO_ResetBits( NTRX_RST_PORT, NTRX_RST_PIN ) ;
}

void NtrxResetOff( void )
{
	GPIO_SetBits( NTRX_RST_PORT, NTRX_RST_PIN ) ;
}

void NtrxCtrlInit( void )
{
	GPIO_InitTypeDef GPIO_InitStructure ;

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIO_SSN | RCC_APB2Periph_GPIO_RST | RCC_APB2Periph_GPIO_DIO, ENABLE ) ;

	GPIO_InitStructure.GPIO_Pin = NTRX_SSN_PIN ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;
	GPIO_Init( NTRX_SSN_PORT, &GPIO_InitStructure ) ;

	GPIO_InitStructure.GPIO_Pin = NTRX_RST_PIN ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;
	GPIO_Init( NTRX_RST_PORT, &GPIO_InitStructure ) ;

	GPIO_InitStructure.GPIO_Pin = NTRX_DIO0_PIN ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;
	GPIO_Init( NTRX_DIO_PORT, &GPIO_InitStructure ) ;

	NtrxResetOn() ;
	NtrxSSN_Hi() ;
}

void NtrxPrepWakeupByDio( void )
{
	NtrxClrDio0();
}

void NtrxWakeupByDio( void )
{
	NtrxSetDio0();
}

//=============================================================================
// CPU����͹���״̬

void EXTI0_IRQHandler( void )
{
	EXTI_ClearITPendingBit( EXTI_Line0 );
}

void CpuEnterPowerStop( void )
{
	ErrorStatus HSEStartUpStatus ;
	EXTI_InitTypeDef EXTI_InitStructure ;

	// ����WKUP(PA0)���Ż���
	GPIO_EXTILineConfig( GPIO_PortSourceGPIOA, GPIO_PinSource0 ) ;
	EXTI_InitStructure.EXTI_Line = EXTI_Line0 ;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt ;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising ;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE ;
	EXTI_Init( &EXTI_InitStructure ) ;

	// ����STOPģʽ
	WatchdogReset() ;
	PWR_EnterSTOPMode( PWR_Regulator_LowPower, PWR_STOPEntry_WFI ) ;
	WatchdogReset() ;

	// �ر��ж�
	EXTI_InitStructure.EXTI_Line = EXTI_Line0 ;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt ;
	EXTI_InitStructure.EXTI_LineCmd = DISABLE ;
	EXTI_Init( &EXTI_InitStructure ) ;

	// STOPģʽ���Ѻ���������CLOCK
	RCC_HSEConfig( RCC_HSE_ON ) ;
	HSEStartUpStatus = RCC_WaitForHSEStartUp() ;
	if( HSEStartUpStatus == SUCCESS )
	{
		RCC_PLLCmd( ENABLE ) ;
		while( RCC_GetFlagStatus( RCC_FLAG_PLLRDY ) == RESET )
		{
		}
		RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK ) ;
	    while( RCC_GetSYSCLKSource() != 0x08 )
		{
		}
	}
}

void CpuEnterPowerSave( void )
{
	#ifndef WATCHDOG_ENABLE
		RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE ) ;	// ����WKUP(PA0)���Ż���
		PWR_WakeUpPinCmd( ENABLE ) ;
		PWR_EnterSTANDBYMode() ;	// �������״̬
	#else
		// �������Ź����������Ѵ���״̬,��˲��ø�λ���ٽ�������ķ���
		RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE ) ;
		PWR_BackupAccessCmd( ENABLE ) ;
		BKP_WriteBackupRegister( BKP_DR1, 0xB4FD ) ;	// "��"�ֵ�����
		BKP_WriteBackupRegister( BKP_DR2, 0xBBFA ) ;	// "��"�ֵ�����
	#endif

	//NVIC_SETFAULTMASK() ;		// ���ڴ���״̬RAM�����ݶ�ʧ,��˻��Ѻ����λ��������
	//__set_PRIMASK(1);		//�ر����жϡ� ��3.5�̼��⺯������
	//__set_FAULTMASK(1);		// ���ڴ���״̬RAM�����ݶ�ʧ,��˻��Ѻ����λ��������

	//NVIC_GenerateSystemReset() ;???????????
	//NVIC_SystemReset() ;
	//while( 1 ) ;
}

void CpuResetCheck( void )
{
	#ifdef WATCHDOG_ENABLE
		RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE ) ;
		PWR_BackupAccessCmd( ENABLE ) ;
		if( RCC_GetFlagStatus( RCC_FLAG_SFTRST ) == SET )
			if( ( BKP_ReadBackupRegister( BKP_DR1 ) == 0xB4FD ) && ( BKP_ReadBackupRegister( BKP_DR2 ) == 0xBBFA ) )
			{
				RCC_ClearFlag() ;
				BKP_WriteBackupRegister( BKP_DR1, 0 ) ;
				BKP_WriteBackupRegister( BKP_DR2, 0 ) ;
				PWR_WakeUpPinCmd( ENABLE ) ;
				PWR_EnterSTANDBYMode() ;	// �������״̬
			}
	#endif
}

//=============================================================================
// ����STM32F103CB, ��Flashģ��EEPROM,
// ����ֻ�������洢255���ֽ�,ʹ��RAM�������ӿ��ٶ�


#define EEPROM_SIZE             255
#define EEPROM_FLASH_BLOCK1     ((uint32)0x0801F800)
#define EEPROM_FLASH_BLOCK2     ((uint32)0x0801FC00)

#define PAGE_VALID_ID           0x4F4B

static uint32 FlashPage = 0 ;
static uint32 LastIndex = 0 ;
static uint8  EepromBuffer[ EEPROM_SIZE ] ;

//-------------------------------------------------------------------------
// ��ʼ��,�ж���Чҳ,����ȡ���ݵ�������

static void EepromInitCheck( void )
{
	uint16 t ;

	if( FlashPage != 0 )
		return ;

	FLASH_Unlock() ;
	for( t = 0 ; t < EEPROM_SIZE ; t ++ )
		EepromBuffer[ t ] = 0xFF ;

	if( *(uint16*)EEPROM_FLASH_BLOCK1 == PAGE_VALID_ID )
		FlashPage = EEPROM_FLASH_BLOCK1 ;
	else if( *(uint16*)EEPROM_FLASH_BLOCK2 == PAGE_VALID_ID )
		FlashPage = EEPROM_FLASH_BLOCK2 ;
	else
	{
		FlashPage = 1 ;
		if( FLASH_ErasePage( EEPROM_FLASH_BLOCK1 ) != FLASH_COMPLETE )
			return ;
		if( FLASH_ProgramHalfWord( EEPROM_FLASH_BLOCK1, PAGE_VALID_ID ) != FLASH_COMPLETE )
			return ;
		FlashPage = EEPROM_FLASH_BLOCK1 ;
	}
	for( LastIndex = 0 ; LastIndex < EEPROM_SIZE ; LastIndex ++ )
		EepromBuffer[ LastIndex ] = 0xFF ;
	LastIndex = 2 ;
	while( 1 )
	{
		t = *(uint16*)(FlashPage+LastIndex) ;
		if( (uint8)t >= EEPROM_SIZE )
			break ;
		EepromBuffer[ (uint8)t ] = (uint8)(t>>8) ;
		LastIndex += 2 ;
		if( LastIndex == 0x400 )
			break ;
	}
}

//-----------------------------------------------------------------------------
// �л��洢ҳ: ������������д���µ�ҳ,��������ǰҳ

static void EepromSwitchPage( void )
{
	uint32 DstPage, DstIndex = 2 ;
	uint16 i ;
	DstPage = EEPROM_FLASH_BLOCK1 + EEPROM_FLASH_BLOCK2 - FlashPage ;
	FlashPage = 1 ;
	if( FLASH_ErasePage( DstPage ) != FLASH_COMPLETE )
		return ;
	for( i = 0 ; i < EEPROM_SIZE ; i ++ )
		if( EepromBuffer[ i ] != 0xFF )
		{
			if( FLASH_ProgramHalfWord( DstPage+DstIndex, ((uint16)EepromBuffer[i]<<8) | i ) != FLASH_COMPLETE )
				return ;
			DstIndex += 2 ;
		}
	if( FLASH_ProgramHalfWord( DstPage, PAGE_VALID_ID ) != FLASH_COMPLETE )
		return ;
	FLASH_ErasePage( EEPROM_FLASH_BLOCK1 + EEPROM_FLASH_BLOCK2 - DstPage ) ;
	FlashPage = DstPage ;
	LastIndex = DstIndex ;
}

//-----------------------------------------------------------------------------
// EEPROM��,ֻ�������������

uint8 EepromRead( uint16 EE_Addr )
{
	EepromInitCheck() ;
	if( EE_Addr >= EEPROM_SIZE )
		return 0xFF ;
	return EepromBuffer[ EE_Addr ] ;
}

//-----------------------------------------------------------------------------
// EEPROMд,����û��Ͳ���д,��ǰҳ���˾��л��洢ҳ

void EepromWrite( uint16 EE_Addr, uint8 EE_Data )
{
	EepromInitCheck() ;
	if( EE_Addr >= EEPROM_SIZE )
		return ;
	if( EepromBuffer[ EE_Addr ] == EE_Data )
		return ;
	EepromBuffer[ EE_Addr ] = EE_Data ;
	if( FlashPage == 1 )
		return ;
	if( LastIndex >= 0x400 )
	{
		EepromSwitchPage() ;
		return ;
	}
	if( FLASH_ProgramHalfWord( FlashPage+LastIndex, ((uint16)EE_Data<<8) | EE_Addr ) != FLASH_COMPLETE )
		FlashPage = 1 ;
	else
		LastIndex += 2 ;
}


//=============================================================================
