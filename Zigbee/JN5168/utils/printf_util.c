/*****************************************************************************
 *
 * MODULE:             Printf Function
 *
 * COMPONENT:          Printf.c
 *
 * AUTHOR:             LJM
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
 * $Id: printf_util.c,v 1.8 2011/04/13 20:37:50 chenxun Exp $
 *
 ****************************************************************************
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
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdarg.h>
#include <ctype.h>
#include <printf_util.h>

#include <AppHardwareApi.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void vNum2String(uint32 u32Number, uint8 u8Base);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE uint8 u8Port_G;
PRIVATE uint8 u8BaudRate_G;

/* pointer to whatever putchar function the user gives us */
PRIVATE void (*vPutChar) (char c) = NULL;

PRIVATE void vUART_Init(uint8 u8Port,uint8 u8BaudRate,uint8 *pau8Buffer, uint32 u32Length);


PRIVATE tsQueue		asUart_RxQueue[1];



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
PUBLIC void vInitPrintf(void (*fp)(char c))
{
    vPutChar = fp;
}


/*
 * printf()
 *  Print to display - really trivial impelementation!
 */
PUBLIC void PrintfUtil_vPrintf(const char *fmt, ...)
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

PUBLIC void PrintfUtil_vPrintMem(uint8 *p, uint16 len)
{
    uint32 i;
    for(i=0;i<len;i++)
    {
        PrintfUtil_vPrintf("%X ", *(p+i));
    }
    PrintfUtil_vPrintf("\n");
}

/****************************************************************************/
/***  Functions To Allow Printf To Work Via The UART                      ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vUART_printInit
 *
 * DESCRIPTION:
 * Initialises the UART print environment
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PUBLIC uint8 PrintfUtil_u8Init(uint8 u8Port, uint8 u8BaudRate,uint8 *pau8Buffer, uint32 u32Length)
{
    if(u8Port != E_AHI_UART_0 && u8Port != E_AHI_UART_1)
        return 1;
    if(u8BaudRate != PRINTF_BAUD_RATE_9600 && u8BaudRate != PRINTF_BAUD_RATE_115200 && u8BaudRate != PRINTF_BAUD_RATE_1M)
        return 1;

    u8Port_G = u8Port;
    u8BaudRate_G = u8BaudRate;
    vInitPrintf((void*)vPutC);
    vUART_Init(u8Port, u8BaudRate,pau8Buffer,u32Length);

    return 0;
}

PUBLIC void vQueue_Init(tsQueue *psQueue, uint8 *pau8Buffer, uint32 u32Length)
{

	/* Initialise the event queue */
	psQueue->u32ReadPtr = 0;
	psQueue->u32WritePtr = 0;
	psQueue->u32Length = u32Length;
	psQueue->pau8Buffer = pau8Buffer;

}

PUBLIC bool bQueue_Write(tsQueue *psQueue, uint8 u8Item)
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

PUBLIC bool bQueue_Read(tsQueue *psQueue, uint8 *pu8Item)
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



PUBLIC uint8 u8UartRead(uint8 u8Uart)
{

	uint8 u8Data;

	//while(!bQueue_Read(&asUart_RxQueue[u8Uart], &u8Data));
	if(bQueue_Read(&asUart_RxQueue[u8Uart], &u8Data))
	    return(u8Data);
    else
        return 0;

}


PRIVATE void vUartRxIsr(uint8 u8Uart)
{

	bQueue_Write(&asUart_RxQueue[u8Uart], u8AHI_UartReadData(u8Uart));

}


PRIVATE void vUartISR(uint32 u32DeviceId, uint32 u32ItemBitmap)
{

	uint8 u8Uart;


	switch(u32DeviceId){

	case E_AHI_DEVICE_UART0:
		u8Uart = 0;
		break;
	#if UART_NUM_UARTS == 2
	case E_AHI_DEVICE_UART1:
		u8Uart = 1;
		break;
	#endif
	default:
		return;
	}


	switch(u32ItemBitmap){


	case E_AHI_UART_INT_RXDATA:
		vUartRxIsr(u8Uart);
		break;

	}

}



/****************************************************************************
 *
 * NAME: vUART_Init
 *
 * DESCRIPTION:
 * Initialises the UART
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PUBLIC void vUART_Init(uint8 u8Port, uint8 u8BaudRate,uint8 *pu8RxBuffer, uint32 u32RxBufferLen)
{
	vQueue_Init(&asUart_RxQueue[0], pu8RxBuffer, u32RxBufferLen);

	vAHI_UartEnable(u8Port);
    vAHI_UartReset(u8Port, TRUE, TRUE);
    vAHI_UartReset(u8Port, FALSE, FALSE);

    vAHI_UartSetControl(u8Port, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, FALSE);

    switch(u8BaudRate){
        case PRINTF_BAUD_RATE_115200:
            vAHI_UartSetClockDivisor(u8Port, E_AHI_UART_RATE_115200);
            break;
        case PRINTF_BAUD_RATE_9600:
            vAHI_UartSetClockDivisor(u8Port, E_AHI_UART_RATE_9600);
            break;
        case PRINTF_BAUD_RATE_1M:
            vAHI_UartSetBaudDivisor(u8Port, 1); // 1000000/1
            break;
        default:
            vAHI_UartSetClockDivisor(u8Port, E_AHI_UART_RATE_115200);
            break;
    }

    vAHI_UartSetControl(u8Port, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, FALSE);

	vAHI_Uart0RegisterCallback((void*)vUartISR);


	/* Enable TX Fifo empty and Rx data interrupts */
		vAHI_UartSetInterrupt(u8Port, FALSE, FALSE, TRUE, TRUE, E_AHI_UART_FIFO_LEVEL_1);
}

PUBLIC void DebugUtil_vInit(uint8 u8Port, PR_HWINT_APPCALLBACK pUart0Callback)
{
    if(pUart0Callback)
    {
        vAHI_Uart0RegisterCallback(pUart0Callback);
        vAHI_UartSetInterrupt(u8Port, FALSE, FALSE, FALSE, TRUE, E_AHI_UART_FIFO_LEVEL_1);
    }
}

/****************************************************************************
 *
 * NAME: vPutC
 *
 * DESCRIPTION:
 * UART Callback Function To Allow printf to work over the Serial Port
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PUBLIC void vPutC(unsigned char c)
{
    //wait for tx fifo empty (bit 5 set in LSR when empty)
    while ((u8AHI_UartReadLineStatus(u8Port_G) & E_AHI_UART_LS_THRE ) == 0);
    // ..and send the character
    vAHI_UartWriteData(u8Port_G,c);

}


PUBLIC char acGetC(void)
{

    return(u8UartRead(0));
}



/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/*
 * vNum2String()
 *  Convert a number to a string
 */
PRIVATE void vNum2String(uint32 u32Number, uint8 u8Base)
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


