/**************************************************************************************************
Filename:       MineApp_Local.c
Revised:        $Date: 2011/07/22 17:38:35 $
Revision:       $Revision: 1.5 $

Description:  User definable common Parameters for MineApp_MP and MineApp_MP_Function task.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "App_cfg.h"
#include "MineApp_MP.h"
#include "MineApp_Local.h"

/*********************************************************************
* GLOBAL VARIABLES
*/
const cId_t __code MineApp_InputClusterList[] = 
{
	MINEAPP_CLUSTERID,
};
const cId_t __code MineApp_OutputClusterList[] = 
{
	MINEAPP_CLUSTERID,
};

const SimpleDescriptionFormat_t __code MineApp_SimpleDesc =
{
	MINEAPP_ENDPOINT,
	MINEAPP_PROFID,

	MINEAPP_DEVICEID,

	MINEAPP_VERSION,
	MINEAPP_FLAGS,

	sizeof(MineApp_InputClusterList),
	(cId_t*)MineApp_InputClusterList,

	sizeof(MineApp_OutputClusterList),
	(cId_t*)MineApp_OutputClusterList
};

const endPointDesc_t __code MineApp_epDesc =
{
	MINEAPP_ENDPOINT,
	&MineApp_TaskID,
	(SimpleDescriptionFormat_t *)&MineApp_SimpleDesc,
	noLatencyReqs
};

Node_Attr_t MineApp_NodeAttr;
devStates_t MineApp_NwkState;
uint8 MineApp_TransID;

AppEnScan_info_t  AppEnScan_info;
app_CmdUp_t  AppDialUpData;   // stored  dial up data 
bool                AppDialUpFound; // have received found or accept when dialup 
APPWrapper_t AppData;
afAddrType_t dstAddr;
//APPWrapper_t SignalData;
uint8 MineApp_SignalLengh = 0;
