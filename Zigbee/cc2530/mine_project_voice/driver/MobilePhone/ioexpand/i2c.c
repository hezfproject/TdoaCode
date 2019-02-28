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


/*
函数：I2C_Init()
功能：I2C总线初始化，使总线处于空闲状态
说明：在main()函数的开始处，通常应当要执行一次本函数
*/
void I2C_Init()
{

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

    return 0;
}


/*
函数：I2C_Close()
功能：关闭I2C总线
*/
void I2C_Close()
{

}
