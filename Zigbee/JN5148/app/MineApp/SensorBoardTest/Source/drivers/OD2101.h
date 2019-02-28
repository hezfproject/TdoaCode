/*
 * OD2101.h
 *
 *  Created on: 2011-4-15
 *      Author: Dong Biwen
 */

#ifndef OD2101_H_
#define OD2101_H_

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define OD2101_ADRESS (0x50)
#define OD2101_WR     (0x00) /*数据寄存器*/
#define OD2101_RD	  (0x00)
#define OD2101_UARTBUF (0x01) /*UART接收缓存接收字节数*/
#define OD2101_I2CBUF (0x02) /*I2C可加载字节数*/
#define OD2101_CTRL   (0x03) /*UART接口控制寄存器*/

/****************************************************************************/
/***              Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void OD2101_Init(void);
PUBLIC bool_t OD2101_Write(char *date_pt,uint16 data_num);
//PUBLIC bool_t OD2101_Read(char *date_pt,uint16 data_num);
PUBLIC bool_t OD2101_Write_Byte(char date);
PUBLIC void OD2101_vPrintf(const char *fmt, ...);
#endif /* OD2101_H_ */
