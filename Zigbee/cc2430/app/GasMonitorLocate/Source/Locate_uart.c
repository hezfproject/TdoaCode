/**************************************************************************************************
Filename:       Locate_Uart.c
Revised:        $Date: 2011/08/06 02:01:59 $
Revision:       $Revision: 1.8 $

**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "Locate_Uart.h"
#include "Hal_uart.h"
#include "OnBoard.h"
#include "delay.h"
#include "mac_pib.h"

//#include "ChargeED.h"
/*********************************************************************/

bool    Locate_isvalid=false;
bool   F_direction=true;  //true:+  false:-   
uint16  LocateDistance=0;
uint16  save_real_locatedistance=0;
uint16  save_locatedistance=0;
uint16  Target_Name=0;

uint16 uartWantReadLen;
uint8  receive_uart_buff[128];
bool sync_flag=false;

void LocateUart_init(void);
void GasMonitor_UartSync();
static uint16 locate_fabs(uint16 ,uint16 j);
void GasMonitor_UartWrite(uint8* p, uint8 len, uint8 cmdtype);
uint16 GasMonitor_GetLocateDistance(void);
uint16 GasMonitor_GetLocateTargetName(void);
uint8 Locate_Uart_BuildandSendDataPAN(uint8 *p, uint8 len, uint16 DstAddr, uint16 DstPan);

//uint8 MAC_UTIL_BuildandSendData(uint8 *p, uint8 len, uint8 DeliverMode, uint16 uniDstAddr, uint8 txOption)

uint8 Locate_Uart_BuildandSendDataPAN(uint8 *p, uint8 len, uint16 DstAddr, uint16 DstPan)
{

    uint16 lenth;
    if((p == NULL)||!len)
    {
        return ZFailure;
    }

    uart_wireless_t uart_wireless;
    uart_wireless.hdr.header_h=UART_PREAMBLE_H;
    uart_wireless.hdr.header_l=UART_PREAMBLE_L;
    uart_wireless.hdr.cmdtype=WIRELESS;
    uart_wireless.hdr.len=len+sizeof(uart_wireless_t);
    uart_wireless.hdr.checksum=WIRELESS+len+sizeof(uart_wireless_t);
    uart_wireless.hdr.padding=0;

    uart_wireless.DstAddr=DstAddr;
    uart_wireless.DstPan=DstPan;
    uart_wireless.SrcAddr=macPib.shortAddress;
    uart_wireless.SrcPan=macPib.panId;
    lenth=HalUARTWrite(HAL_UART_PORT_0, (uint8 *)&uart_wireless, sizeof(uart_wireless_t));
    if(lenth==sizeof(uart_wireless_t))
    {
        lenth=HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, len);
        if(lenth==len)return ZSuccess;
    }

    return ZFailure;


}

void GasMonitor_UartWrite(uint8* p, uint8 len, uint8 cmdtype)
{

    Uart_Header_t  uart_header;
    uart_header.header_h=UART_PREAMBLE_H;
    uart_header.header_l=UART_PREAMBLE_L;
    uart_header.padding=0;
    uart_header.cmdtype=cmdtype;
    uart_header.len=len;
    uart_header.checksum=uart_header.len+cmdtype;
    HalUARTWrite(HAL_UART_PORT_0, (uint8 *)&uart_header, sizeof(Uart_Header_t));
    HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, len);
}
static uint16 locate_fabs(uint16 i,uint16 j)
{
    if(i>j)return i-j;

    else return j-i;
}
uint16 GasMonitor_Get_SAVE_LocateDistance(void)
{
	return save_real_locatedistance;
}
uint16 GasMonitor_GetLocateDistance(void)
{
    static bool  save_F_direction=true;
    //save_real_locatedistance=LocateDistance;	
    if(LocateDistance>save_locatedistance)
    {
        F_direction=true;
    }
    else if(LocateDistance<save_locatedistance)
    {
        F_direction=false;
    }      	
    //if((!locate_direction)||(locate_direction==0xFFFF))locate_direction=0x7FFF;
    if(locate_fabs(LocateDistance,save_locatedistance)>7)
    	{
    	    int8 n=0; 
    	    if(F_direction)
    	    	{    	    	
    	    		if(save_F_direction)
    	    		{
			n=7;
			}
			else	 
			{
			n=1;
			}
    	    	}
		else
		{
    	    		if(save_F_direction)
    	    		{
			n=-1;
			}
			else	
			{
			n=-7;
			}
		}
		LocateDistance=save_locatedistance+n;		
    	}
    save_F_direction=F_direction;	
    save_locatedistance=LocateDistance;
   // if(LocateDistance>2)
    //{
      //  LocateDistance=LocateDistance-2;
    //}
    //else
    //{
      //  LocateDistance=0;
   // }
    return LocateDistance;
}
uint16  GasMonitor_GetLocateTargetName(void)
{
    return Target_Name;
}

void	 GasMonitor_stationID_fromdensity(uint8* pNum10000,uint8* pNum1000,uint8* pNum100, uint8* pNum10,uint8* pNum1,uint16 density)
{
    *pNum10000= density/10000;
    density -= (*pNum10000)*10000;
    *pNum10000 = (*pNum10000)%10;

    *pNum1000= density/1000;
    density -= (*pNum1000)*1000;

    *pNum100= density/100;
    density -= (*pNum100)*100;

    *pNum10 = density/10;
    density -= (*pNum10)*10;

    *pNum1 = density;
}

//#define UART_STATE_IDLE 	0
//#define UART_STATE_SYNC	 1
//#define UART_STATE_RECV 	2


