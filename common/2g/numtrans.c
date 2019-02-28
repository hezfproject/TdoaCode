#include "numtrans.h"
#include "app_protocol.h"

#include "string.h"
#include "../CommonTypes.h"
static uint_8  num_bcd2char(char*pc, uint_8 num);
static uint_8  num_char2bcd(uint_8*pnum,char c);

/* change termNbr to string */
/* return the string size */
unsigned int num_term2str(char* s, const app_termNbr_t *p)
{
	char* ss = s;
	unsigned char i;
	
	if(!s || !p)  return 0;
	
	for(i=0; i<APP_NMBRDIGIT;i++)
	{
		char c;
		if(num_bcd2char(&c,p->nbr[i] & 0x0F))
		{
			if(c == 0)
			{	
				break;
			}
			else
			{
				*s++ = c;
			}
		}
		else
		{
			break;
		}
		if(num_bcd2char(&c,(p->nbr[i]>>4) & 0x0F))
		{
			if(c == 0)
			{	
				break;
			}
			else
			{
				*s++ = c;
			}
		}	
		else
		{
			break;
		}
	}

	*s = 0;
	return (unsigned int)(ss - s);
}

/* change termNbr to string */
/* return the string size */
unsigned int  num_str2term(app_termNbr_t *pterm, char* s)
{
	memset((uint_8*)pterm, 0xFF, sizeof(app_termNbr_t));
	char* ss = s;
	unsigned int i=0;
        
	while( i< 2*APP_NMBRDIGIT-1)
	{
		uint_8 num;
		if(num_char2bcd(&num, *(ss++)))
		{
			if(num == 0x0F)
			{
				break;
			}
			else
			{
				unsigned int idx = i/2;
				if(i == 2*idx)
				{
					pterm->nbr[idx] &= 0xF0;
					pterm->nbr[idx] |= (num & 0x0F);
				}
				else
				{
					pterm->nbr[idx] &= 0x0F;
					pterm->nbr[idx] |= (num<<4) & 0xF0;
				}
			}
			i++;
		}
	}	
	return i;
}

unsigned int num_term_getlen(const app_termNbr_t *p)
{
	unsigned int i;
	unsigned int idx;
	
	for(i=0; i<2*APP_NMBRDIGIT-1; i++)
	{
		idx = i/2;
		if(i == 2*idx)
		{
			if((p->nbr[idx] & 0x0F) == 0x0F)
			{
				break;
			}
		}
		else
		{
			if((p->nbr[idx]>>4 & 0x0F) == 0x0F)
			{
				break;
			}
		}
	}
	return i;
}

unsigned char num_isequal(const app_termNbr_t *p1, const app_termNbr_t *p2)
{
	unsigned int i;
    	unsigned int j;
        
        for(i=0; i<APP_NMBRDIGIT; i++)
       {
            for(j=0; j<2; j++)
            {
                uint_8 num1,num2;
                if(j==0)
                {
                    num1 = p1->nbr[i] & 0xFF;
                    num2 = p2->nbr[i] & 0xFF;
                }
                else
                {      
                      num1 = p1->nbr[i]>>4 & 0xFF;
                      num2 = p2->nbr[i]>>4 & 0xFF;
                }

                if(num1 == 0xFF || num2 == 0xFF)  // one end
                {
                    if(num1 == num2)
                    {
                        return 1;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    if(num1 != num2)
                    {
                           return 0;
                    }
                }
            }
       }
        return 0;
}

/*bcd code translation */
uint_8  num_bcd2char(char*pc, uint_8 num)
{
	num &= 0x0F;
	if(num<=0x09)
	{
		*pc = '0'+num;
		return 1;
	}
	else if(num == 0x0A)
	{
		*pc = '+';
		return 1;
	}
	else if(num == 0x0B)
	{
		*pc = '-';
		return 1;
	}
	else if(num == 0x0C)
	{
		*pc = '*';
		return 1;
	}
 	else if(num == 0x0D)
	{
		*pc = '#';
		return 1;
	}   
	else	 if(num == 0x0F)
	{
		*pc = 0;
		return 1;
	}
	
	return 0;
}
uint_8  num_char2bcd(uint_8*pnum,char c)
{
	if(c == 0)
	{
		*pnum = 0x0F;
		return 1;
	}
	else if(c>= '0' && c<= '9')
	{
		*pnum = c-'0';
		return 1;
	}
	else if( c=='+' )
	{
		*pnum = 0x0A;
		return 1;
	}
	else if( c== '-')
	{
		*pnum = 0x0B;
		return 1;
	}
	else if( c== '*')
	{
		*pnum = 0x0C;
		return 1;
	} 
	else if( c== '#')
	{
		*pnum = 0x0D;
		return 1;
	}    
	return 0;
}

