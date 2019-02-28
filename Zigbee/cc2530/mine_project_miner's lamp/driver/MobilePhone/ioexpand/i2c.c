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

static void I2C_Start(void);
static void I2C_Write(char dat);
static char I2C_Read(void);
static unsigned char I2C_GetAck(void);
static void I2C_PutAck(unsigned char ack);
static void I2C_Stop(void);

/*
������I2C_Init()
���ܣ�I2C���߳�ʼ����ʹ���ߴ��ڿ���״̬
˵������main()�����Ŀ�ʼ����ͨ��Ӧ��Ҫִ��һ�α�����
*/
void I2C_Init()
{

    P2SEL &= ~BV(1);     // P2_3 used as gpio
    P0SEL &= ~BV(6);     // P0_6 used as gpio

    I2C_SCL_SETOUT;
    I2C_SDA_SETOUT;

    I2C_SCL = 1;
    I2C_Delay();
    I2C_SDA = 1;
    I2C_Delay();
}

/*
������I2C_Start()
���ܣ�����I2C���ߵ���ʼ״̬
˵����
SCL���ڸߵ�ƽ�ڼ䣬��SDA�����½���ʱ����I2C����
����SDA��SCL����ʲô��ƽ״̬��������������ȷ������ʼ״̬
������Ҳ�������������ظ���ʼ״̬
������ִ�к�I2C���ߴ���æ״̬
*/
static void I2C_Start()
{
    I2C_SDA = 1;
    I2C_Delay();
    I2C_SCL = 1;
    I2C_Delay();
    I2C_SDA = 0;
    I2C_Delay();
    I2C_SCL = 0;
    I2C_Delay();
}

/*
������I2C_Write()
���ܣ���I2C����д1���ֽڵ�����
������
dat��Ҫд�������ϵ�����
*/
static void I2C_Write(char dat)
{
    unsigned char t = 8;
    do
    {
        I2C_SDA = (dat & 0x80) > 0 ? 1 : 0;
        dat <<= 1;
        I2C_Delay();
        I2C_SCL = 1;
        I2C_Delay();
        I2C_SCL = 0;
        I2C_Delay();
    }
    while(--t != 0);
}

/*
������I2C_Read()
���ܣ��Ӵӻ���ȡ1���ֽڵ�����
���أ���ȡ��һ���ֽ�����
*/
static char I2C_Read()
{
    char dat;
    unsigned char t = 8;
    I2C_SDA = 1; //�ڶ�ȡ����֮ǰ��Ҫ��SDA����
    I2C_Delay();
    I2C_Delay();

    // change gpio direction
    I2C_SDA_SETIN;

    I2C_Delay();

    do
    {
        I2C_SCL = 1;
        I2C_Delay();
        dat <<= 1;
        if(I2C_SDA) dat |= 0x01;
        I2C_SCL = 0;
        I2C_Delay();
    }
    while(--t != 0);

    // change back
    I2C_SDA_SETOUT;

    return dat;
}

/*
������I2C_GetAck()
���ܣ���ȡ�ӻ�Ӧ��λ
���أ�
0���ӻ�Ӧ��
1���ӻ���Ӧ��
˵����
�ӻ����յ�ÿ���ֽڵ����ݺ�Ҫ����Ӧ��λ
�ӻ����յ����1���ֽڵ����ݺ�һ��Ҫ������Ӧ��λ
*/
static unsigned char I2C_GetAck()
{
    unsigned char ack;
    I2C_SDA = 1;
    I2C_Delay();
    I2C_Delay();
    // change gpio direction
    I2C_SDA_SETIN;
    I2C_Delay();

    I2C_SCL = 1;
    I2C_Delay();
    ack = I2C_SDA;
    I2C_SCL = 0;
    // change back
    I2C_SDA_SETOUT;

    I2C_Delay();

    return ack;
}

/*
������I2C_PutAck()
���ܣ���������Ӧ��λ���Ӧ��λ
������
ack=0����������Ӧ��λ
ack=1������������Ӧ��λ
˵����
�����ڽ�����ÿһ���ֽڵ����ݺ󣬶�Ӧ������Ӧ��λ
�����ڽ��������һ���ֽڵ����ݺ�Ӧ��������Ӧ��λ
*/
static void I2C_PutAck(unsigned char ack)
{
    I2C_SDA = ack & 1;
    I2C_Delay();
    I2C_SCL = 1;
    I2C_Delay();
    I2C_SCL = 0;
    I2C_Delay();
}

