/**************************************************************************************************
  Filename:       StringUtil.h
  Revised:        $Date: 2010/06/05 00:37:57 $
  Revision:       $Revision: 1.5 $
 **************************************************************************************************/
/*********************************************************************
string related operation.
*********************************************************************/
#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include "hal_types.h"
#include <string.h>

int16 atol(uint8 *str);
uint16 atoul(uint8 *str);
char *StrUtil_trim(char *str);
uint16 u16DataToHexStr(uint8 *hexdata, uint8 hlen, uint8 elen, char* buf, uint16 buflen, char* separator, uint8 slen);
int16 HexStrToU8Data(uint8 * pDest, char* pSrc);
#endif

