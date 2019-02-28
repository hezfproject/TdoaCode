#ifndef __CH4_H__
#define __CH4_H__
// data type define
#include "hal_types.h"
/*
#ifndef uint8
#define uint8   unsigned char
#endif
#ifndef uint16
#define uint16  unsigned short
#endif
#ifndef uint32
#define uint32  unsigned int
#endif
#ifndef int8
#define int8    signed char
#endif
#ifndef int16
#define int16   short
#endif
#ifndef int32
#define int32   int
#endif
*/

#define GAS_MAX_DENSITY 		400
#define GAS_INVALID_DENSITY      10001

#define SLOP_A_MIN	700
#define SLOP_A_MAX   5000
#define SLOP_A_INIT  1724

#define ZERO_VAL_MIN  0
#define ZERO_VAL_MAX 2048
#define ZERO_VAL_INIT 512


#define CAL_VALUE_MIN 	(50)
#define CAL_VALUE_MAX 	(400)
#define CAL_VALUE_INIT 	(100)

typedef struct _TYRCH4 {
	uint32	slope_a;			// slope value for ch4 density curve
	uint32	zero_val;		// adc value of zero point
	uint32	cal_value;		// refence value for calibration
	//	uint32 	cal_step;		// step when doing calibration
	//	uint32	cal_prec;		// precision threshold of calibraion
}TYRCH4, *PYRCH4;


// initial ch4 driver
int8 ch4_init(void);

// get ch4 density, denisty is between 0and 400, 0 is for 0%, 400 is for 4.00%
uint16 ch4_get_density(void);

// do zero adjust
int8 ch4_zero_adjust(void);

// do calibration,  return 0 for success, others for fail this time
int8 ch4_calibration(void);

// set reference value of calibration, cal_value is between 0 and 400, i.e., 0% ~ 4.00%
int8 ch4_set_cal_value(uint16 cal_value); 

void ch4_set_ch4ctl(TYRCH4 ctl);

TYRCH4 ch4_get_ch4ctl(void);

void ch4_LdoCtrl(bool flag);

bool ch4_GetLdoStatus(void);

uint16 ch4_get_adc_value(void);

bool ch4_getnum_fromdensity(uint8* pNum100, uint8* pNum10,uint8* pNum1,uint16 density);

void ch4_getdensity_fromnum(uint16* pDensity,uint8 num100, uint8 num10,uint8 num1);

bool ch4_getstr_fromdensity (char*str, uint8 pNum100, uint8 pNum10,uint8 pNum1);


#endif	// __CH4_H__
