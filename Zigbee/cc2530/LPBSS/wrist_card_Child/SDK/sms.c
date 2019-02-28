/*******************************************************************************
  Filename:     sms.c
  Revised:        $Date: 17:17 2014年3月18日
  Revision:       $Revision: 1.0 $
  Description:  

*******************************************************************************/


#include "sms.h"
//#include "hal_timer.h"

#define CUR_SMS_NUM_FLASH   1      

uint8 curIndex = 0;//最新短信序号
uint8 smsIndex = 0;//要显示前第几条短信(从curIndex起)
uint8 smsSum = 0;  // 已存短信总数
uint8 sDisSmsIndex = 0;
uint16 unReadSMSbits = 0;
uint8 Is_chars_In_buffs(const uint8 *pdata,uint8 count);


void PreviousSMS(void)
{
	smsIndex++;	 
}

void NextSMS(void)
{
	uint8 hSumlIndex = 0;
	
	app_FLASH_Read(SMS_INFO_PG,0, &hSumlIndex, 1);
	
	smsSum = hSumlIndex>>4;

	if(smsIndex <= 0)
	{
		smsIndex = smsSum;	
	}
	smsIndex--;
}

bool Is_SMS_Unread(void)
{
	if(unReadSMSbits)
	{
		return true; 
	}
	else
	{
		return false;
	}
}

void ResetSMSIndex(void)
{
	 smsIndex = 0;
	 sDisSmsIndex = 0;
	 
}


bool SetSMS(uint8 *pdata,uint16 u16len)
{	
    bool rs = false;
	uint8 hSumlIndex = 0;

	if(u16len > SMS_MAX_LEN)
	{
		return rs;
	}

	app_FLASH_Read(SMS_INFO_PG,0, &hSumlIndex, 1);

	curIndex = hSumlIndex&0x0f;
	smsSum = hSumlIndex>>4;

	if(curIndex >= SMS_NUMBER_MAX)
	{
		curIndex = 0;
	}

	if(smsSum < SMS_NUMBER_MAX)
	{
		smsSum++;
	}
	else if(smsSum == 0x0f)
	{
		smsSum = 1;
	}
	else
	{
		smsSum = SMS_NUMBER_MAX;
	}
	
    rs = app_FLASH_Write(SMS_INFO_PG,SMS_MAX_LEN * curIndex+CUR_SMS_NUM_FLASH, pdata, u16len);
	++curIndex;

	unReadSMSbits |= 0x01<<(curIndex - 1)|0x01<<(curIndex - 1 + 8);

	hSumlIndex = (smsSum<<4)|(curIndex%0x0f);
	app_FLASH_Write(SMS_INFO_PG,0, &hSumlIndex, 1);
	
    return rs;
}

bool GetSMS(uint8 *pdata,uint8 idx)
{	
    bool rs = false;

	if(idx <= 0)
	{
		return rs;
	}

    rs = app_FLASH_Read(SMS_INFO_PG,SMS_MAX_LEN * (idx - 1)+CUR_SMS_NUM_FLASH, pdata, SMS_MAX_LEN);
    return rs;
}

void Menu_UpdateSMS(void)
{
	#define DIS_LINE_LEN       16
	uint8 len;	
	uint8 Lineidx = 0;
	uint8 SMSBuff[SMS_MAX_LEN+10] = {0};
	uint8 disBuff[DIS_LINE_LEN+2] = {0}; 	
	uint8 charsCnt = 0;
	uint8 offset = 1;//;一个BYTE为长度
	uint8 disSmsIndex = 0;
	uint8 betterDis = 0;
    Time_t time;
	Date_t date;
	uint8 hSumlIndex = 0;
	
	app_FLASH_Read(SMS_INFO_PG,0, &hSumlIndex, 1);
	
	smsSum = hSumlIndex>>4;
	curIndex = hSumlIndex&0x0f;
	smsIndex = smsIndex%smsSum;
	
	if(curIndex != 0x0F && smsSum >=curIndex)
	{		
		disSmsIndex = curIndex > smsIndex?(curIndex-smsIndex):(curIndex-smsIndex+smsSum);

		if(!disSmsIndex)
		{
			disSmsIndex = 1;
		}

		if(sDisSmsIndex == disSmsIndex)
		{
			return;
		}
		
		sDisSmsIndex = disSmsIndex;

		if(unReadSMSbits&(0x01<<(disSmsIndex - 1)))
		{
			unReadSMSbits &= ~(0x01<<(disSmsIndex - 1));
		}
		else if(unReadSMSbits&(0x01<<(disSmsIndex - 1 + 8)))
		{
			unReadSMSbits &= ~(0x01<<(disSmsIndex - 1 + 8));
		}

		HalLcd_HW_Clear();
	
		GetSMS(SMSBuff,disSmsIndex);	
		time = *((Time_t *)SMSBuff);
		date = *((Date_t *)(SMSBuff+sizeof(Time_t)));

		Display_SMS_Time(time,date,smsIndex+1);
	
		len = *(SMSBuff+SMS_DIS_HEADER_LEN);
		offset += SMS_DIS_HEADER_LEN;

		if(len > DIS_LINE_LEN*3)
		{
			return;
		}

		Lineidx = 1;
		//; 8pages ==> 4 lines				
 		for(; Lineidx < 4; Lineidx++)
		{	
			if(len < offset -SMS_DIS_HEADER_LEN)
			{
				break;
			}
			memset(disBuff,0,DIS_LINE_LEN);

			if(len - (offset -SMS_DIS_HEADER_LEN -1) <= DIS_LINE_LEN)
			{
				memcpy(disBuff,SMSBuff+offset,len - (offset -SMS_DIS_HEADER_LEN -1));

				if(len < DIS_LINE_LEN)
				{
					betterDis =(DIS_LINE_LEN - len)<<2;  //; (X/2)*8
				}
				offset += DIS_LINE_LEN;
			}
			else
			{
	 			charsCnt = Is_chars_In_buffs(SMSBuff+offset,DIS_LINE_LEN);

				if(charsCnt%2)//;奇数个char
				{				
					memcpy(disBuff,SMSBuff+offset,DIS_LINE_LEN - 1);
					offset += DIS_LINE_LEN - 1;
				}
				else
				{				
					memcpy(disBuff,SMSBuff+offset,DIS_LINE_LEN);
					offset += DIS_LINE_LEN;
				}
			}
 			
			HalLcd_HW_Page_GB(2*Lineidx,betterDis,0,CHAR_15X16_GB2312,disBuff);
		}			
	}
	else
	{
		//HalLcd_HW_Page_GB(3,24,4,CHAR_15X16_GB2312,"没有短信");	
		Menu_UpdateNoSMS();
	}
}

uint8 Is_chars_In_buffs(const uint8 *pdata,uint8 count)
{
	uint8 rtn = 0;
	
	for(uint8 i = 0; i < count; i++)
	{
		if(*(pdata+i) <= 0x7f)
		{
			rtn += 1;
		}
	}
	return rtn;
}



