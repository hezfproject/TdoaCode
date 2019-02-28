/****************************************************************************
Filename:       AppProtocol.c
Revised:        $Date: 2011/04/01 23:16:30 $
Revision:       $Revision: 1.1 $
Description:    App protocol wrapper util.
*****************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "AppProtocolWrapper.h"
#include "ZComDef.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */
//the sequence needs to match with msgtype in AppProtocol.h.
/*CONST*/ uint8 AppPacketSizeTable[] = {
	0, //fill the first element of array.
	sizeof (app_Loc_t),
	sizeof(app_Gas_t),
	sizeof(app_GasThresh_t),
	sizeof(app_Voice_t),
	sizeof(app_Mode_t),
	sizeof(app_Urgent_t),
	sizeof(app_Sleep_t),
	sizeof(app_ssReq_t),
	sizeof(app_ssInd_t),
	sizeof(app_PWRCTL_t),
	sizeof(app_startNwk_t),
	sizeof(app_JoinNwk_t),
	sizeof(app_LeaveNwk_t),
	0, //no defination of ZB_DATA_TRANSMIT.
	sizeof(app_Dialup_t),
	//sizeof(app_Hangup_t),
	//sizeof(app_DialAck_t),
	sizeof(app_CmdUp_t),
	sizeof(app_CmdDown_t),
	//sizeof(app_ssRsp_t),
	sizeof(app_DeputyJoin_t),
	sizeof(app_DeputyJoinRsp_t),
	//sizeof(app_NwkAddrNotify_t),
	sizeof(app_TryConnection_t),
	sizeof(app_SignalStrength_t),
	sizeof(app_MPssReq_t),
	sizeof(app_MPssInd_t),
	sizeof(app_TimessInd_t),
	sizeof(app_ClearNWK_t),
	sizeof(app_RejoinNwk_t),
	sizeof(app_SMS_t),
	sizeof(app_SMSRsp_t),
	sizeof(app_MP_Poll_Req_t),
      sizeof(app_MP_Poll_Ack_t),
};

