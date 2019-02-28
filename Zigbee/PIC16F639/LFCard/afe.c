#include "afe.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "afe_cfg.h"

_LFSTATE LFSTATE;

unsigned char T0_val;
//unsigned char addr_val;
//unsigned char data_val;	//临时值
unsigned char LF_rec_data_buf[5];
unsigned char LF_rec_data[5];
unsigned char bits = 0x80;	//接收时移位
unsigned char LF_addr_val;	//接收完存放收到的值，直到取出
unsigned char LF_data_val;
unsigned char LF_new_val;	//有新值标志位
unsigned int rec_count = 0;
unsigned int sys_count = 0;



void delay(unsigned int c)
{
	unsigned int a,b;
	for(a=c;a>0;a--)
		for(b=110;b>0;b--);
}


void delay_200us(unsigned char c)
{
	unsigned char a,b;
	for(a=c;a>0;a--)
		for(b=1;b>0;b--);
}

/*********************************
**
**		时钟选择
**
************************************/
void OSC_INIT(void)
{
	OSCCON = 0x41;	//1M,
//	OSCCON = 0x51;	//2M
//	OSCCON = 0x71;	//8M,
//	OSCCON = 0x11;	//125K,
	while(!HTS);	//稳定  32K以上使用
//	OSCCON = 0x01;	//31K,	运行约十几uA
//	while(!LTS);	//稳定  32K以上使用
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
	WDA = 0xFB;		//上拉连通
//	WPUDA = 0xFF;	//FF使能上下拉
	WPUDA = 0x00;	//FF使能上下拉
	OPTION |= 0x80;		//通过单独的上拉使能

//	TRISA = 0x07;
	TRISA = 0xFF;
	TRISC = 0x0F;
	PORTA = 0x00;
	PORTC = 0x00;
}


/****************************************
**
**		初始化解码功能，占用 T0，
**
*****************************************/

void INT_AFE_INIT(void)
{
	TRISA |= 0x04;	//RA2输入
	PORTA &= 0xFB;
	WDA &= 0xFB;	//RA2下拉
	WPUDA |= 0xFF;	//RA2下拉使能

	IOCA = 0x00;		//关RA电平中断
	INTEDG = 1;		//上升沿
	INTF = 0;


	LFSTATE = LFstart;
	LF_new_val = 0;
	I2C_SDA_H;
	I2C_SCL_H;

//	OPTION	&= 0xD0;	//T0 16分频		//  1M内部时钟Fosc /4/16=64us	(T0CS=0)
//	OPTION	|= 0x03;
	OPTION = 0xd3;		//1M内部时钟Fosc /4/16=64us
	TMR0 = 0;
	T0IF = 0;	//
//	T0IE = 1;	//
	INTE = 1;		// 使能外部中断
	PEIE = 1;			//开外设中断
	GIE = 1;

//T1CON |= BV(3)|BV(1)|BV(2);// 预分频系数为1，外部时钟  不同步外部时钟
 /*  T1CON = 0x30;
   TMR1IE = 1;
   PEIE =1;
   GIE = 1;
   TMR1H = 0;
   TMR1L = 0;
   TMR1ON = 1;*/

}

/******************************************************************************
 *   打开响应低频输入响应(RA2);
 *   上升沿触发中断；
 */
void start_LFReceiver(void)
{
    INTE = 1;

}
/******************************************************************************
 *   关闭响应低频输入响应(RA2);
 *   上升沿触发中断；
 */
void stop_LFReceiver(void)
{
    INTE = 0;
}

void IIC_Init(void)
{
    I2C_SCL_L;
    I2C_SDA_L;
}

void Send_LFDate(unsigned char u8Byte)
{
    unsigned char i;
    //u8Byte = 0x67;


    I2C_SCL_H;
    delay_200us(100);      //wakeup 5168
    for(i=0;i<8;i++)
    {
        if((u8Byte << i) & 0x80)
        {
            I2C_SDA_H;
        }
        else
        {
            I2C_SDA_L;
        }
        I2C_SCL_L;
        delay_200us(6);
        I2C_SCL_H;
        delay_200us(6);
    }
    I2C_SCL_L;
    //delay_200us(10);
}


