#ifndef _TEMPERATURE_H_
#define _TEMPERATURE_H_

#define TEM_CALIBTEMP_MIN (-200) // -20 *C
#define TEM_CALIBTEMP_MAX (600)  //  60 *C
#define TEM_CALIBTEMP_INIT (200) //  20 *C

#define TEM_CALIBVAL_MIN (-8192)
#define TEM_CALIBVAL_MAX (8192)
#define TEM_CALIBVAL_INIT (5300)

typedef struct
{
	int16 temp;
	int16 value;
}tyrTemp_t;

int16 tem_get_temperature(void);
bool tem_calibration(int16 temp);
bool tem_set_tempCtrl(tyrTemp_t tyrTemp);
tyrTemp_t tem_get_tempCtrl(void);

int16 tem_get_adc(void);
bool tem_getnum_fromtemper(char str[], int16 temper);

#endif

