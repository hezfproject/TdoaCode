/****************************************************************************
Filename:       AppProtocolWrapper.h
Revised:        $Date: 2011/04/01 23:16:30 $
Revision:       $Revision: 1.1 $
Description:    App protocol wrapper util.
*****************************************************************************/
#ifndef APPPROTOCOLWRAPPER_H
#define APPPROTOCOLWRAPPER_H

/*********************************************************************
 * INCLUDES
 */
#include "AppProtocol.h"

/*********************************************************************
 * TYPEDEFS
 */
typedef union
{
	unsigned char 	app_flag;
	app_Loc_t app_Loc;
	app_Gas_t app_Gas;
	app_GasThresh_t app_GasThr;
	app_Voice_t app_Voice;
	app_Mode_t app_Mode;
	app_Urgent_t app_Urgent;
	app_Sleep_t app_Sleep;
	app_ssReq_t app_ssReq;
	app_ssInd_t app_ssInd;
	app_PWRCTL_t app_PWRCTL;
	app_startNwk_t app_StartNwk;
	app_JoinNwk_t app_JoinNwk;
	app_LeaveNwk_t app_LeaveNwk;
	app_Dialup_t app_Dialup;
	//app_Hangup_t app_Hangup;
	//app_DialAck_t app_DialAck;
	app_CmdUp_t app_Cmdup;
	app_CmdDown_t app_CmdDown;
	//app_ssRsp_t app_ssRsp;
	app_DeputyJoin_t app_DeputyJoin;
	app_DeputyJoinRsp_t app_DeputyJoinrsp;
	app_TryConnection_t  app_TryConnection;
      app_SignalStrength_t app_SignalStrength;
	app_MPssReq_t app_MPssReq;
	app_MPssInd_t app_MPssInd;
      app_TimessInd_t app_TimessInd;
	app_ClearNWK_t  app_ClearNWK;
	app_RejoinNwk_t app_RejoinNwk;
	app_SMS_t          app_SMS;
	app_SMSRsp_t     app_SMSRsp;
	app_MP_Poll_Req_t app_MP_Poll_Req;
	app_MP_Poll_Ack_t app_MP_Poll_Ack;
	app_ZC_Report_t app_ZC_Report;
	app_CrossPan_t app_CrossPan;
	app_CrossPanAck_t app_CrossPanAck;
	app_Signal_ACK_t app_Signal_ACK;
	app_ZcDetect_t app_ZcDetect;
	app_SPIDetect_t app_SPIDetect;
	app_SPIDebug_t app_SPIDebug;
      app_RFDebug_t  app_RFDebug;
	app_GasReport_t app_GasReport;
	app_LocNodeCast_t app_LocNodeCast;
	app_GasAlarm_t app_GasAlarm;
	app_GasAlarm_ack_t app_GasAlarm_ack;
	app_amquery_t app_amquery;
	app_amret_t app_amret;
	app_amdata_t app_amdata;
	app_amack_t app_amack;
	app_am_routerpt_t app_am_routerpt;
	app_gassms_t app_gassms;
	app_gassms_ack_t app_gassms_ack;
	app_amdata_cmd_t app_amdata_cmd;
	app_chargeed_ssReq_t app_chargeed_ssReq;
	app_chargeed_ssRsp_t app_chargeed_ssRsp;
	app_chargeed_ssInd_t app_chargeed_ssInd;
	app_locnode_info_req_t app_locnode_info_req;
	app_locnode_info_set_t app_locnode_info_set;
	app_dataCrossPan_t app_dataCrossPan;
	app_RouteInfo_t app_RouteInfo;
	app_rfversion_t   app_rfversion;
    app_rfmac_query_t app_rfmac_query;
	app_rfmac_set_t  app_rfmac_set;
	
}APPWrapper_t;

typedef struct
{
	spi_event_hdr_t hdr;
	APPWrapper_t data;
}AppPacket_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */
//extern /*CONST*/ unsigned char AppPacketSizeTable[];

#endif

