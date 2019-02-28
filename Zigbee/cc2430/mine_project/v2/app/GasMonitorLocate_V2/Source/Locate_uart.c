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
bool    locateIsValid=false;
bool   F_direction=true;  //true:+  false:-   
uint16  locateDistance=0;
uint16  saveRealLocatedistance=0;
uint16  saveLocatedistance=0;
uint16  targetName=0;
uint16  count10min=0;
int8   i8Rssi=NO_RSSI;

uint16 uartWantReadLen =0;
uint8  receive_uart_buff[128];
uint8  radioVersion[16] ={"NO VERSION"};

void LocateUart_init(void);
void GasMonitor_UartSync();
static uint16 locate_fabs(uint16 ,uint16 j);
void GasMonitor_UartWrite(uint8* p, uint8 len, uint8 cmdtype);
void GasMonitor_UartWriteACK(uint8 cmdtype);
uint16 GasMonitor_GetLocateDistance(void);
uint16 GasMonitor_GetLocateTargetName(void);
int8  GasMonitor_Get_Network_Rssi(void);

#ifndef  NONWK    
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
#endif
void GasMonitor_UartWrite(uint8* p, uint8 len, uint8 cmdtype)
{
    uint16 headerCRC=0;
    uint8  offset=0;
    offset = sizeof(unsigned short)+2*sizeof(unsigned char);
        
    Uart_Header_t  uart_header;
    uart_header.header_h=UART_PREAMBLE_H;
    uart_header.header_l=UART_PREAMBLE_L;
    uart_header.padding=0;
    uart_header.cmdtype=cmdtype;
    uart_header.len=len;
    headerCRC=CRC16((uint8 *)&uart_header+offset,sizeof(Uart_Header_t)-offset,0xffff);
    uart_header.crc=CRC16( p,len,headerCRC);
    HalUARTWrite(HAL_UART_PORT_0, (uint8 *)&uart_header, sizeof(Uart_Header_t));
    HalUARTWrite(HAL_UART_PORT_0, (uint8 *)p, len);
}

void GasMonitor_UartWriteACK(uint8 cmdtype)
{
    uint8  offset=0;
    offset = sizeof(unsigned short)+2*sizeof(unsigned char);
        
    Uart_Header_t  uart_header;
    uart_header.header_h=UART_PREAMBLE_H;
    uart_header.header_l=UART_PREAMBLE_L;
    uart_header.padding=0;
    uart_header.cmdtype=cmdtype;
    uart_header.len=0;
    uart_header.crc=CRC16((uint8 *)&uart_header+offset,sizeof(Uart_Header_t)-offset,0xffff);
    HalUARTWrite(HAL_UART_PORT_0, (uint8 *)&uart_header, sizeof(Uart_Header_t));
}

static uint16 locate_fabs(uint16 i,uint16 j)
{
    if(i>j)return i-j;

    else return j-i;
}
uint16 GasMonitor_Get_SAVE_LocateDistance(void)
{
	return saveRealLocatedistance;
}

int8 GasMonitor_Get_Network_Rssi(void)
{
	return i8Rssi;
}


uint16 GasMonitor_GetLocateDistance(void)
{
#define THE_MAX_MODIFY       7
#define THE_MIN_MODIFY       1

    static uint8  firstToThird;
    static bool  save_F_direction=true;
    
    if(firstToThird++<3)
        {
                saveLocatedistance=locateDistance;
                return locateDistance;
        }
    
    if(locateDistance>saveLocatedistance)
    {
        F_direction=true;
    }
    else if(locateDistance<saveLocatedistance)
    {
        F_direction=false;
    }      	
    if(locate_fabs(locateDistance,saveLocatedistance)>THE_MAX_MODIFY)
    	{
    	    int8 n=0; 
    	    if(F_direction)
    	    	{    	    	
    	    		if(save_F_direction)
    	    		{
			n=THE_MAX_MODIFY;
			}
			else	 
			{
			n=THE_MIN_MODIFY;
			}
    	    	}
		else
		{
    	    		if(save_F_direction)
    	    		{
			    n=-THE_MIN_MODIFY;
			}
			else	
			{
			    n=-THE_MAX_MODIFY;
			}
		}
		locateDistance=saveLocatedistance+n;		
    	}
    save_F_direction=F_direction;	
    saveLocatedistance=locateDistance;
    return locateDistance;
}
uint16  GasMonitor_GetLocateTargetName(void)
{
    return targetName;
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


