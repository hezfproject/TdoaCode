/*****************************************************************************
 *
 * MODULE:             Printf Function
 *
 * COMPONENT:          Printf.c
 *
 * AUTHOR:             LJB
 *
 * DESCRIPTION:        Code to provide a simple printf function
 *
 * $HeadURL:           http://svn/sware/Projects/Jeneric/Modules/Utilities/Trunk/Source/Printf.c $
 *
 * $Revision: 1.8 $
 *
 * $LastChangedBy: $
 *
 * $LastChangedDate: $
 *
 * $Id: printf_util.c,v 1.8 2016/08/11 20:37:50 linjianbin Exp $
 *
 ****************************************************************************
 *
 * This software is owned by stm32 and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on yiri products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". YIRI MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Yiri Ltd. 2016 All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdarg.h>
#include <type_def.h>
#include <printf_util.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

void vNum2String(uint32 u32Number, uint8 u8Base);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
uint8 u8Port_G;
int8 u8BaudRate_G;

/* pointer to whatever putchar function the user gives us */
void (*vPutChar) (char c) = NULL;

void vUART_Init(uint8 u8Port,uint8 u8BaudRate,uint8 *pau8Buffer, uint32 u32Length);


tsQueue		asUart_RxQueue[1];



/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/*
* Before using these functions, you must call vInitPrintf
* and give it a pointer to a function that will write the individual
* characters to whatever device you decide to use for displaying output.
*
* example below :-
*
*   #include "Printf.h"
*
*   void main(void)
*   {
*
*       vInitPrintf((void*)vPutC);  // pass the pointer to the putc function
*
*       vPrintf("\nHello World!");  // show some text !
*       while(1);
*   }
*
*   void vPutC(char c)
*   {
*       do something with c here, maybe write it to a uart
*   }
*
*/


void PrintfUtil_u8Init(void)
{
    vInitPrintf((void*)vPutC);
}

void vInitPrintf(void (*fp)(char c))
{
    vPutChar = fp;
}

/*
 * printf()
 *  Print to display - really trivial impelementation!
 */
void PrintfUtil_vPrintf(const char *fmt, ...)
{
    char *bp = (char *)fmt;
    va_list ap;
    char c;
    char *p;
    int32 i;

    va_start(ap, fmt);

    while ((c = *bp++)) {
        if (c != '%') {
            if (c == '\n'){
                vPutChar('\n');
                vPutChar('\r');
            } else {
                vPutChar(c);
            }
            continue;
        }

        switch ((c = *bp++)) {

        /* %d - show a decimal value */
        case 'd':
            vNum2String(va_arg(ap, uint32), 10);
            break;

        /* %x - show a value in hex */
        case 'x':
            vPutChar('0');
            vPutChar('x');
            vNum2String(va_arg(ap, uint32), 16);
            break;

        case 'X':
            vNum2String(va_arg(ap, uint32), 16);
            break;

        /* %b - show a value in binary */
        case 'b':
            vPutChar('0');
            vPutChar('b');
            vNum2String(va_arg(ap, uint32), 2);
            break;

        /* %c - show a character */
        case 'c':
            vPutChar(va_arg(ap, int));
            break;

        case 'i':
            i = va_arg(ap, int32);
            if(i < 0){
                vPutChar('-');
                vNum2String((~i)+1, 10);
            } else {
                vNum2String(i, 10);
            }
            break;

        /* %s - show a string */
        case 's':
            p = va_arg(ap, char *);
            do {
                vPutChar(*p++);
            } while (*p);
            break;

        /* %% - show a % character */
        case '%':
            vPutChar('%');
            break;

        /* %something else not handled ! */
        default:
            vPutChar('?');
            break;

        }
    }

    return;
}

void PrintfUtil_vPrintMem(uint8 *p, uint16 len)
{
    uint32 i;
    for(i=0;i<len;i++)
    {
        PrintfUtil_vPrintf("%X ", *(p+i));
    }
    PrintfUtil_vPrintf("\n");
}

#if 0


bool bQueue_Write(tsQueue *psQueue, uint8 u8Item)
{

	/* Make a copy of the write pointer */
	uint32 u32NewWritePtr = psQueue->u32WritePtr;

	u32NewWritePtr++;
	if(u32NewWritePtr == psQueue->u32Length)
	{
		u32NewWritePtr = 0;
	}

	/* If new incremented pointer is same as read pointer, queue is full */
	if(u32NewWritePtr == psQueue->u32ReadPtr)
	{
		return(FALSE);
	}

	psQueue->pau8Buffer[psQueue->u32WritePtr] = u8Item;	/* Add item to queue */
	psQueue->u32WritePtr = u32NewWritePtr;				/* Write new pointer */

	return(TRUE);
}

bool bQueue_Read(tsQueue *psQueue, uint8 *pu8Item)
{

	/* If pointers are same, nothing in the queue */
	if(psQueue->u32ReadPtr == psQueue->u32WritePtr)
	{
		return(FALSE);
	}

	/* Read an event from the queue */
	*pu8Item = psQueue->pau8Buffer[psQueue->u32ReadPtr++];

	if(psQueue->u32ReadPtr == psQueue->u32Length)
	{
		psQueue->u32ReadPtr = 0;
	}

	return(TRUE);

}
#endif


uint8 vPutC(unsigned char ch)
{
    //wait for tx fifo empty (bit 5 set in LSR when empty)
    //while ((u8AHI_UartReadLineStatus(u8Port_G) & E_AHI_UART_LS_THRE ) == 0);
    // ..and send the character
   // vAHI_UartWriteData(u8Port_G,c);

   USART_SendData(USART2, (unsigned char) ch);	/* Loop until the end of transmission */
   while (USART_GetFlagStatus(USART2, USART_FLAG_TC) != SET);

   return ch;
    //while((USART2->SR&0X40)==0);//循环发送,直到发送完毕
    //USART2->DR = (unsigned char) ch;
}




/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/*
 * vNum2String()
 *  Convert a number to a string
 */
void vNum2String(uint32 u32Number, uint8 u8Base)
{
    char buf[33];
    char *p = buf + 33;
    uint32 c, n;

    *--p = '\0';
    do {
        n = u32Number / u8Base;
        c = u32Number - (n * u8Base);
        if (c < 10) {
            *--p = '0' + c;
        } else {
            *--p = 'a' + (c - 10);
        }
        u32Number /= u8Base;
    } while (u32Number != 0);

    while (*p){
        vPutChar(*p);
        p++;
    }

    return;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/


