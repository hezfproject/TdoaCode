/**************************************************************************************************
  Filename:       App_cfg.h
  Revised:        $Date: 2011/05/10 17:40:09 $
  Revision:       $Revision: 1.4 $

  Description:   Application config.
  **************************************************************************************************/
#ifndef APP_CFG_H
#define APP_CFG_H

/*************************************************************************************************
  *CONSTANTS
  */
/*************************************************************************************************
  *MACROS
  */

//A invalid network Id used by all cards
#define CARD_NWK_ADDR                          0xFFF0
#define CONTROLLER_NWK_ADDR              0xFFF1
#define LOCNODE_NWK_ADDR                    0xFFF2 //for loc node.
#define CARDREADER_NWK_ADDR              0xFFF3
#define MOBILEPHONE_NWK_ADDR		    0xFFF4


/* software versions */
#define MP_SW_VERSION     "1008Anbiao"

/************************************************************************/
/* EXT MAC ADDRESS DEFINATION		                                     */
/************************************************************************/

// Definations of EXT MAC ADDRESS:

// BITS:      7            6              5         4                3                      2                     1                   0
//         undefined  channel      TYPE    Version     Customer(High)    Customer(Low)  DeviceID(High)  DeviceID(Low)

// Extend mac Address Definations, what per byte stand for
#define EXT_MACADDR_DEVID_LBYTE                      0
#define EXT_MACADDR_DEVID_HBYTE                      1
#define EXT_MACADDR_CUSTORMERID_LBYTE            2
#define EXT_MACADDR_CUSTORMERID_HBYTE          3
#define EXT_MACADDR_VERSION                              4
#define EXT_MACADDR_TYPE                                   5
//#ifdef MENU_RF_DEBUG
#define EXT_MACADDR_CHANNEL                             6
//#endif

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
#define EXT_MACADDR_TYPE_INVALID           0xFF

/************************************************************************/
/* Number  distribute definitions					                     */
/************************************************************************/
#define NUMBER_CARD_MIN 1000
#define NUMBER_CARD_MAX 7999
#define NUMBER_IS_CARD(x)  ((x>=NUMBER_CARD_MIN && x<=NUMBER_CARD_MAX)?true:false)

#define NUMBER_PHONE_MIN 8000
#define NUMBER_PHONE_MAX 9999
#define NUMBER_IS_PHONE(x)  ((x>=NUMBER_PHONE_MIN && x<=NUMBER_PHONE_MAX)?true:false)


#define NUMBER_SUBSTATION_MIN 10000
#define NUMBER_SUBSTATION_MAX 11999
#define NUMBER_IS_SUBSTATION(x)  ((x>=NUMBER_SUBSTATION_MIN && x<=NUMBER_SUBSTATION_MAX)?true:false)

#define NUMBER_GASMONITOR_MIN 12000
#define NUMBER_GASMONITOR_MAX 15999
#define NUMBER_IS_GASMONITOR(x)  ((x>=NUMBER_GASMONITOR_MIN && x<=NUMBER_GASMONITOR_MAX)?true:false)


#define NUMBER_LOCNODE_MIN 20000
//#define NUMBER_LOCNODE_MAX 20000
#define NUMBER_IS_LOCNODE(x)  ((x>=NUMBER_LOCNODE_MIN)?true:false)


#endif
