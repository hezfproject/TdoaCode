/**************************************************************************************************
  Filename:       StringUtil.c
  Revised:        $Date: 2011/04/21 23:22:11 $
  Revision:       $Revision: 1.1 $

  Description:    String utils. 
  **************************************************************************************************/
/*********************************************************************
 * INCLUDES
 */
#include "StringUtil.h"
#include "string.h"
#include "Ctype.h"
#include "stdio.h"
#include "Hal_defs.h"

int16 atol(const char *str)
{
	int16 ret = 0;
	int8 sign = 1;
	
	/* skip whitespace */
	//while ( isspace((int)(unsigned char)*str) )
	//++str;
	
	if (*str == '-')
		sign = -1; 
	else
		ret = *str - '0'; 
	
	++str;
	while (*str <= '9' && *str >= '0') 
	{
		ret = ret * 10 + (*str - '0');
		++str;
	}

	return (sign*ret);
}

uint16 atoul(const char *str)
{
	uint16 ret = 0;
	
	ret = *str - '0'; 
	
	++str;
	while (*str <= '9' && *str >= '0') 
	{
		ret = ret * 10 + (*str - '0');
		++str;
	}

	return (ret);
}

/**************************************************************************************************
 * @fn      Util_itoa
 *
 * @brief   convert a 16bit number to ASCII
 *
 * @param   num -
 *          buf -
 *          radix -
 *
 * @return  void
 *
 **************************************************************************************************/

void Util_itoa(uint16 num, char *buf, uint8 radix)
{
  char c,i;
  char *p, rst[5];

  p = rst;
  for ( i=0; i<5; i++,p++ )
  {
    c = num % radix;  // Isolate a digit
    *p = c + (( c < 10 ) ? '0' : '7');  // Convert to Ascii
    num /= radix;
    if ( !num )
      break;
  }

  for ( c=0 ; c<=i; c++ )
    *buf++ = *p--;  // Reverse character order

  *buf = '\0';
}

/**************************************************************************
 *  
 *  NAME : StrUtil_tirm
 *
 *  DESCRIPTION:
 *  remove '\n', '\r' and space at the head and tail of a string
 *
**************************************************************************/

char *StrUtil_trim(char *str)
{
	char *phead = str;
	char *ptail;
	if(phead)
	{
		ptail = phead + strlen(str) - 1;

		//remove head
		while(*phead && isspace(*phead)) 
			phead++;

		//remove tail and set to '\0'
		while(ptail > phead && isspace(*ptail)) 
			*(ptail--) = '\0';
	}
	return phead;
}

/**************************************************************************
 *  
 *  NAME : u8ToHexStr
 *
 *  DESCRIPTION:
 *  translate the data to a Hex string with seperator.
 *
 *  PARAMETERS:
 *  hlen is the length of hexdata
 *  elen is the length of an element; for example, 16-bit short address can be considered as 2 uint8 data
 *  slen is the lenght of seperator. no seperator after the last element
 *
**************************************************************************/

uint16 u16DataToHexStr(uint8 *hexdata, uint8 hlen, uint8 elen, char* buf, uint16 buflen, char* separator, uint8 slen)
{
	uint8 i, j;
	uint8 t;
	uint16 count=0;
	uint8 temp;
	uint8 gnum = 0;

	for(i=0;i<hlen;i++)
	{
		if((buflen - count) < (slen+4)) break;
		
		temp=hexdata[i];
		/* 1 byte = 2 char*/
		for(j=0;j<2;j++)
		{
			t = (temp & 0xF0) >> 4;
			temp = temp << 4;

			if(t<0xA) buf[count] = '0' + t;
			else       buf[count] = 'A' + (t-0xA);
			
			count++;
		}
		gnum++;

		/*add seperator between elements*/
		if(gnum == elen && i != hlen -1)
		{
			for(j=0;j<slen;j++)
			{
				buf[count++] = separator[j];
			}
			gnum = 0;
		}
	}

	return count;
	
}

/**************************************************************************
 *  
 *  NAME : u8ToHexStr
 *
 *  return:  length of the translated uint8 data
 *		   -1: error
 *
**************************************************************************/

int16 HexStrToU8Data(uint8 * pDest, char* pSrc)
{
        uint8 i;
	uint8 srcLen = strlen(pSrc);
	if(srcLen%2 != 0)
	{
		return -1;
	}
	for(i=0;i<srcLen;i++)
	{
		if(!isxdigit(*(pSrc+i)))
		{
			return -1;
		}
	}
	for(i=0; i<srcLen/2; i++)
	{
		char c;

		c = *(pSrc+2*i);
		
		pDest[i] = 0;
		if(c < 'A') pDest[i] |= (c - '0') << 4;
		else pDest[i] |= (c - 'A') << 4;

		c = *(pSrc+2*i+1);
		if(c < 'A') pDest[i] |= (c - '0');		
		else pDest[i] |= (*pSrc - 'A');				
	}
	*(pDest+i) = 0;
	return srcLen/2;
}
