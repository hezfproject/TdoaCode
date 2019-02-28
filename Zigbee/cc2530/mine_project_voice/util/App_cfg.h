/**************************************************************************************************
  Filename:       App_cfg.h
  Revised:        $Date: 2011/07/07 23:48:20 $
  Revision:       $Revision: 1.6 $

  Description:   Application config.
  **************************************************************************************************/
#ifndef APP_CFG_H
#define APP_CFG_H


#include "version.h"
/*************************************************************************************************
  *CONSTANTS
  */
/*************************************************************************************************
  *MACROS
  */
#define MP_SW_VERSION     VERSION##"V2"    //less  14char

//A invalid network Id used by all cards
#define CARD_NWK_ADDR                          0xFFF0
#define CONTROLLER_NWK_ADDR              0xFFF1
#define LOCNODE_NWK_ADDR                    0xFFF2 //for loc node.
#define CARDREADER_NWK_ADDR              0xFFF3
#define MOBILEPHONE_NWK_ADDR		    0xFFF4

/************************************************************************/
/* EXT MAC ADDRESS DEFINATION		                                     */
/************************************************************************/
// TYPE:
//	01 			Card
//	02 			MobilePhone
//	03 			Substation
//	FF 			Unvalid
//	others 		Undefined

#define EXT_MACADDR_TYPE_CARD               0x01
#define EXT_MACADDR_TYPE_PHONE             0x02
#define EXT_MACADDR_TYPE_SUBSTATION     0x03
#define EXT_MACADDR_TYPE_GASMONITOR    0x04
#define EXT_MACADDR_TYPE_LOCNODE		   0x05
#define EXT_MACADDR_TYPE_CARDREADER     0x06
#define EXT_MACADDR_TYPE_SENSORREADER     0x07
#define EXT_MACADDR_TYPE_INVALID           0xFF

// Definations of EXT MAC ADDRESS:

// BITS:      7                   6                  5                 4                3               2                     1                   0
//       NAMEPLATE1 NAMEPLATE2  NAMEPLATE3 NAMEPLATE4     NULL        channel  DeviceID(High)  DeviceID(Low)

// Extend mac Address Definations, what per byte stand for
#define EXT_MACADDR_DEVID_LBYTE				0
#define EXT_MACADDR_DEVID_HBYTE				1
#define EXT_MACADDR_CHANNEL					2
#define EXT_MACADDR_TYPE					3
#define EXT_MACADDR_NAMEPLATE4				4
#define EXT_MACADDR_NAMEPLATE3				5
#define EXT_MACADDR_NAMEPLATE2				6
#define EXT_MACADDR_NAMEPLATE1				7


/************************************************************************/
/* Number  distribute definitions					                     */
/************************************************************************/
//#define NUMBER_CARD_MIN 1000
//#define NUMBER_CARD_MAX 7999
//#define NUMBER_IS_CARD(x)  ((x>=NUMBER_CARD_MIN && x<=NUMBER_CARD_MAX)?true:false)

#define NUMBER_PHONE_MIN 6000
#define NUMBER_PHONE_MAX 8999
#define NUMBER_IS_PHONE(x)  ((x>=NUMBER_PHONE_MIN && x<=NUMBER_PHONE_MAX)?true:false)


//#define NUMBER_SUBSTATION_MIN 10000
//#define NUMBER_SUBSTATION_MAX 11999
//#define NUMBER_IS_SUBSTATION(x)  ((x>=NUMBER_SUBSTATION_MIN && x<=NUMBER_SUBSTATION_MAX)?true:false)

//#define NUMBER_GASMONITOR_MIN 12000
//#define NUMBER_GASMONITOR_MAX 15999
//#define NUMBER_IS_GASMONITOR(x)  ((x>=NUMBER_GASMONITOR_MIN && x<=NUMBER_GASMONITOR_MAX)?true:false)

#endif
