/**************************************************************************************************
  Filename:       StringUtil.h
  Revised:        $Date: 2010/11/26 01:08:48 $
  Revision:       $Revision: 1.2 $
 **************************************************************************************************/
/*********************************************************************
string related operation.
*********************************************************************/
#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include "hal_types.h"
#include <string.h>

int16 atol(const char *str);
uint16 atoul(const char *str);
void Util_itoa(uint16 num, char *buf, uint8 radix);

char *StrUtil_trim(char *str);
uint16 u16DataToHexStr(uint8 *hexdata, uint8 hlen, uint8 elen, char* buf, uint16 buflen, char* separator, uint8 slen);
int16 HexStrToU8Data(uint8 * pDest, char* pSrc);
#endif

