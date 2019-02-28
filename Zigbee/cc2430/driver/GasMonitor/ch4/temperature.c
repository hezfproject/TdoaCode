#include <iocc2430.h>
#include "hal_types.h"
#include "ZComDef.h"
#include "delay.h"
#include "temperature.h"
#include "GasMenulib_global.h"
#include "app_cfg.h"
#include "OSAL_Nv.h"

static tyrTemp_t TyrTemp;

static const int16 temp_coeff = 245;
static uint8 update_flash(void);
/**********************************************************************
* Description:
get temperautre by reading from adc channel
* Return value:
temperature value (0.1 digree)
**********************************************************************/
int16 tem_get_temperature(void)
{
	int16 adc[3];
	int32 adc_mean;
	int32 temperature;
	for (uint8 i=0;i<3;i++)
	{
		adc[i] = tem_get_adc();
	}
	adc_mean = (adc[0]+adc[1]+adc[2])/3;
	
	temperature = (adc_mean-TyrTemp.value)*305/(2*temp_coeff) + TyrTemp.temp; // 305/2 = 1.25*1000*10*100/8192
	return  (int16)temperature;
}

/**********************************************************************
* Description:
calibration in a normal temperature point
* Return value:
calibration success or failed
**********************************************************************/

bool tem_calibration(int16 temp)
{
	int value1,value2,value3;
	if(temp>= TEM_CALIBTEMP_MIN && temp<=TEM_CALIBTEMP_MAX)
	{
		TyrTemp.temp =  temp;
		
	}
	value1 = tem_get_adc();
	value2 = tem_get_adc();
	value3 = tem_get_adc();
	TyrTemp.value = (value1+value2+value3)/3;
	
	update_flash();
	return true;
}

bool tem_set_tempCtrl(tyrTemp_t tyrTemp)
{
	TyrTemp = tyrTemp;
	return true;
}

tyrTemp_t tem_get_tempCtrl(void)
{
	return TyrTemp;
}
bool tem_getnum_fromtemper(char str[], int16 temper)
{
	uint8 n1,n2,n3;
	bool sign;

	int16 tem;
	
	tem = temper;
	if(tem>=0)
	{
		sign = true;
	}
	else
	{
                tem =-tem;
		sign = false;
	}
	n1= tem/100;
	tem -= n1*100;
	n1 = (n1)%10;
	n2 = tem/10;
	tem -= n2*10;
	n3 = tem;

	char *p = str;
	if(n1 == 0)
	{
		*p++ = ' ';
		if(sign)
		{
			*p++ = ' ';
		}
		else
		{
			*p++ = '-';
		}
		*p++ =n2+'0';
		*p++ =n3+'0';
		*p++ = '\0';
	}
	else
	{
		if(sign)
		{
			*p++ = ' ';
		}
		else
		{
			*p++ = '-';
		}
		*p++ =n1+'0';		
		*p++ =n2+'0';
		*p++ =n3+'0';
		*p++ = '\0';
	}
	
	return true;

	
}

/**********************************************************************
* Description:
get adc value of temperature sensor
**********************************************************************/
int16 tem_get_adc(void)
{
	int16 adc_val;

	/* Clear ADC interrupt flag */
	ADCIF = 0;
	adc_val = 0;	//initial adc_val;

	ADCCON3 = (0x00<<6|0x03<<4|0x0E<<0);//8'b 00(1.25V) 11(14bit) 1110(temperature)
	ADCCON1 = 0x73; //8'b 0111 0011

	/* Wait for the conversion to finish */
	while ( !ADCIF );

	adc_val = ADCH;
	adc_val = adc_val << 8; //get adc high byte
	adc_val |= ADCL; //get adc low byte
	adc_val = adc_val >> 2;

	if(adc_val & (0x01<<13))
	{
		adc_val = 0xC000;
	}

	return adc_val;

}


/**********************************************************************
* Description:
get adc value of temperature sensor
**********************************************************************/
uint8  update_flash(void)
{
	set_info_t info;
	uint8 flag1,flag2;
	
	flag1 = osal_nv_read(GASMONITOR_NV_SETTINGS, 0, sizeof(info), &info);
	if(flag1 == ZSuccess)
	{
		info.tyrTemper = TyrTemp;
		flag2 = osal_nv_write(GASMONITOR_NV_SETTINGS, 0, sizeof(info), &info);
		if(flag2 == ZSuccess)
		{
			return ZSuccess;
		}
	}
	return ZFailure;
}


