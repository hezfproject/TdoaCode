
#include "WatchDogUtil.h"
#include  "Hal_mcu.h"

//__near_func 
void  FeedWatchDog(void) // write 0xA followed by 0x5 within o.5 watchdog clock
{
	uint8 RS0,RS1;
	//halIntState_t intState;
		
	RS0 = RS1 = WDCTL;
	RS0 &= ~(0x0F<<WDCTL_CLR_POS);      
	RS0 |=   (0x0A<<WDCTL_CLR_POS);     
	RS1 &= ~(0x0F<<WDCTL_CLR_POS); 
	RS1 |=   (0x05<<WDCTL_CLR_POS); 

	//HAL_ENTER_CRITICAL_SECTION(intState);    
	WDCTL = RS0;
	WDCTL = RS1;
	//HAL_EXIT_CRITICAL_SECTION(intState);    
	/*
      EA = 0;
      WDCTL |= (0x0A<<WDCTL_CLR_POS); 
      WDCTL |= (0x05<<WDCTL_CLR_POS); 
      EA=1;
      */
	return;
}

bool IsWatchDogOn(void)
{
    return (WDCTL &  (0x01<<WDCTL_EN_POS));
}


void  StartWatchDog(uint8 interval)
{
	WDCTL &= ~(0x01<<WDCTL_MODE_POS);     // set mode to watchdog mode

      WDCTL &= ~(0x03<<WDCTL_INT_POS);        //set time interval
      WDCTL |= (interval&3)<<WDCTL_INT_POS;       

	WDCTL |=   (0x01<<WDCTL_EN_POS);          //Enable
}

void ChangeWatchDogInterval(uint8 interval)
{
    FeedWatchDog();
    WDCTL &= ~(0x03<<WDCTL_INT_POS);        //set time interval
    WDCTL |= (interval&3)<<WDCTL_INT_POS;       

}

uint8 GetResetFlag(void)
{
	return ((SLEEP & (0x03<<SLEEP_RST_POS))>>SLEEP_RST_POS);
}
