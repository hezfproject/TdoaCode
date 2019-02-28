/**********************************************************************
*dirver for ch4-detector, including density reading, zero-adjust and calibration
* the basic formula is ch4_dens = slope_a * (adc_val - zero_val)
**********************************************************************/

#include <iocc2430.h>
#include "Ch4.h"
#include "hal_types.h"
#include "ZComDef.h"
#include "OSAL_NV.h"
#include "GasMenulib_global.h"
#include "app_cfg.h"
/**********************************************************************
* Local macro
**********************************************************************/
#define REF_MIN_SCALE       (500)
#define SLOPE_MAX_VAL		(0xffff)
#define CH4_ENLDO_BIT		(3)
#define CH4_LDOADC_BIT		(1)
#define CH4_VAL_SHIFT		(15)
/**********************************************************************
* Local structure and variable
**********************************************************************/
static TYRCH4 yrCh4;

/**********************************************************************
* Local function
**********************************************************************/
static uint8 update_flash(void);
// set precision threshold of calibration
//static int8 ch4_set_cal_prec(uint32 cal_prec);
/**********************************************************************
* Description:
do ch4 driver initialize
* Return value:
0 : success
others: fail
**********************************************************************/


int8 ch4_init(void)
{
	// initial driver structure
	//yrCh4.slope_a = 1500;
	//yrCh4.zero_val = 0;//400;
	//yrCh4.cal_value = 0;

	// initial adc hardware
	ADCCFG |= (1<<CH4_LDOADC_BIT);     //Set P0_1 as ADC input
	//ADCCFG &= ~(1<<CH4_LDOADC_BIT);    //set P0_1 as GPIO
	//P0DIR |= (1<<CH4_LDOADC_BIT);           // output
	//P0INP |= (1<<CH4_LDOADC_BIT);		// tristate
		
	P1SEL &= ~(1<<CH4_ENLDO_BIT);	// set to gpio
	P1DIR |=   (1<<CH4_ENLDO_BIT);     // set as output

	ch4_LdoCtrl(true);

	return 0;
}
/**********************************************************************
* Description:
get density of ch4 by reading from adc channel
* Return value:
density value
**********************************************************************/
uint16 ch4_get_density(void)
{
	uint32 rs = 0;
	uint32 adc_val = 0;
	uint32 diff = 0;

	// get adc value
	adc_val = ch4_get_adc_value();

	if (adc_val > yrCh4.zero_val) {
		diff = adc_val - yrCh4.zero_val;
	}

	rs = yrCh4.slope_a * diff;

	rs = (rs >> CH4_VAL_SHIFT) & 0x1ff;

	const uint32 slopB = 5;

	if(rs > 200)
	{
		rs = rs + (rs-200)/slopB;
	}

	return rs;
}

/**********************************************************************
* Description:
do zero adjust, just record the value for zero point by placing ch4-dector in pure air
* Return value:
0 : success
others : fail
**********************************************************************/
int8 ch4_zero_adjust(void)
{
	uint16 adc_val = 0;

	adc_val = ch4_get_adc_value();

	yrCh4.zero_val = adc_val;

	update_flash();

	return 0;
}

/**********************************************************************
* Description:
do calibration, by placing ch4-dector into a known-density ch4-air, and the density is set
by user interface
* Return value:
0 : success
others: fail
**********************************************************************/
int8 ch4_calibration(void)
{
	uint32 adc_val;//dens_val;
	//	uint32 step_val = yrCh4.cal_step;
	uint32 cal_value2 = yrCh4.cal_value  << CH4_VAL_SHIFT;

	cal_value2 +=  0x01<<(CH4_VAL_SHIFT-1);

	adc_val = ch4_get_adc_value();

	if (adc_val < yrCh4.zero_val) return -1;  //adc value should not less than zero. Maybe zero adjust error

	if ((adc_val - yrCh4.zero_val) < REF_MIN_SCALE) return -2; //must have lager referenc ch4 density value

	yrCh4.slope_a = (cal_value2)/(adc_val - yrCh4.zero_val);

	update_flash();
	return 0;

}

