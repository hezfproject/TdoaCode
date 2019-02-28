/**************************************************************************************************
  Filename:       MineApp_Local.h
  Revised:        $Date: 2011/07/22 17:38:35 $
  Revision:       $Revision: 1.6 $

  Description:   User definable Parameters, used for MineApp_MP and MineApp_MP_Function task only.
  ONLY USE IT IN MineApp_MP and MineApp_MP_Function!!!!
**************************************************************************************************/
#ifndef MINEAPP_LOCAL_H
#define MINEAPP_LOCAL_H
/*********************************************************************
* INCLUDES
*/
#include "ZDApp.h"
#include "AppProtocolWrapper.h"

/*********************************************************************
* TYPEDEFS
*/
typedef struct
{
	termNbr_t Node_nmbr;   //string format of own number
	uint16 nmbr; 			   //number format of own number.
	uint16 peernmbr; 	//number format of peer number.
	termNbr_t peer_termnmbr; 	//number format of peer number.

	bool  IsTalkWithGateWay;    //is dialing with outside number
	int8 coordrssi;   // my coord's rssi
}Node_Attr_t;

typedef struct
{
	bool scanen;
	uint32  peer_tick;
} AppEnScan_info_t;
/*********************************************************************
* GLOBAL VARIABLES
*/
extern const cId_t __code MineApp_InputClusterList[];
extern const cId_t __code MineApp_OutputClusterList[];
extern const SimpleDescriptionFormat_t __code MineApp_SimpleDesc;
extern const endPointDesc_t __code MineApp_epDesc;
extern Node_Attr_t MineApp_NodeAttr;
extern devStates_t MineApp_NwkState;
extern uint8 MineApp_TransID;

extern AppEnScan_info_t  AppEnScan_info;
extern bool    AppDialUpEn;		     // indacate dial up is sending, send dial up continuesly 
extern app_CmdUp_t  AppDialUpData;   // stored  dial up data 
extern bool                AppDialUpFound; // have received found or accept when dialup 
extern APPWrapper_t AppData;
extern afAddrType_t dstAddr;
//extern APPWrapper_t SignalData;
extern uint8 MineApp_SignalLengh;

#endif
