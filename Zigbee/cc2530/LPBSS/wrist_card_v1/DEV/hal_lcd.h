/**************************************************************************************************
  Filename:       hal_lcd.h
  Revised:        $Date: 2011/04/01 22:32:59 $
  Revision:       $Revision: 1.1 $

  Description:    This file contains the interface to the LCD Service.


  Copyright 2005-2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

#ifndef HAL_LCD_H
#define HAL_LCD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>

/**************************************************************************************************
 *                                          INCLUDES
 **************************************************************************************************/
//#include "hal_board.h"

/**************************************************************************************************
 *                                         CONSTANTS
 **************************************************************************************************/

/* These are used to specify which line the text will be printed */
#define HAL_LCD_LINE_1      0x01
#define HAL_LCD_LINE_2      0x02
/*
   This to support LCD with extended number of lines (more than 2).
   Don't use these if LCD doesn't support more than 2 lines
*/
#define HAL_LCD_LINE_3      0x03
#define HAL_LCD_LINE_4      0x04
#define HAL_LCD_LINE_5      0x05
#define HAL_LCD_LINE_6      0x06
#define HAL_LCD_LINE_7      0x07
#define HAL_LCD_LINE_8      0x08

/* Max number of chars on a single LCD line */
#define HAL_LCD_MAX_CHARS   16
#define HAL_LCD_MAX_BUFF    25


typedef enum
{
     CHAR_5X7_ASCII   = 1,
     CHAR_7X8_ASCII,
     CHAR_6X12_ASCII,
     CHAR_8X16_ASCII,
     CHAR_12_ASCII_ARIAL,
     CHAR_16_ASCII_ARIAL,
     CHAR_11X12_GB2312,
     CHAR_15X16_GB2312,
     CHAR_6X12_GB2312,
     CHAR_8X16_GBEXPAND,
}charType;

extern void Menu_UpdadeBattery(UINT8 BatteryLevel);
extern void Menu_UpdateNoSMS(void);
extern bool IsBatteryCharge();



/**************************************************************************************************
 *                                          MACROS
 **************************************************************************************************/


/**************************************************************************************************
 *                                         TYPEDEFS
 **************************************************************************************************/


/**************************************************************************************************
 *                                     GLOBAL VARIABLES
 **************************************************************************************************/


/**************************************************************************************************
 *                                     FUNCTIONS - API
 **************************************************************************************************/

/*
 * Initialize LCD Service
 */
extern void HalLcdInit(void);
extern void HalLcdTurnOn(void);
extern void HalLcdTurnOff(void);

void  HalLcd_BigChar_StartX(uint8 line_x,uint8 page, uint8 model,const uint8 CODE * pData, uint16 len);
UINT8 HalLcd_HW_Page_GB(uint8 page,uint8 line_x,uint8 offset,charType charType,const uint8 *pdata);
uint8 HalLcd_Page_Char(UINT8 page,UINT8 line_x,charType charType,const UINT16 pdata);
void  HalLcd_HW_Clear(void);

void Menu_UpdateTime(uint8 flash);
void Menu_UpdateID(uint16 id);
void Menu_UpdateDate(void);
void Menu_UpdateTumble(void);
void Menu_UpdateSOS(void);
void Menu_UpdateWeather(void);
void Menu_UpdateWeek(void);
void Menu_UpdateBandState(uint8 flash,uint8 status);
void Menu_UpdateEnvelope(uint8 flash);




/**************************************************************************************************
**************************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
