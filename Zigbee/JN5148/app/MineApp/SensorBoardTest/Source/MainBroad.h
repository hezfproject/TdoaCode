/*
 * MainBroad.h
 *
 *  Created on: 2011-4-25
 *      Author: Administrator
 */

#ifndef MAINBROAD_H_
#define MAINBROAD_H_

#define PORT_RUN_LED       (0x0001)
#define PORT_LINK_LED      (0x0002)
#define PORT_485_LED       (0x0004)
#define PORT_PLUS1_LED     (0x0008)
#define PORT_PLUS2_LED     (0x0010)
#define PORT_CURRENT1_LED  (0x0020)
#define PORT_CURRENT2_LED  (0x0040)
#define PORT_CURRENT3_LED  (0x0080)
#define PORT_CURRENT4_LED  (0x0100)
#define PORT_CURRENT5_LED  (0x0200)
#define PORT_CURRENT6_LED  (0x0400)
#define PORT_CURRENT7_LED  (0x0800)
#define PORT_CURRENT8_LED  (0x1000)
#define PORT_RELAY1_LED    (0x2000)
#define PORT_RELAY2_LED    (0x4000)

extern uint8  gCurrent_Channel ;
extern uint8  gCurrent_Channel_old;

extern uint16 gflash_bit ;

//extern char OD2101_FIFO_UATR [64];  //receive fifo
//extern char OD2101_FIFO_I2C [64];   //send    filo

#endif /* MAINBROAD_H_ */