/**********************************************
**
**		数据解码，在中断中调用
**		---	表示开始，下降沿计数开始
**		_- 	表示0	共0.5mS
**		___-	表示1	共1.0mS
**		RA2/INT  电平变化中断
**		低电平超4mS就回到LFidle。
***********************************************/
void AFE_RECEIVER(void)
{
	stop_LFReceiver();//INTE = 0;
	T0_val = TMR0;
	T0IE = 0;
	TMR0 = 0;
	switch(LFSTATE)
	{
		case LFstart:
        {
            LF_rec_data_buf[0] = 0;
            LF_rec_data_buf[1] = 0;

            T0IE = 1;
            LF_new_val = 0;
    		LFSTATE = RCV_11;
    		bits = 0x80;	//移位
    		while(TMR0<time_hi_mask);
    		break;
		}
		case RCV_11:
        {
    		if(T0_val>time_max || T0_val < 13)	//数据太宽或太窄
    		{
    			LFSTATE = LFstart;
    			break;
    		}
    		T0IE = 1;
    		if(T0_val>time_mid)
    		{
    			LF_rec_data_buf[0] |= bits;
    		}
            else
            {
                LF_rec_data_buf[0] &= (~bits);
            }
    		bits = bits>>1;
    		if(!bits)
    		{
                LFSTATE = RCV_22;
    			bits = 0x80;
    			//LF_new_val = 1;
    			//T0IE = 0;
    		}
    		while(TMR0<time_hi_mask);
    		break;
        }
        case RCV_22:
        {
            if(T0_val>time_max || T0_val < 13)  //数据太宽或太窄
            {
                LFSTATE = LFstart;
                break;
            }
            T0IE = 1;
            if(T0_val>time_mid)
            {
                LF_rec_data_buf[1] |= bits;
            }
            else
            {
                LF_rec_data_buf[1] &= (~bits);
            }
            bits = bits>>1;
            if(!bits)
            {

                LFSTATE = RCV_33;
                bits = 0x80;
            }
            while(TMR0<time_hi_mask);
            break;
        }

        case RCV_33:
        {
            if(T0_val>time_max || T0_val < 13)  //数据太宽或太窄
            {
                LFSTATE = LFstart;
                break;
            }
            T0IE = 1;
            if(T0_val>time_mid)
            {
                LF_rec_data_buf[2] |= bits;
            }
            else
            {
                LF_rec_data_buf[2] &= (~bits);
            }
            bits = bits>>1;
            if(!bits)
            {
                if((LF_rec_data_buf[0] == LF_rec_data_buf[1]) && (LF_rec_data_buf[0] == LF_rec_data_buf[2]) && (LF_rec_data_buf[0] != 0))
                {
                    LF_new_val = 1;
                    LF_rec_data[0] = LF_rec_data_buf[0];
                }
                else
                {
                    LF_rec_data_buf[0] = 0;
                    LF_rec_data_buf[1] = 0;
                    LF_rec_data_buf[2] = 0;
                    LF_new_val = 0;
                }
                LFSTATE = LFstart;
                bits = 0x80;

                T0IE = 0;
            }
            while(TMR0<time_hi_mask);
            break;
        }

		default:
       		LFSTATE = LFstart;
          	break;
		}

    if((LF_rec_data[1] != LF_rec_data[0]) && (LF_new_val == 1))
        AFE_NEW_data();

    if(LF_new_val == 0)
	    start_LFReceiver();//INTE = 1;

}

/*********************************************
**
**	查询有无新数值
**  FL_vol是值
**
*******************************************/
void AFE_NEW_data(void)
{
    if(LF_new_val==1)
	{
        stop_LFReceiver();//INTE = 0;

        LF_new_val = 0;

		//memcpy(LF_rec_data,LF_rec_data_buf,sizeof(LF_rec_data_buf));
		//memset(LF_rec_data_buf,0,sizeof(LF_rec_data_buf));
		if(LF_rec_data[0] != 0)
		{
            Send_LFDate(LF_rec_data[0]);

			//memset(LF_rec_data,0,sizeof(LF_rec_data));
		}
        LF_rec_data[1] = LF_rec_data[0];
        LF_rec_data[0] = 0;
        start_LFReceiver();//INTE = 1;
        T0IE = 1;
        return;
	}

    start_LFReceiver();//INTE = 1;
}

/****************************************
**
**		中断服务
**
*****************************************/
void interrupt INT_SERVICE(void)
{
	if(T0IF)	//T0溢出
	{
		T0IF = 0;
		LFSTATE = LFstart;
        T0IE = 1;

        rec_count++;
	}
	if(INTF)	//INT脚中断
	{
        rec_count = 0;
        AFE_RECEIVER();
		INTF = 0;
	}
    /*else if(TMR1IF)
    {
        TMR1IF = 0;
        sys_count++;
        I2C_SDA_H;
        I2C_SCL_H;
        delay(200);

        //delay_200us(10);  // 1100us
        I2C_SDA_L;
        I2C_SCL_L;
        delay(200);      //1s dealy
    }*/
}

