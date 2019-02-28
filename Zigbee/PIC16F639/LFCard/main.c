#include <htc.h>
#include "spi.h"
#include "afe.h"


// ÈÛË¿ÅäÖÃ²ÎÊý

//__CONFIG(INTIO & WDTDIS & PWRTEN & MCLRDIS & UNPROTECT & BORXSLP & IESODIS & FCMDIS & WAKECNT);
__CONFIG(INTIO & WDTDIS & PWRTDIS & MCLRDIS & UNPROTECT & BORDIS & IESODIS & FCMDIS & WAKECNT);

_LFCMD LF_CMD;


void main()
{
//	unsigned char *Paddr_vol;
//	unsigned char *Pdata_vol;
	OSC_INIT();
	PORT_INIT();
	delay(1);


	SET_AFE();
	INT_AFE_INIT();
    IIC_Init();
    asm("sleep");

	while(1)
	{
        //I2C_SDA_H;
        //I2C_SCL_H;
        //delay_200us(10);  // 1100us
        //I2C_SDA_L;
        //I2C_SCL_L;
        //delay(200);      //1s dealy

        if(rec_count > 500)
        {
            rec_count = 0;
            start_LFReceiver();//INTE = 1;
            asm("sleep");
        }

    	AFE_NEW_data();
        delay(200);      //1s dealy
	}
}










