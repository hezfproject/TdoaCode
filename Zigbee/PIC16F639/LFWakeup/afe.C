#include "afe.h"



_LFSTATE LFSTATE;



void delay(unsigned int c)
{
	unsigned int a,b;
	for(a=c;a>0;a--)
		for(b=110;b>0;b--);
}


/*********************************
**
**		时钟选择
**
************************************/
void OSC_INIT(void)
{
	//OSCCON = 0x41;	//1M,
//	OSCCON = 0x71;	//8M,
//	OSCCON = 0x11;	//125K,
//	while(!HTS);	//稳定  32K以上使用
	//OSCCON = 0x00;	//31K,	运行约十几uA
	//while(!LTS);	//稳定  32K以上使用
	//while(!OSTS);
	OSCCON = 0x40;	//1M,	
	while(!HTS);
}


/***************************************
**
**		端口初始化，关掉一些耗电的东西	
**		关最低休眠
**	
***************************************/

void PORT_INIT(void)
{
	CMCON0 = 0x07;	//比较器关
	PCON = 0x0;		//禁用超低压唤醒和BOR
	VRCON = 0x20;	//关Vref
	T1CON = 0x00;
	WDA = 0xFB;		//RA2下拉; 
	WPUDA = 0x00;	//禁止上下拉

		
	TRISA = 0x04;	//RA2-in
	//RA1,RA5 out
	TRISA &= ~0x22;	//RA5-out
	TRISC = 0x0C;		//RC3-in	RC2-in
	TRISC &= 0x01;		//RC0-out
	PORTA = 0xFF;
	PORTC = 0xFF;
}
	

/****************************************
**
**		初始化解码功能，占用 T1，
**
*****************************************/
void INT_AFE_INIT(void)
{	

	INTEDG = 1;		//上升沿
	INTE = 1;		// INT中断开

	LFSTATE = LFidle;

	GIE	= 1;		//
}




/**********************************************
**
**		数据解码，在中断中调用
**		250uS高250uS低表示0
**		250uS高500uS低表示1
**		RA2/INT  上升沿有效
**
***********************************************/

void AFE_RECEIVER(void)
{

	switch(LFSTATE)
 		{
   			case LFidle:	//定时器开始计脉宽
				

				LFSTATE = LFstart;
				break;
				
			case LFstart:	//确定有上个脉宽和开始计下一个

				LFSTATE = LFaddr;
				break;

			case LFaddr:

				LFSTATE = LFdata;
				break;
				
			case LFdata:	//...........解析数据对应到命令

				LFSTATE = LFidle;
				break;
				
			default:
           		LFSTATE = LFidle;
              break;
		}


}






