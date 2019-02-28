
#include "WatchDogUtil.h"
#include  "Hal_mcu.h"


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

