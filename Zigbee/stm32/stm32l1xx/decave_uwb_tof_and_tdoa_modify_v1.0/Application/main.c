/******************************************************************************
 Copyright 2012
******************************************************************************/
#include "stm32l1xx.h"
#include "mem.h"
#include "debug.h"
#include "board.h"
//#include "timer_event.h"
#include "instance.h"

#include "compiler.h"
#include "port.h"
#include "printf_util.h"
#include "config.h"

#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$ZI$$Limit;
#else
#error "not armcc"
#endif


int instance_anchaddr = 0;
int dr_mode = 0;
#define SLOW 0
#define QUICK 1

int instance_mode = ANCHOR;
int instance_TxSpeed = SLOW;

#define STM32_HEAP_BEGIN    (void *)(&Image$$RW_IRAM1$$ZI$$Limit)
#define STM32_SRAM_SIZE     16
#define STM32_HEAP_END      (void *)(0x20000000 + STM32_SRAM_SIZE * 1024)

int inittestapplication()
{
    uint32 devID ;
    instanceConfig_t instConfig;
    int  result;

    SPI_ConfigFastRate(SPI_BaudRatePrescaler_16);  //max SPI before PLLs configured is ~4M
	
	//this is called here to wake up the device (i.e. if it was in sleep mode before the restart)
    devID = instancereaddeviceid() ;
	PrintfUtil_vPrintf("devID = %d DWT_DEVICE_ID = %d\n", devID, DWT_DEVICE_ID);
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

	//reset the DW1000 by driving the RSTn line low
	reset_DW1000();

    result = instance_init() ;
    if (0 > result) return(-1) ; // Some failure has occurred

    SPI_ConfigFastRate(SPI_BaudRatePrescaler_4); //increase SPI to max
    devID = instancereaddeviceid() ;

    if (DWT_DEVICE_ID != devID)   // Means it is NOT MP device
    {
        // SPI not working or Unsupported Device ID
		return(-1) ;
    }

	instancesetrole(instance_mode) ;     // Set this instance role

    dr_mode = 0;//decarangingmode();

    instConfig.channelNumber = chConfig[dr_mode].channel ;
    instConfig.preambleCode = chConfig[dr_mode].preambleCode ;
    instConfig.pulseRepFreq = chConfig[dr_mode].prf ;
    instConfig.pacSize = chConfig[dr_mode].pacSize ;
    instConfig.nsSFD = chConfig[dr_mode].nsSFD ;

    instConfig.dataRate = chConfig[dr_mode].datarate ;
    instConfig.preambleLen = chConfig[dr_mode].preambleLength ;

    instance_config(&instConfig) ;                  // Set operating channel etc

#if (DR_DISCOVERY == 0)
    addressconfigure() ;                            // set up initial payload configuration
#endif
    instancesettagsleepdelay(500); //set the Tag sleep time

    //if(is_button_low() == S1_SWITCH_ON)
    	instancesetreplydelay(FIXED_REPLY_DELAY);
    //else
    	//instancesetreplydelay(FIXED_LONG_REPLY_DELAY);

    //use this to set the long blink response delay (e.g. when ranging with a PC anchor that wants to use the long response times)
    //if(is_switch_on(TA_SW1_8) == S1_SWITCH_ON)
    	//instancesetblinkreplydelay(FIXED_LONG_BLINK_RESPONSE_DELAY);

    //reset_DW1000();
    
	PrintfUtil_vPrintf("return devID = %d \n", devID);

    return devID;
}



int main(void)
{
	board_init();

	PrintfUtil_u8Init();


#ifdef  DEC_UWB_ANCHOR
		instance_mode = ANCHOR;  //卡
		//instance_TxSpeed = QUICK; //SLOW为待测卡 QUICK为快发卡
#else
	
		instance_mode = SUB_STA; //辅站
#endif

#ifdef DEC_UWB_TAG
		instance_mode = TAG;     //主站
#endif

#ifdef DEC_UWB_TAG_STANDARD
			instance_mode = TDOA_INST_TAG_STANDARD;	 //快发卡
#endif
	
	Application1();

	NVIC_DisableIRQ(EXTI0_IRQn);
	DBG(PrintfUtil_vPrintf("\n ************** main start*************\n");)
	reset_DW1000();

	if(inittestapplication() == -1)
	{
		EDBG(PrintfUtil_vPrintf("Init dw1000 error\n");)
		while(1)
		{
			LED_Red_Flash();
			LED_Green_Flash();
			mSleep(200);
		}
	}

	NVIC_EnableIRQ(EXTI0_IRQn);
	portResetTickCnt();

	Application();
}

