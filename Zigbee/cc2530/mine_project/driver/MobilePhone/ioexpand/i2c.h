/*
I2C.h
��׼80C51��Ƭ��ģ��I2C���ߵ���������ͷ�ļ�
Copyright (c) 2005��������������Ƭ����չ���޹�˾
All rights reserved.
���������ѧϰ�ο������ṩ�κοɿ��Է���ĵ���������������ҵĿ��
*/

#ifndef _I2C_H_
#define _I2C_H_  


//I2C���߳�ʼ����ʹ���ߴ��ڿ���״̬
void I2C_Init(void);

//I2C�����ۺϷ��ͺ�������ӻ����Ͷ���ֽڵ�����
unsigned char I2C_Puts(unsigned char SlaveAddr,unsigned char *pSubAddr,unsigned char SubMod,unsigned char *dat,unsigned int Size);

//I2C�����ۺϽ��պ������Ӵӻ����ն���ֽڵ�����
unsigned char I2C_Gets(unsigned char SlaveAddr,unsigned char *pSubAddr,unsigned char SubMod,unsigned char *dat,unsigned int Size);

void I2C_Close(void);

#endif //_I2C_H_