/**********************************************************************
* Description:
get value of adc channel
* Return value:
the value
**********************************************************************/
uint16 ch4_get_adc_value(void)
{
	uint16 adc_val;

	/* Clear ADC interrupt flag */
	ADCIF = 0;

	/* open the channel */
	//ADCCFG |= (1<<CH4_LDOADC_BIT);   	  //set P0_1 as ADC
	//P0DIR &= ~(1<<CH4_LDOADC_BIT);           // input

	adc_val = 0;	//initial adc_val;
	ADCCON3 = (0x00<<6|0x03<<4|0x01<<0);//8'b 00(1.25V) 11(14bit) 0001(AIN1)
	ADCCON1 = 0x73; //8'b 0111 0011

	/* Wait for the conversion to finish */
	while ( !ADCIF );

	adc_val = ADCH;
	adc_val = adc_val << 8; //get adc high byte
	adc_val |= ADCL; //get adc low byte
	adc_val = adc_val >> 2;

	/* close the channel */
	//ADCCFG &= ~(1<<CH4_LDOADC_BIT);    //set P0_1 as GPIO
	//P0DIR |= (1<<CH4_LDOADC_BIT);           // output
	//P0INP |= (1<<CH4_LDOADC_BIT);		// tristate


	if(adc_val & (0x01<<13))
	{
		adc_val = 0;
	}

	return adc_val;

}

/**********************************************************************
* Description:
set refence density value of calibration, which should be done before call ch4_calibration
* Return value:
0 : success
others : fail
**********************************************************************/
int8 ch4_set_cal_value(uint16 cal_value)
{
	if (cal_value > 400) {		// cal_value should be [0, 400], stands for 0%~4.00%
		return -1;
	}

	yrCh4.cal_value = cal_value;
	//yrCh4.cal_value = yrCh4.cal_value  << CH4_VAL_SHIFT;

	return 0;
}

/**********************************************************************
* Description:
set precsion threshold of calibration, if beyond this threshold, calibration will fail
* Return value:
0 : success
others: fail
**********************************************************************/
/*
int8 ch4_set_cal_prec(uint32 cal_prec)
{
yrCh4.cal_prec = cal_prec;

return 0;
}
*/

/**********************************************************************
**********************************************************************/
bool	 ch4_getnum_fromdensity(uint8* pNum100, uint8* pNum10,uint8* pNum1,uint16 density)
{
	*pNum100= density/100;
	density -= (*pNum100)*100;
	*pNum100 = (*pNum100)%10;

	*pNum10 = density/10;
	density -= (*pNum10)*10;

	*pNum1 = density;
	return true;
}

void ch4_getdensity_fromnum(uint16* pDensity,uint8 num100, uint8 num10,uint8 num1)
{
	*pDensity = num100*100 + num10*10+ num1;
}

bool  ch4_getstr_fromdensity (char*str, uint8 pNum100, uint8 pNum10,uint8 pNum1)
{
	if(str ==NULL)
	{
		return false;
	}
	char *p = str;
	*p++ = pNum100+'0';
	*p++ = '.';		
	*p++ = pNum10+'0';
	*p++ = pNum1+'0';
	*p++ = '%';
	*p++ = ' ';
	*p++ = 'c';
	*p++ = 'h';
	*p++ = '4';
	*p++ = '\0';
	return true;
}

void ch4_LdoCtrl(bool flag)
{
	if(flag)
	{
		P1 |= (0x01<<CH4_ENLDO_BIT);
	}
	else
	{
		P1 &=~(0x01<<CH4_ENLDO_BIT);
	}
}

bool ch4_GetLdoStatus(void)
{
	if (P1 & (0x01<<CH4_ENLDO_BIT))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ch4_set_ch4ctl(TYRCH4 ctl)
{
	yrCh4 = ctl;
	return;
}
TYRCH4 ch4_get_ch4ctl(void)
{
	return yrCh4;
}

uint8  update_flash(void)
{
	set_info_t info;
	uint8 flag1,flag2;
	
	flag1 = osal_nv_read(GASMONITOR_NV_SETTINGS, 0, sizeof(info), &info);
	if(flag1 == ZSuccess)
	{
		info.ch4_ctl = yrCh4;
		flag2 = osal_nv_write(GASMONITOR_NV_SETTINGS, 0, sizeof(info), &info);
		if(flag2 == ZSuccess)
		{
			return ZSuccess;
		}
	}
	return ZFailure;
}
