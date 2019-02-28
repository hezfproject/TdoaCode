/**************************************************************************************************
Filename:       MacUtil.h
Revised:        $Date: 2011/04/22 18:51:37 $
Revision:       $Revision: 1.3 $

Description:    This file contains the upper Head build and remove based on mac layer
**************************************************************************************************/

#ifndef MAC_UTIL_H
#define MAC_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

	/**************************************************************************************************
	* INCLUDES
	**************************************************************************************************/
#include "Hal_types.h"
#include "Saddr.h"
#include "Sdata.h"
	/**************************************************************************************************
	* DEFINES
	**************************************************************************************************/
#define MAC_UTIL_UNICAST      0
#define MAC_UTIL_BROADCAST   1

#define MAC_UTIL_BROADCAST_SHORTADDR_DEVZCZR     0xFFFC
#define MAC_UTIL_BROADCAST_SHORTADDR_DEVALL       0xFFFF

extern uint8 MAC_UTIL_McpsDataReq(const uint8* data, uint8 dataLength,uint16 panID,sAddr_t dstAddr,uint8 txOption);
extern uint8 MAC_UTIL_SendDataPAN(uint16 dstPID, uint8 *p, uint8 len, uint8 DeliverMode, uint16 uniDstAddr, uint8 txOption);

#ifdef __cplusplus
}
#endif

#endif

