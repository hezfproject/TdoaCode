/**************************************************************************************************
  Filename:       App_cfg.h
  Revised:        $Date: 2011/08/27 00:58:11 $
  Revision:       $Revision: 1.64 $

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
/*
move to appprotocol.h
#define MAX_DEPTH 10
#define INIT_TRANID 0
#define INIT_OPN AF_DISCV_ROUTE
*/
//#ifndef DEVNUMBER
//#define DEVNUMBER   0x1234
//#endif

//device start option.

/************************************************************************/
/* Cluster IDs		                                                      */
/************************************************************************/

#define MINEAPP_CLUSTERID  		    0x0001
#define CARD_CLUSTERID  		    0x0002
#define GASMONITOR_CLUSTERID  	    0x0004
#define LOCNODE_CLUSTERID  	    	    0x0008
#define CHARGEED_CLUSTERID  	    0x0009 // the old one are one shot, new ones start sequence
#define BLAST_CLUSTERID             0x000A

#define TOF_CLUSTERID                      0x1000
#if (defined CARDTRANSFER_COLLECTOR) || (defined CARDTRANSFER_DATACENTER)
#define NWK_CLUSTERID           0x000a
#endif

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
#define EXT_MACADDR_TYPE_SENSORREADER     0x07
#define EXT_MACADDR_TYPE_BLAST              0x08
#define EXT_MACADDR_TYPE_INVALID           0xFF

//Version£ºDevice version, is 0x01 now

//CustomerID: Two Chars£¬Write its ASCII  in bit 3:2

//DeviceID:
//Card		1000-7999
//Phone       	8000-9999
//Substation	10000-16000


/************************************************************************/
/* NV_ID settings					                                     */
/************************************************************************/
    #define SANY_VERSION           VERSION

 #ifdef MINE_PROJECTS
	#define MINEAPP_NV_DIALED        0x0206
	#define MINEAPP_NV_MISSED        0x0207
	#define MINEAPP_NV_ANSWERED   0x0208
	#ifdef NEW_DOUBLE_NVID_OP
	#define MINEAPP_NV_CONTACT1      0x0210
	#define MINEAPP_NV_CONTACT2      0x0211
	#define MINEAPP_NV_SMS1                0x0212
	#define MINEAPP_NV_SMS2                0x0213
	#else
	#define MINEAPP_NV_SMS               0x020A
	#define MINEAPP_NV_CONTACT      0x0209
	#endif

	#define MINEAPP_SW_VERSION     VERSION

	#ifdef MENU_RF_DEBUG
	#define ZCD_NV_SET_CHANLIST     0x020B
	#define ZCD_NV_SET_PANID           0x020C
	#endif
	#ifdef SMS_TEMPLATE
	#define MINEAPP_NV_SMS_TEMPLATE              0x020D
	#endif
	#ifdef SMS_SENDBOX
	#define MINEAPP_NV_SMS_SENDBOX                0x020F
	#endif
	#define MINEAPP_NV_SET_INFORMATION        0x020E//set information include volume, backlight and time

#elif (defined GASMONITOR_PROJECT)
	#define GASSMS_MAX_NUM			   10
	#define GASMONITOR_NV_SETTINGS        0x0206
	#define GASMONITOR_NV_SMS_FLAGS	   0x0207
	#define GASMONITOR_NV_SMS_BASE	   0x0208
	#define GASMONITOR_NV_SMS_END	   (GASMONITOR_NV_SMS_BASE + GASSMS_MAX_NUM)
	#define GASMONITOR_SW_VERSION VERSION
#endif



#ifndef BLAST_RSSI_PERIOD
#define BLAST_RSSI_PERIOD         3000//10000        /* Blast RSSI to substation.*/
#endif

#define LOCNODE_VOL  1 //Location node voltage threshold.


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
#define NUMBER_SUBSTATION_MAX 12999
#define NUMBER_IS_SUBSTATION(x)  ((x>=NUMBER_SUBSTATION_MIN && x<=NUMBER_SUBSTATION_MAX)?true:false)

#define NUMBER_GASMONITOR_MIN 13000
#define NUMBER_GASMONITOR_MAX 15999
#define NUMBER_IS_GASMONITOR(x)  ((x>=NUMBER_GASMONITOR_MIN && x<=NUMBER_GASMONITOR_MAX)?true:false)


#define NUMBER_LOCNODE_MIN 20000
//#define NUMBER_LOCNODE_MAX 20000
#define NUMBER_IS_LOCNODE(x)  ((x>=NUMBER_LOCNODE_MIN)?true:false)

#endif

/************************************************************************/
/* App Settings		                                                      */
/************************************************************************/
#define APP_STARTUP_COORD   2       // Start up as coordinator only
#define APP_STARTUP_ROUTER  1       // Start up as router only
#define APP_STARTUP_ENDEVICE    0     //Start up as enddevice only

/************************************************************************/
/* MineApp Settings                                                      */
/************************************************************************/
#define MINEAPP_MAX_DATA_LEN  	120
#define MINEAPP_ENDPOINT                 0x21
#define MINEAPP_PROFID                   	0x2001
#define MINEAPP_APS_PROFID              0x0100
#define MINEAPP_DEVICEID                 13
#define MINEAPP_VERSION                  31
#define MINEAPP_FLAGS                   	 0

#ifndef BLAST_SIGNALSTRENGTH_PERIOD
#define BLAST_SIGNALSTRENGTH_PERIOD         3000        /* Blast signal strength packets*/
#endif
#define DMA_RT_MAX  128 //can save in nv to dynamic adjust when device wake up.

/************************************************************************/
/* Card Settings                                                     							*/
/************************************************************************/
#define MED_POLL_INTERVAL        3           	 /* Number of signals send once*/
#define MED_POLL_TIMEOUT         40         	 /* Period of signals send once*/
#define MED_SLEEP_PERIOD          5000         	/* sleep length in ms */

/************************************************************************/
/* Jennic Card Settings                                                     							*/
/************************************************************************/
#define JCARD_POLL_INTERVAL        3           	 /* Number of signals send once*/
#define JCARD_POLL_TIMEOUT         40         	 /* Period of signals send once*/
#define JCARD_SLEEP_PERIOD          5000         	/* sleep length in ms */


/************************************************************************/
/* Location Node  Settings                                                     					*/
/************************************************************************/
#define MLN_LOCCAST_FREQ 			300 		//every 300 ms  blast LocNode Cast.
#define MLN_VDD_LIMT				24              /* Vdd limit 2.4v*/

/************************************************************************/
/* Charge Card Settings                                                    						 */
/************************************************************************/

/* 3s */
//#define CHARGEED_RECV_PERIOD           (5*MLN_LOCCAST_FREQ)
//#define CHARGEED_BLAST_PERIOD         (2*CHARGEED_RECV_PERIOD)

/* 4.8s */
#define CHARGEED_RECV_PERIOD           (8*MLN_LOCCAST_FREQ)
#define CHARGEED_BLAST_PERIOD         (2*CHARGEED_RECV_PERIOD)

/************************************************************************/
/* Card Reader Settings                                                    						 */
/************************************************************************/
#define CARDREADER_START_ADDRESS    30000
#define CARDREADER_END_ADDRESS        30253

#define CARDREADER_485_FRAME_INTERVAL 50  //ms

/************************************************************************/
/* Different A 			                                                     					*/
/************************************************************************/
#define A_SUBSTATION 	 (-15)
#define A_LOCNODE		 (-34)



