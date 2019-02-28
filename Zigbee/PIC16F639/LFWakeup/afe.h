#ifndef  __AFE_H__
#define  __AFE_H__
#include <htc.h>
#include "spi.h"

#define OPENLED	{RC0 = 0;}
#define CLOSELED {RC0 = 1;}

typedef enum{
				LFidle,		//��
				LFstart,	//��ʼ
				LFaddr,		//��ʶ
				LFdata		//����
}_LFSTATE;

typedef enum{
				Upload,
				Download,
				Verify
}_LFCMD;



extern void OSC_INIT(void);
extern void PORT_INIT(void);
extern void delay(unsigned int c);
extern void INT_AFE_INIT(void);



#endif //__AFE_H__




