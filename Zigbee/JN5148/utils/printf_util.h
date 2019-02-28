/*****************************************************************************
 *
 * MODULE:      Utilities
 *
 * COMPONENT:   Printf.h
 *
 * AUTHOR:      LJM
 *
 * DESCRIPTION: Printf header file
 *
 * $HeadURL: http://svn/sware/Projects/Jeneric/Modules/Utilities/Tags/JENERIC_UTILITIES_1v1_RC2/Include/Printf.h $
 *
 * $Revision: 1.5 $
 *
 * $LastChangedBy: jahme $
 *
 * $LastChangedDate: 2009-06-12 14:23:59 +0100 (Fri, 12 Jun 2009) $
 *
 * $Id: printf_util.h,v 1.5 2011/04/13 20:37:50 chenxun Exp $
 *
 *****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd. 2009 All rights reserved
 *
 ****************************************************************************/

#ifndef __PRINTF_UTIL_H__
#define __PRINTF_UTIL_H__

#if defined __cplusplus
extern "C" { 
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include <AppHardwareApi.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define PRINTF_BAUD_RATE_9600     0
#define PRINTF_BAUD_RATE_115200 1
#define PRINTF_BAUD_RATE_1M         2

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC uint8 PrintfUtil_u8Init(uint8 u8Port, uint8 u8BaudRate);
PUBLIC void vInitPrintf(void (*fp)(char c));
PUBLIC void PrintfUtil_vPrintf(const char *fmt, ...);
PUBLIC void PrintfUtil_vPrintMem(uint8 *p, uint16 len);

PUBLIC void vPutC(unsigned char c);
PUBLIC void DebugUtil_vInit(uint8 u8Port, PR_HWINT_APPCALLBACK pUart0Callback);

#if defined __cplusplus
}
#endif

#endif /* PRINTF_H_INCLUDED */


