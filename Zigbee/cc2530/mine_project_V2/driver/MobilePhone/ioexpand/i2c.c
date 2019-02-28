/*
I2C.c
标准80C51单片机模拟I2C总线的主机程序
Copyright (c) 2005，广州周立功单片机发展有限公司
All rights reserved.
本程序仅供学习参考，不提供任何可靠性方面的担保；请勿用于商业目的
*/

#include <ioCC2530.h>
#include "i2c.h"
#include "delay.h"

//模拟I2C总线的引脚定义

#define I2C_SCL     (P2_3)
#define I2C_SDA     (P0_6)

#define I2C_SCL_SETIN    (P2DIR &= ~BV(3))
#define I2C_SCL_SETOUT  (P2DIR |= BV(3))

#define I2C_SDA_SETIN    (P0DIR &= ~BV(6))
#define I2C_SDA_SETOUT (P0DIR |= BV(6))


//定义I2C总线停止后在下一次开始之前的等待时间，取值1～65535
//等待时间约为(I2C_STOP_WAIT_VALUE*8)个机器周期
//对于多数器件取值为1即可；但对于某些器件来说，较长的延时是必须的
#define I2C_STOP_WAIT_VALUE 120

/*
宏定义：I2C_Delay()
功能：延时，模拟I2C总线专用
*/
#define I2C_Delay()  st(DelayUs(5);)

static void I2C_Start(void);
static void I2C_Write(char dat);
static char I2C_Read(void);
static unsigned char I2C_GetAck(void);
static void I2C_PutAck(unsigned char ack);
static void I2C_Stop(void);

/*
函数：I2C_Init()
功能：I2C总线初始化，使总线处于空闲状态
说明：在main()函数的开始处，通常应当要执行一次本函数
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
函数：I2C_Start()
功能：产生I2C总线的起始状态
说明：
SCL处于高电平期间，当SDA出现下降沿时启动I2C总线
不论SDA和SCL处于什么电平状态，本函数总能正确产生起始状态
本函数也可以用来产生重复起始状态
本函数执行后，I2C总线处于忙状态
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
函数：I2C_Write()
功能：向I2C总线写1个字节的数据
参数：
dat：要写到总线上的数据
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
函数：I2C_Read()
功能：从从机读取1个字节的数据
返回：读取的一个字节数据
*/
static char I2C_Read()
{
    char dat;
    unsigned char t = 8;
    I2C_SDA = 1; //在读取数据之前，要把SDA拉高
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
函数：I2C_GetAck()
功能：读取从机应答位
返回：
0：从机应答
1：从机非应答
说明：
从机在收到每个字节的数据后，要产生应答位
从机在收到最后1个字节的数据后，一般要产生非应答位
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
函数：I2C_PutAck()
功能：主机产生应答位或非应答位
参数：
ack=0：主机产生应答位
ack=1：主机产生非应答位
说明：
主机在接收完每一个字节的数据后，都应当产生应答位
主机在接收完最后一个字节的数据后，应当产生非应答位
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
函数：I2C_Stop()
功能：产生I2C总线的停止状态
说明：
SCL处于高电平期间，当SDA出现上升沿时停止I2C总线
不论SDA和SCL处于什么电平状态，本函数总能正确产生停止状态
本函数执行后，I2C总线处于空闲状态
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
    while(--t != 0);      //在下一次产生Start之前，要加一定的延时
}

/*
函数：I2C_Puts()
功能：I2C总线综合发送函数，向从机发送多个字节的数据
参数：
SlaveAddr：从机地址（7位纯地址，不含读写位）
SubAddr：从机的子地址
SubMod：子地址模式，0－无子地址，1－单字节子地址，2－双字节子地址
*dat：要发送的数据
Size：数据的字节数
返回：
0：发送成功
1：在发送过程中出现异常
说明：
本函数能够很好地适应所有常见的I2C器件，不论其是否有子地址
当从机没有子地址时，参数SubAddr任意，而SubMod应当为0
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
    //定义临时变量
    unsigned char i;
    char a[4];
    //检查长度
    if(Size == 0) return 0;
    //准备从机地址
    a[0] = (SlaveAddr << 1);
    //检查子地址模式
    if(SubMod > 2) SubMod = 2;

    //确定子地址
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

    //发送从机地址，接着发送子地址（如果有子地址的话）
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
    //发送数据
    do
    {
        I2C_Write(*dat++);
        if(I2C_GetAck()) break;
    }
    while(--Size != 0);
    //发送完毕，停止I2C总线，并返回结果
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
函数：I2C_Gets()
功能：I2C总线综合接收函数，从从机接收多个字节的数据
参数：
SlaveAddr：从机地址（7位纯地址，不含读写位）
SubAddr：从机的子地址
SubMod：子地址模式，0－无子地址，1－单字节子地址，2－双字节子地址
*dat：保存接收到的数据
Size：数据的字节数
返回：
0：接收成功
1：在接收过程中出现异常
说明：
本函数能够很好地适应所有常见的I2C器件，不论其是否有子地址
当从机没有子地址时，参数SubAddr任意，而SubMod应当为0
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
    //定义临时变量
    unsigned char i;
    char a[4];
    //检查长度
    if(Size == 0) return 0;
    //准备从机地址
    a[0] = (SlaveAddr << 1);
    //检查子地址模式
    if(SubMod > 2) SubMod = 2;
    //如果是有子地址的从机，则要先发送从机地址和子地址
    //确定子地址

    //确定子地址
    if(SubMod == 1)
    {
        a[1] = (char)(*pSubAddr);
    }
    else
    {
        a[1] = (char)(*pSubAddr);
        a[2] = (char)(*(pSubAddr + 1));
    }

    //发送从机地址，接着发送子地址
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
    //这里的I2C_Start()对于有子地址的从机是重复起始状态
    //对于无子地址的从机则是正常的起始状态
    I2C_Start();

    //发送从机地址
    I2C_Write(a[0] + 1);
    if(I2C_GetAck())
    {
        I2C_Stop();
        return 1;
    }

    //接收数据
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
    //接收完毕，停止I2C总线，并返回结果
    I2C_Stop();
    return 0;
}


/*
函数：I2C_Close()
功能：关闭I2C总线
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
