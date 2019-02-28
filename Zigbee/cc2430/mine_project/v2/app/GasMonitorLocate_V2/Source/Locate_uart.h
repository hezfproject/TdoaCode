/**************************************************************************************************
Filename:       Locate_uart.h
Revised:        $Date: 2011/07/22 18:49:55 $
Revision:       $Revision: 1.5 $

**************************************************************************************************/
#ifndef _LOCATE_UART_H
#define _LOCATE_UART_H

#include "hal_types.h"
#include "Hal_uart.h"
#include "string.h"
#include "GasLocate_protocol_V2.h"

#include "crc.h"

/* UART*/
/***************************************************/
//static uint_8 uartPort = HAL_UART_PORT_0;

#define GASMONITOR_FRAME_TIMEOUT   1// millisecond

extern bool    locateIsValid;
extern uint16  locateDistance;
extern uint16 saveRealLocatedistance;
extern uint16  targetName;
extern uint16 uartWantReadLen;
extern uint8  receive_uart_buff[128];
extern uint8  radioVersion[16];
extern uint16  count10min;
extern int8   i8Rssi;


#define   NO_RSSI      -127
#define MIN10_CONT_SECOND    (10*60)

#define  RESET_JN5148_BV           BV(4)
#define  RESET_JN5148_SBIT         P0_4
#define  RESET_JN5148_DDR          P0DIR
#define  RESET_JN5148_POLARITY     ACTIVE_LOW

#define  RESET_JN5148_BIT         4

#define  INIT_RESET_JN5148()   {    \
	  	P1SEL &= ~(0x1<<RESET_JN5148_BIT);			/*  Select GPIO*/  \
  		P1DIR |= (0x1<<RESET_JN5148_BIT);			/*  Set to output*/  \
  		P1INP &= ~(0x1<<RESET_JN5148_BIT);                 /* set to Pull-up*/     \
}
//extern  void  LocateUart_init(void);
extern  void  GasMonitor_stationID_fromdensity(uint8* pNum10000,uint8* pNum1000,uint8* pNum100, uint8* pNum10,uint8* pNum1,uint16 density);
extern  uint8  Locate_Uart_BuildandSendDataPAN(uint8 *p, uint8 len, uint16 DstAddr, uint16 DstPan);
extern   void GasMonitor_UartWrite(uint8* p, uint8 len, uint8 cmdtype);
extern void GasMonitor_UartWriteACK(uint8 cmdtype);
extern  uint16 GasMonitor_Get_SAVE_LocateDistance(void);
extern  uint16 GasMonitor_GetLocateDistance(void);
extern uint16 GasMonitor_GetLocateTargetName(void);
extern int8 GasMonitor_Get_Network_Rssi(void);
#endif
/***************************************************/
