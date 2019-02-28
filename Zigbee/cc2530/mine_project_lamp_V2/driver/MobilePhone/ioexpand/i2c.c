/*
I2C.c
��׼80C51��Ƭ��ģ��I2C���ߵ���������
Copyright (c) 2005��������������Ƭ����չ���޹�˾
All rights reserved.
���������ѧϰ�ο������ṩ�κοɿ��Է���ĵ���������������ҵĿ��
*/

#include <ioCC2530.h>
#include "i2c.h"
#include "delay.h"

//ģ��I2C���ߵ����Ŷ���

#define I2C_SCL     (P2_3)
#define I2C_SDA     (P0_6)

#define I2C_SCL_SETIN    (P2DIR &= ~BV(3))
#define I2C_SCL_SETOUT  (P2DIR |= BV(3))

#define I2C_SDA_SETIN    (P0DIR &= ~BV(6))
#define I2C_SDA_SETOUT (P0DIR |= BV(6))


//����I2C����ֹͣ������һ�ο�ʼ֮ǰ�ĵȴ�ʱ�䣬ȡֵ1��65535
//�ȴ�ʱ��ԼΪ(I2C_STOP_WAIT_VALUE*8)����������
//���ڶ�������ȡֵΪ1���ɣ�������ĳЩ������˵���ϳ�����ʱ�Ǳ����
#define I2C_STOP_WAIT_VALUE 120

/*
�궨�壺I2C_Delay()
���ܣ���ʱ��ģ��I2C����ר��
*/
#define I2C_Delay()  st(DelayUs(5);)


/*
������I2C_Init()
���ܣ�I2C���߳�ʼ����ʹ���ߴ��ڿ���״̬
˵������main()�����Ŀ�ʼ����ͨ��Ӧ��Ҫִ��һ�α�����
*/
void I2C_Init()
{

}



/*
������I2C_Puts()
���ܣ�I2C�����ۺϷ��ͺ�������ӻ����Ͷ���ֽڵ�����
������
SlaveAddr���ӻ���ַ��7λ����ַ��������дλ��
SubAddr���ӻ����ӵ�ַ
SubMod���ӵ�ַģʽ��0�����ӵ�ַ��1�����ֽ��ӵ�ַ��2��˫�ֽ��ӵ�ַ
*dat��Ҫ���͵�����
Size�����ݵ��ֽ���
���أ�
0�����ͳɹ�
1���ڷ��͹����г����쳣
˵����
�������ܹ��ܺõ���Ӧ���г�����I2C�������������Ƿ����ӵ�ַ
���ӻ�û���ӵ�ַʱ������SubAddr���⣬��SubModӦ��Ϊ0
*/
unsigned char I2C_Puts
(
    unsigned char SlaveAddr,
    unsigned char *pSubAddr,
    unsigned char SubMod,
    unsigned char *dat,
    unsigned int Size
)
{
}

/*
������I2C_Gets()
���ܣ�I2C�����ۺϽ��պ������Ӵӻ����ն���ֽڵ�����
������
SlaveAddr���ӻ���ַ��7λ����ַ��������дλ��
SubAddr���ӻ����ӵ�ַ
SubMod���ӵ�ַģʽ��0�����ӵ�ַ��1�����ֽ��ӵ�ַ��2��˫�ֽ��ӵ�ַ
*dat��������յ�������
Size�����ݵ��ֽ���
���أ�
0�����ճɹ�
1���ڽ��չ����г����쳣
˵����
�������ܹ��ܺõ���Ӧ���г�����I2C�������������Ƿ����ӵ�ַ
���ӻ�û���ӵ�ַʱ������SubAddr���⣬��SubModӦ��Ϊ0
*/
unsigned char I2C_Gets
(
    unsigned char SlaveAddr,
    unsigned char *pSubAddr,
    unsigned char SubMod,
    unsigned char *dat,
    unsigned int Size
)
{

    return 0;
}


/*
������I2C_Close()
���ܣ��ر�I2C����
*/
void I2C_Close()
{

}
