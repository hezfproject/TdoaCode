#include "afe.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "afe_cfg.h"

_LFSTATE LFSTATE;

unsigned char T0_val;
//unsigned char addr_val;
//unsigned char data_val;	//��ʱֵ
unsigned char LF_rec_data_buf[5];
unsigned char LF_rec_data[5];
unsigned char bits = 0x80;	//����ʱ��λ
unsigned char LF_addr_val;	//���������յ���ֵ��ֱ��ȡ��
unsigned char LF_data_val;
unsigned char LF_new_val;	//����ֵ��־λ
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
**		ʱ��ѡ��
**
************************************/
void OSC_INIT(void)
{
	OSCCON = 0x41;	//1M,
//	OSCCON = 0x51;	//2M
//	OSCCON = 0x71;	//8M,
//	OSCCON = 0x11;	//125K,
	while(!HTS);	//�ȶ�  32K����ʹ��
//	OSCCON = 0x01;	//31K,	����Լʮ��uA
//	while(!LTS);	//�ȶ�  32K����ʹ��
}


/***************************************
**
**		�˿ڳ�ʼ�����ص�һЩ�ĵ�Ķ���
**		���������
**
***************************************/

void PORT_INIT(void)
{
	CMCON0 = 0x07;	//�Ƚ�����
	PCON = 0x0;		//���ó���ѹ���Ѻ�BOR
	VRCON = 0x20;	//��Vref
	T1CON = 0x00;
	WDA = 0xFB;		//������ͨ
//	WPUDA = 0xFF;	//FFʹ��������
	WPUDA = 0x00;	//FFʹ��������
	OPTION |= 0x80;		//ͨ������������ʹ��

//	TRISA = 0x07;
	TRISA = 0xFF;
	TRISC = 0x0F;
	PORTA = 0x00;
	PORTC = 0x00;
}


/****************************************
**
**		��ʼ�����빦�ܣ�ռ�� T0��
**
*****************************************/

void INT_AFE_INIT(void)
{
	TRISA |= 0x04;	//RA2����
	PORTA &= 0xFB;
	WDA &= 0xFB;	//RA2����
	WPUDA |= 0xFF;	//RA2����ʹ��

	IOCA = 0x00;		//��RA��ƽ�ж�
	INTEDG = 1;		//������
	INTF = 0;


	LFSTATE = LFstart;
	LF_new_val = 0;
	I2C_SDA_H;
	I2C_SCL_H;

//	OPTION	&= 0xD0;	//T0 16��Ƶ		//  1M�ڲ�ʱ��Fosc /4/16=64us	(T0CS=0)
//	OPTION	|= 0x03;
	OPTION = 0xd3;		//1M�ڲ�ʱ��Fosc /4/16=64us
	TMR0 = 0;
	T0IF = 0;	//
//	T0IE = 1;	//
	INTE = 1;		// ʹ���ⲿ�ж�
	PEIE = 1;			//�������ж�
	GIE = 1;

//T1CON |= BV(3)|BV(1)|BV(2);// Ԥ��Ƶϵ��Ϊ1���ⲿʱ��  ��ͬ���ⲿʱ��
 /*  T1CON = 0x30;
   TMR1IE = 1;
   PEIE =1;
   GIE = 1;
   TMR1H = 0;
   TMR1L = 0;
   TMR1ON = 1;*/

}

/******************************************************************************
 *   ����Ӧ��Ƶ������Ӧ(RA2);
 *   �����ش����жϣ�
 */
void start_LFReceiver(void)
{
    INTE = 1;

}
/******************************************************************************
 *   �ر���Ӧ��Ƶ������Ӧ(RA2);
 *   �����ش����жϣ�
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
**		���ݽ��룬���ж��е���
**		---	��ʾ��ʼ���½��ؼ�����ʼ
**		_- 	��ʾ0	��0.5mS
**		___-	��ʾ1	��1.0mS
**		RA2/INT  ��ƽ�仯�ж�
**		�͵�ƽ��4mS�ͻص�LFidle��
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
    		bits = 0x80;	//��λ
    		while(TMR0<time_hi_mask);
    		break;
		}
		case RCV_11:
        {
    		if(T0_val>time_max || T0_val < 13)	//����̫���̫խ
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
            if(T0_val>time_max || T0_val < 13)  //����̫���̫խ
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
            if(T0_val>time_max || T0_val < 13)  //����̫���̫խ
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
**	��ѯ��������ֵ
**  FL_vol��ֵ
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
**		�жϷ���
**
*****************************************/
void interrupt INT_SERVICE(void)
{
	if(T0IF)	//T0���
	{
		T0IF = 0;
		LFSTATE = LFstart;
        T0IE = 1;

        rec_count++;
	}
	if(INTF)	//INT���ж�
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

