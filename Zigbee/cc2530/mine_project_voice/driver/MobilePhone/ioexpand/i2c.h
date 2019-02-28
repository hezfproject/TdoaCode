/*
I2C.h
标准80C51单片机模拟I2C总线的主机程序头文件
Copyright (c) 2005，广州周立功单片机发展有限公司
All rights reserved.
本程序仅供学习参考，不提供任何可靠性方面的担保；请勿用于商业目的
*/

#ifndef _I2C_H_
#define _I2C_H_  


//I2C总线初始化，使总线处于空闲状态
void I2C_Init(void);

//I2C总线综合发送函数，向从机发送多个字节的数据
unsigned char I2C_Puts(unsigned char SlaveAddr,unsigned char *pSubAddr,unsigned char SubMod,unsigned char *dat,unsigned int Size);

//I2C总线综合接收函数，从从机接收多个字节的数据
unsigned char I2C_Gets(unsigned char SlaveAddr,unsigned char *pSubAddr,unsigned char SubMod,unsigned char *dat,unsigned int Size);

void I2C_Close(void);

#endif //_I2C_H_