/*
������I2C_Stop()
���ܣ�����I2C���ߵ�ֹͣ״̬
˵����
SCL���ڸߵ�ƽ�ڼ䣬��SDA����������ʱֹͣI2C����
����SDA��SCL����ʲô��ƽ״̬��������������ȷ����ֹͣ״̬
������ִ�к�I2C���ߴ��ڿ���״̬
*/
static void I2C_Stop()
{
    unsigned int t = I2C_STOP_WAIT_VALUE;
    I2C_SDA = 0;
    I2C_Delay();
    I2C_SCL = 1;
    I2C_Delay();
    I2C_SDA = 1;
    I2C_Delay();
    while(--t != 0);      //����һ�β���Start֮ǰ��Ҫ��һ������ʱ
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
    //������ʱ����
    unsigned char i;
    char a[4];
    //��鳤��
    if(Size == 0) return 0;
    //׼���ӻ���ַ
    a[0] = (SlaveAddr << 1);
    //����ӵ�ַģʽ
    if(SubMod > 2) SubMod = 2;

    //ȷ���ӵ�ַ
    switch(SubMod)
    {
    case 0:
        break;
    case 1:
        a[1] = (char)(*pSubAddr);
        break;
    case 2:
        a[1] = (char)(*pSubAddr);
        a[2] = (char)(*(pSubAddr + 1));
        break;
    default:
        break;
    }

    //���ʹӻ���ַ�����ŷ����ӵ�ַ��������ӵ�ַ�Ļ���
    SubMod++;
    I2C_Start();
    for(i = 0; i < SubMod; i++)
    {
        I2C_Write(a[i]);
        if(I2C_GetAck())
        {
            I2C_Stop();
            return 1;
        }
    }
    //��������
    do
    {
        I2C_Write(*dat++);
        if(I2C_GetAck()) break;
    }
    while(--Size != 0);
    //������ϣ�ֹͣI2C���ߣ������ؽ��
    I2C_Stop();
    if(Size == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
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
    //������ʱ����
    unsigned char i;
    char a[4];
    //��鳤��
    if(Size == 0) return 0;
    //׼���ӻ���ַ
    a[0] = (SlaveAddr << 1);
    //����ӵ�ַģʽ
    if(SubMod > 2) SubMod = 2;
    //��������ӵ�ַ�Ĵӻ�����Ҫ�ȷ��ʹӻ���ַ���ӵ�ַ
    //ȷ���ӵ�ַ

    //ȷ���ӵ�ַ
    if(SubMod == 1)
    {
        a[1] = (char)(*pSubAddr);
    }
    else
    {
        a[1] = (char)(*pSubAddr);
        a[2] = (char)(*(pSubAddr + 1));
    }

    //���ʹӻ���ַ�����ŷ����ӵ�ַ
    SubMod++;
    I2C_Start();
    for(i = 0; i < SubMod; i++)
    {
        I2C_Write(a[i]);
        if(I2C_GetAck())
        {
            I2C_Stop();
            return 1;
        }
    }
    //�����I2C_Start()�������ӵ�ַ�Ĵӻ����ظ���ʼ״̬
    //�������ӵ�ַ�Ĵӻ�������������ʼ״̬
    I2C_Start();

    //���ʹӻ���ַ
    I2C_Write(a[0] + 1);
    if(I2C_GetAck())
    {
        I2C_Stop();
        return 1;
    }

    //��������
    for(;;)
    {
        *dat++ = I2C_Read();
        if(--Size == 0)
        {
            I2C_PutAck(1);
            break;
        }
        I2C_PutAck(0);
    }
    //������ϣ�ֹͣI2C���ߣ������ؽ��
    I2C_Stop();
    return 0;
}


/*
������I2C_Close()
���ܣ��ر�I2C����
*/
void I2C_Close()
{

    P2SEL &= ~BV(1);     // P2_3 used as gpio
    P0SEL &= ~BV(6);     // P0_6 used as gpio

    I2C_SCL_SETOUT;
    I2C_SDA_SETOUT;


    I2C_SCL = 1;
    I2C_SDA = 1;
}
