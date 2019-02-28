/*******************************************************************************
  Filename:     sms.c
  Revised:        $Date: 17:17 2014Äê3ÔÂ18ÈÕ
  Revision:       $Revision: 1.0 $
  Description:  

*******************************************************************************/

#ifndef _SMS_H_
#define _SMS_H_

#include <bsp_flash.h>
#include <hal_mcu.h>
#include "app_flash.h"
#include "hal_lcd.h"

#define  SMS_MAX_LEN     SMS_INFO_OFFSET

void PreviousSMS(void);
void NextSMS(void);
bool Is_SMS_Unread(void);
void ResetSMSIndex(void);
bool SetSMS(uint8 *pdata,uint16 u16len);
bool GetSMS(uint8 *pdata,uint8 idx);
void Menu_UpdateSMS(void);

#endif//_SMS_H_



