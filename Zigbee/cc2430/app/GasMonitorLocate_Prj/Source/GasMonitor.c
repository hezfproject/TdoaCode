/**************************************************************************************************
Filename:       GasMonitor.c
Revised:        $Date: 2011/08/06 02:03:00 $
Revision:       $Revision: 1.9 $

Description:    Mine Application of station.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "GasMonitor.h"
#include "GasMonitor_MenuLib.h"
#include "GasMonitorDep.h"
#include "GasMonitor_sms.h"
#include "GasMenuLib_tree.h"
#include "GasMenuLib_global.h"

#include "AppProtocolWrapper.h"
#include "MacUtil.h"
#include "App_cfg.h"
#include "Mac_radio_defs.h"
#include "Mac_spec.h"
#include "drivers.h"
#include "delay.h"
#include "WatchdogUtil.h"

#if !defined( NONWK )
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "ZGlobals.h"
#endif

#include "OSAL.h"
#include "OSAL_Nv.h"
#include "key.h"
#include "OnBoard.h"
#include "hal_adc.h"
#include "MenuAdjustUtil.h"
#include "GasMenulib_orphan.h"
#include "ch4.h"
#include "beeper.h"
#include "string.h"

#include "Locate_uart.h"
/*********************************************************************
* DEFINES
*/
//#define RSSI_MIN      (MAC_RADIO_RECEIVER_SENSITIVITY_DBM + MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY)
//#define RSSI_MAX    MAC_RADIO_RECEIVER_SATURATION_DBM

#define RSSI_MIN     -81 //(MAC_RADIO_RECEIVER_SENSITIVITY_DBM + MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY)
#define RSSI_MAX    10 //MAC_RADIO_RECEIVER_SATURATION_DBM
#define POWEROFF         6  //shutdown command

#define CONV_LQI_TO_RSSI( lqi )  (int8)(( lqi*((int16)RSSI_MAX - (int16)RSSI_MIN)/MAC_SPEC_ED_MAX + (int16)RSSI_MIN))

typedef struct
{
    uint16 LocNode_ID;
    float  meanRSSI;
    uint8  RssiCnt;
} LocRssiRecord_t;


static void  GasMonitor_ParseUartCmd(uint8 * data);
static uint8 Locate_update_count=0;

uint8 shutdown = 0; 


/*********************************************************************
* GLOBAL VARIABLES
*/
uint8 GasMonitor_TaskID;
/*********************************************************************
* LOCAL VARIABLES
*/
static const cId_t GasMonitor_InputClusterList[] =
{
    GASMONITOR_CLUSTERID,
    CARD_CLUSTERID,
    LOCNODE_CLUSTERID,
    CHARGEED_CLUSTERID,
};
static const cId_t GasMonitor_OutputClusterList[] =
{
    GASMONITOR_CLUSTERID,
    CARD_CLUSTERID,
    CHARGEED_CLUSTERID
};

static const SimpleDescriptionFormat_t GasMonitor_SimpleDesc =
{
    MINEAPP_ENDPOINT,
    MINEAPP_PROFID,

    MINEAPP_DEVICEID,

    MINEAPP_VERSION,
    MINEAPP_FLAGS,

    sizeof(GasMonitor_InputClusterList),
    (cId_t*)GasMonitor_InputClusterList,

    sizeof(GasMonitor_OutputClusterList),
    (cId_t*)GasMonitor_OutputClusterList
};

static const endPointDesc_t GasMonitor_epDesc =
{
    MINEAPP_ENDPOINT,
    &GasMonitor_TaskID,
    (SimpleDescriptionFormat_t *)&GasMonitor_SimpleDesc,
    noLatencyReqs
};

__idata static  int8  GasMonitor_RSSI;
static devStates_t GasMonitor_NwkState = DEV_INIT;
static uint16 GasMonitor_Keys;
static uint16 GasMonitor_Density;
static uint16 Gas_UpdateCnt = 0;
static uint16 GasMonitor_seq = 0;
//static uint16 seq = 0;

static GasAlarmType SOSAlarmtype;
static uint8 GasMonitor_SOS_Cnt=0;
static uint8 GasMonitor_SOSAlarm_Cnt=0;

static LocRssiRecord_t LocNodeList[LOCNODENUM];
static int8 LocNodeCnt = 0;

static uint8 GasMonitor_SignalLevel;
static bool Menu_IsOverDensityed = FALSE;
static uint8 GasMonitor_PwrOffReason;


buf_t             data_buf = {0, NULL};
static uint8 uartPortTemp = HAL_UART_PORT_0;

static __code const int8 RSSI_table[] =
{
    -80,		//LQI 28,
    -70,		//LQI 53,
    -50,		//LQI 104,
    -30,		//LQI 154,
};

/*********************************************************************
* LOCAL FUNCTIONS
*/
static void GasMonitor_ProcessMSGCB(const afIncomingMSGPacket_t *MSGpkt);
static void GasMonitor_HandleKeys(void);
static void GasMonitor_RingAndPowerOff(uint8 reason);
static void GasMonitor_UpdateLocInfo(LocRssiRecord_t* LocNode, const LocPos_t* newnode);
static void GasMonitor_ProcessGasDensity(void);
static uint8 GasMonitor_GasDensityReport(uint16 density);
static void GasMonitor_LocSortByRSSI(LocRssiRecord_t* LocNode, int8 LastCnt);

/*********************************************************************
* FUNCTIONS
*********************************************************************/

/*********************************************************************
* @fn      GasMonitor_Init
*
* @brief   Initialization function for the MineApp OSAL task.
*
* @param   task_id - the ID assigned by OSAL.
*
* @return  none
*/
void GasMonitor_Init( uint8 task_id )
{
    GasMonitor_TaskID = task_id;

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    StartWatchDog ( DOGTIMER_INTERVAL_1S );
    osal_set_event (GasMonitor_TaskID, GASMONITOR_FEEDWATCHDOG_EVENT);
#endif

    afRegister( (endPointDesc_t *)&GasMonitor_epDesc);
    RegisterForKeys(GasMonitor_TaskID);
    HalResetBackLightEvent();
    Menu_Init();

    // check vdd when start up
    /*
    if(!GasMonitor_vdd_check())
    {
        HalResetBackLightEvent();
        Menu_handle_msg(MSG_NO_POWER, NULL, 0);
        GasMonitor_RingAndPowerOff(GASNODE_POWEROFF_NOPOWER);
        return;
    }
    */
    Gasmonitor_ReadDevDateInfo();

    Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
    HalStartBeeper(BEEP_TYPE_POWERONOFF,0);

    //Menu_handle_msg(MSG_POWERON_ANIM, NULL, 0);
    Menu_UpdateBattery(HalAdcMeasureVdd());

    //INIT_RESET_JN5148()
    P0SEL &= ~(0x1<<RESET_JN5148_BIT);			/*  Select GPIO*/
    P0DIR |= (0x1<<RESET_JN5148_BIT);			/*  Set to output*/
    //P1INP &= ~(0x1<<RESET_JN5148_BIT);                 /* set to Pull-up*/

    RESET_JN5148_SBIT=0;
    DelayMs(5);
    RESET_JN5148_SBIT=1;    

    halUARTCfg_t uartConfig;
    /* UART Configuration */
    uartConfig.configured           = TRUE;
    uartConfig.baudRate             = HAL_UART_BR_115200;
    uartConfig.flowControl          = false;
    uartConfig.flowControlThreshold = SPI_THRESHOLD;
    uartConfig.rx.maxBufSize        = SPI_RX_BUFF_MAX;
    uartConfig.tx.maxBufSize        = SPI_TX_BUFF_MAX;
    uartConfig.idleTimeout          = SPI_IDLE_TIMEOUT;
    uartConfig.intEnable            = TRUE;
    uartConfig.callBackFunc         = NULL;

    /* Start UART */
    HalUARTOpen (HAL_UART_PORT_0, &uartConfig);

    if(ZSuccess!=osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_POWERON_EVENT, 100))
    {
        SystemReset();
    }
    if(ZSuccess!=osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_UPDATETIME_EVENT, 2000))
    {
        SystemReset();
    }
    if(ZSuccess!=osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_UPDATEGAS_EVENT, 2000))
    {
        SystemReset();
    }
    if(ZSuccess != osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_POWERON_ALARM_EVENT,100))
    {
        SystemReset();
    }
    MacUtil_t MacUtil;
    MacUtil.panID = 0;  /* Not fixed. */
    MacUtil.dst_endpoint = MINEAPP_ENDPOINT;
    MacUtil.src_endpoint = 0x20;//MINEAPP_ENDPOINT;
    MacUtil.profile_id = 0x0100;//MINEAPP_PROFID;
    MacUtil.cluster_id = GASMONITOR_CLUSTERID;
    MacUtil.NodeType = ZDO_Config_Node_Descriptor.LogicalType;
    //MAC_UTIL_INIT ( &MacUtil );
    MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP);

    osal_memset(LocNodeList, 0, LOCNODENUM*sizeof(LocNodeList[0]));
}




/*********************************************************************
* @fn      GasMonitor_ProcessEvent
*
* @brief   Mine Application Task event processor.
*
* @param   task_id  - The OSAL assigned task ID.
* @param   events - Bit map of events to process.
*
* @return  none
*/
uint16 GasMonitor_ProcessEvent( uint8 task_id, uint16 events )
{
    afIncomingMSGPacket_t* MSGpkt;

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    if (events & GASMONITOR_FEEDWATCHDOG_EVENT)
    {
        if(ZSuccess!=osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_FEEDWATCHDOG_EVENT, 300))
        {
            SystemReset();
        }
        FEEDWATCHDOG();
        return events ^ GASMONITOR_FEEDWATCHDOG_EVENT;
    }
#endif
    if (events & SYS_EVENT_MSG)
    {
        MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(GasMonitor_TaskID);
        while ( MSGpkt )
        {
            switch ( MSGpkt->hdr.event )
            {
            case ZDO_STATE_CHANGE:
            {
                GasMonitor_NwkState = (devStates_t)(MSGpkt->hdr.status);

                if(GasMonitor_NwkState == DEV_ZB_COORD)
                {
                    if(ZSuccess != osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_POWERON_ALARM_EVENT,100))
                    {
                        SystemReset();
                    }

                }
                break;
            }
            case KEY_CHANGE:
            {
                GasMonitor_Keys = ((keyChange_t *)MSGpkt)->keys;
                if(osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_KEYDEJITTER_EVENT, 10) != ZSUCCESS)
                {
                    SystemReset();
                }
                break;
            }
            case AF_INCOMING_MSG_CMD:
            {
                GasMonitor_ProcessMSGCB(MSGpkt);
                break;
            }
            case AF_DATA_CONFIRM_CMD:
            {
                break;
            }
            default:
                break;
            }
            osal_msg_deallocate( (uint8 *)MSGpkt );
            MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GasMonitor_TaskID );
        }
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & GASMONITOR_UART_READ0_EVENT)
    {
        GasMonitor_ParseUartCmd(receive_uart_buff);
        memset(receive_uart_buff, 0, 128);
        return events ^ GASMONITOR_UART_READ0_EVENT;
    }
	
    if(events & GASMONITOR_UART_SEND_EVENT)
    {
        GasMonitor_UartWrite(aExtendedAddress,8,SYNC_ACK);
        return events ^ GASMONITOR_UART_SEND_EVENT;
    }
	
    if(events & GASMONITOR_POWEROFF_EVENT)
    {
        static uint8 pwroff_seqnum;
        if(pwroff_seqnum<3)
        {

            IEN0 &= 0x7f; 
            GasMonitor_UartWrite((&shutdown),1,POWEROFF);
            IEN0 |= 0x80;
            app_GasAlarm_t app_GasAlarm;
            app_GasAlarm.msgtype = GASALARM;
            app_GasAlarm.DevID = zgConfigPANID;
            app_GasAlarm.DstPan = 0xFFFF;
            app_GasAlarm.seq = pwroff_seqnum++;
            app_GasAlarm.AlarmType = GASNODE_POWEROFF;
            app_GasAlarm.value = GasMonitor_PwrOffReason;
            MacParam_t param;
            param.panID = 0xFFFF;
            param.cluster_id = GASMONITOR_CLUSTERID;
            param.radius = 0x01;
            //MAC_UTIL_BuildandSendDataPAN ( &param, ( uint8 * ) &app_GasAlarm, sizeof ( app_GasAlarm ), MAC_UTIL_UNICAST, 0, NULL );
            Locate_Uart_BuildandSendDataPAN(( uint8 * ) &app_GasAlarm, sizeof ( app_GasAlarm ),0,0xFFFF);

            if(ZSuccess != osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_POWEROFF_EVENT,100))
            {
                SystemReset();
            }
        }
        else
        {
            GasMonitor_UartWrite((&shutdown),1,POWEROFF);  
            GasMonitor_PowerOFF();
        }
        return events ^ GASMONITOR_POWEROFF_EVENT;
    }
    if(events & GASMONITOR_KEYDEJITTER_EVENT)
    {
        GasMonitor_HandleKeys();
        return events ^ GASMONITOR_KEYDEJITTER_EVENT;
    }
    if(events & GASMONITOR_POWERON_EVENT)
    {
        HalStopBeeper(BEEP_TYPE_POWERONOFF,true);
        return events ^ GASMONITOR_POWERON_EVENT;
    }
    if(events & GASMONITOR_POWERON_ALARM_EVENT)
    {
        static uint8 pwron_seqnum;
        if(pwron_seqnum<3)
        {
            app_GasAlarm_t app_GasAlarm;
            app_GasAlarm.msgtype = GASALARM;
            app_GasAlarm.DevID = zgConfigPANID;
            app_GasAlarm.DstPan = 0xFFFF;
            app_GasAlarm.seq = pwron_seqnum++;
            app_GasAlarm.AlarmType = GASNODE_POWERON;
            app_GasAlarm.value = 0;
            MacParam_t param;
            param.panID = 0xFFFF;
            param.cluster_id = GASMONITOR_CLUSTERID;
            param.radius = 0x01;
            //MAC_UTIL_BuildandSendDataPAN ( &param, ( uint8 * ) &app_GasAlarm, sizeof ( app_GasAlarm ), MAC_UTIL_UNICAST, 0, NULL );
            Locate_Uart_BuildandSendDataPAN(( uint8 * ) &app_GasAlarm, sizeof ( app_GasAlarm ),0,0xFFFF);

            if(ZSuccess != osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_POWERON_ALARM_EVENT,100))
            {
                SystemReset();
            }
        }

        return events ^ GASMONITOR_POWERON_ALARM_EVENT;
    }
    if (events & GASMONITOR_MENULIB_EVENT)
    {
        Menu_ProcessMenuLibEvt();
        return events ^ GASMONITOR_MENULIB_EVENT;
    }
    if(events & GASMONITOR_ITSELFMENULIB_EVENT)
    {
        menu_JumptoItself();
        return events ^GASMONITOR_ITSELFMENULIB_EVENT;
    }

    if(events & GASMONITOR_MOTORCLOSE_EVENT)
    {
        //MOTOR_CLOSE();
        //sent MOTOR_CLOSE to  JN5148
        return events ^ GASMONITOR_MOTORCLOSE_EVENT;
    }

    if (events & GASMONITOR_UPDATETIME_EVENT)
    {
        /* Update battery*/
        uint8 battery = HalAdcMeasureVdd();

        Menu_UpdateBattery(battery);
        Menu_UpdateTime();
        //beep_begin();
        if(battery <= 0)
        {
            //    HalResetBackLightEvent();
            //    Menu_handle_msg(MSG_NO_POWER, NULL, 0);
            //    GasMonitor_RingAndPowerOff(GASNODE_POWEROFF_NOPOWER);
            //     return 0;
        }
        if(ZSuccess != osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_UPDATETIME_EVENT, 10000))
        {
            SystemReset();
        }
        return events ^ GASMONITOR_UPDATETIME_EVENT;
    }
    if (events & GASMONITOR_UPDATEGAS_EVENT)
    {
        Gas_UpdateCnt++;
        if(ch4_GetLdoStatus()) // if Ldo is On
        {
            GasMonitor_Density = (uint16) ch4_get_density();
            GasMonitor_ProcessGasDensity();
        }

        if (Gas_UpdateCnt >= REPORTFREQUENCE)
        {
            Gas_UpdateCnt = 0;
            if(ch4_GetLdoStatus())
            {
                GasMonitor_GasDensityReport(GasMonitor_Density);
            }
            else
            {
                GasMonitor_GasDensityReport(GAS_INVALID_DENSITY);
            }

        }

        /* update signal level every 20 seconds */
        static uint8 SignalLevelCnt;
        if(SignalLevelCnt++ %3 ==0)
        {
            Menu_UpdateSignal(GasMonitor_SignalLevel);
            GasMonitor_SignalLevel = 0;
        }

        if(ZSuccess != osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_UPDATEGAS_EVENT, 1000))
        {
            SystemReset();
        }
        return events ^ GASMONITOR_UPDATEGAS_EVENT;
    }
    if(events & GASMONITOR_CALLHELP_EVENT)
    {

        if(GasMonitor_SOS_Cnt <=CALLINGHELP_MAXCNT)
        {
            app_GasAlarm_t app_GasAlarm;
            app_GasAlarm.msgtype = GASALARM;
            app_GasAlarm.DevID = zgConfigPANID;
            app_GasAlarm.DstPan = 0xFFFF;
            app_GasAlarm.seq = GasMonitor_SOS_Cnt++;
            app_GasAlarm.AlarmType = GASNODE_URGENT;
            app_GasAlarm.value = 0;

            MacParam_t param;
            param.panID = 0xFFFF;
            param.cluster_id = GASMONITOR_CLUSTERID;
            param.radius = 0x01;
            //MAC_UTIL_BuildandSendDataPAN ( &param, ( uint8 * ) &app_GasAlarm, sizeof ( app_GasAlarm ),MAC_UTIL_UNICAST, 0, NULL);
            Locate_Uart_BuildandSendDataPAN(( uint8 * ) &app_GasAlarm, sizeof ( app_GasAlarm ),0,0xFFFF);

            Menu_handle_msg(MSG_SOS, &GasMonitor_SOS_Cnt, sizeof(GasMonitor_SOS_Cnt));

            if(ZSuccess!= osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_CALLHELP_EVENT, 1000))
            {
                SystemReset();
            }
        }
        else
        {
            GasMonitor_SOS_Cnt = 0xFF;
            Menu_handle_msg(MSG_SOS, &GasMonitor_SOS_Cnt, sizeof(GasMonitor_SOS_Cnt));
        }
        return events ^ GASMONITOR_CALLHELP_EVENT;
    }

    if(events & GASMONITOR_SOSALARM_EVENT)
    {

        if(GasMonitor_SOSAlarm_Cnt <=CALLINGHELP_MAXCNT)
        {
            app_GasAlarm_t app_GasAlarm;
            app_GasAlarm.msgtype = GASALARM;
            app_GasAlarm.DevID = zgConfigPANID;
            app_GasAlarm.DstPan = 0xFFFF;
            app_GasAlarm.seq = GasMonitor_SOSAlarm_Cnt++;
            app_GasAlarm.AlarmType=SOSAlarmtype;
            app_GasAlarm.value = 0;

            MacParam_t param;
            param.panID = 0xFFFF;
            param.cluster_id = GASMONITOR_CLUSTERID;
            param.radius = 0x01;
            //MAC_UTIL_BuildandSendDataPAN ( &param, ( uint8 * ) &app_GasAlarm, sizeof ( app_GasAlarm ),MAC_UTIL_UNICAST, 0, NULL );
            Locate_Uart_BuildandSendDataPAN(( uint8 * ) &app_GasAlarm, sizeof ( app_GasAlarm ),0,0xFFFF);

            Menu_handle_msg(MSG_SOSALARM, &GasMonitor_SOSAlarm_Cnt, sizeof(GasMonitor_SOSAlarm_Cnt));

            if(ZSuccess!= osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_SOSALARM_EVENT, 1000))
            {
                SystemReset();
            }
        }
        else
        {
            GasMonitor_SOSAlarm_Cnt = 0xFF;
            Menu_handle_msg(MSG_SOSALARM, &GasMonitor_SOSAlarm_Cnt, sizeof(GasMonitor_SOSAlarm_Cnt));
        }
        return events ^ GASMONITOR_SOSALARM_EVENT;
    }



    return 0;
}


/*********************************************************************
* @fn      GasMonitor_ProcessMSGCB
*
* @brief   This function processes OTA incoming message.
*
* @param   MSGpkt - The incoming msg.
*
* @return  none
*/
void GasMonitor_ProcessMSGCB(const afIncomingMSGPacket_t *MSGpkt)
{
    APPWrapper_t* AppPkt = ( APPWrapper_t* ) ( MSGpkt->cmd.Data );
#if (defined WATCHDOG) &&(WATCHDOG == TRUE)
    FeedWatchDog();
#endif

    switch(AppPkt->app_flag )
    {
    case LOCNODE_CAST:
    {
        //Compare and insert into LocNode.
        LocPos_t locpos;
        locpos.LocNode_ID = AppPkt->app_LocNodeCast.DevID;
        GasMonitor_RSSI = CONV_LQI_TO_RSSI (MSGpkt->LinkQuality );
        locpos.rssi = GasMonitor_RSSI;
        if(NUMBER_IS_LOCNODE(locpos.LocNode_ID))
        {
            locpos.rssi  += A_SUBSTATION-A_LOCNODE; //normalization to substation value
        }

        uint8 signalLevel = GasMonitor_RSSI2Level(locpos.rssi);
        if(signalLevel > GasMonitor_SignalLevel)     // get max signal level
        {
            GasMonitor_SignalLevel = signalLevel;
        }

        GasMonitor_UpdateLocInfo(LocNodeList, &locpos);

        if (AppPkt->app_LocNodeCast.vol < LOCNODE_VOL)
        {
            app_LocNodeAlarm_t app_LocNodeAlarm;
            app_LocNodeAlarm.msgtype = LOCNODE_ALARM;
            app_LocNodeAlarm.DevID = _NIB.nwkPanId;
            app_LocNodeAlarm.DstPan = 0xFFFF;
            app_LocNodeAlarm.seq = AppPkt->app_LocNodeCast.seq;
            app_LocNodeAlarm.AlarmType = LOCNODE_LOW_BATTERY;
            app_LocNodeAlarm.LocNodeID = MSGpkt->srcAddr.addr.shortAddr;
            app_LocNodeAlarm.value = AppPkt->app_LocNodeCast.vol;

            MacParam_t param;
            param.panID = 0xFFFF;
            param.cluster_id = GASMONITOR_CLUSTERID;
            param.radius = 0x01;
            //MAC_UTIL_BuildandSendDataPAN ( &param, ( uint8 * ) &app_LocNodeAlarm, sizeof ( app_LocNodeAlarm_t ), MAC_UTIL_UNICAST, 0, NULL );
            Locate_Uart_BuildandSendDataPAN(( uint8 * ) &app_LocNodeAlarm, sizeof ( app_LocNodeAlarm ),0,0xFFFF);

        }
        break;
    }
    case GASALARM_ACK:
    {
        app_GasAlarm_ack_t *app_GasAlarm_ack = (app_GasAlarm_ack_t *) (AppPkt);
        if(app_GasAlarm_ack->AlarmType == GASNODE_URGENT)
        {
            if(app_GasAlarm_ack->DevID == zgConfigPANID)
            {
                menu_set_SOSresult(true);
            }
        }
        break;
    }
#ifdef CFG_GAS_SHORTMESSAGE
    case GASDEV_SMS:
    {
        app_gassms_t *papp_gassms = (app_gassms_t *) (AppPkt);
        if(papp_gassms->dstpan != _NIB.nwkPanId && papp_gassms->dstpan != 0xFFFF)
        {
            break;
        }

        uint8 flag;
        flag = GasSms_SaveSms((const char *)papp_gassms->content, papp_gassms->len, papp_gassms->seqnum, papp_gassms->nmbr);
        if(flag == ZSuccess  || flag == GASSMS_STATUES_ALREADY_EXIST)
        {
            /* send ack */
            app_gassms_ack_t app_gassms_ack;
            app_gassms_ack.msgtype = GASDEV_SMS_ACK;
            app_gassms_ack.srcpan = zgConfigPANID;
            app_gassms_ack.dstpan = papp_gassms->srcpan;
            app_gassms_ack.seqnum = papp_gassms->seqnum;
            app_gassms_ack.nmbr = papp_gassms->nmbr;

            MacParam_t param;
            param.panID =app_gassms_ack.dstpan;
            param.cluster_id = GASMONITOR_CLUSTERID;
            param.radius = 0x01;
            //MAC_UTIL_BuildandSendDataPAN ( &param, ( uint8 * ) &app_gassms_ack, sizeof ( app_gassms_ack ),MAC_UTIL_UNICAST, 0 ,  MAC_TXOPTION_ACK);
            Locate_Uart_BuildandSendDataPAN(( uint8 * ) &app_gassms_ack, sizeof ( app_gassms_ack ),0,0xFFFF);

            if(flag == ZSuccess )
            {
                /* start a shake */
                //MOTOR_OPEN();
                //sent MOTOR_OPEN to  JN5148
                if(ZSuccess!= osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_MOTORCLOSE_EVENT, 3000))
                {
                    SystemReset();
                }

                Menu_handle_msg(MSG_NEW_SHORTMSG, NULL, 0);
            }
        }
        else if(flag == GASSMS_STATUES_BUFFER_FULL)
        {
            uint8 flag2;
            flag2 =  GasSms_FillOverFlowSms(papp_gassms->seqnum, papp_gassms->nmbr);
            if(flag2 == ZSuccess)
            {
                //MOTOR_OPEN();
                //sent MOTOR_OPEN to  JN5148
                if(ZSuccess!= osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_MOTORCLOSE_EVENT, 3000))
                {
                    SystemReset();
                }
                Menu_handle_msg(MSG_NEW_SHORTMSG, NULL, 0);
            }
        }
        break;
    }
#endif
    case  CHARGEED_SSREQ:
    {
        app_chargeed_ssReq_t  *app_chargeed_ssReq= (app_chargeed_ssReq_t *) (AppPkt);
        uint16   shortaddr = (uint16)MSGpkt->srcAddr.addr.shortAddr;
        uint8 reqtype = AppPkt->app_ssReq.reqtype;

        if(app_chargeed_ssReq->srcPan==CARD_NWK_ADDR)
        {
            static int8   RSSI;

            RSSI = CONV_LQI_TO_RSSI (MSGpkt->LinkQuality );
            menu_CardInfo_receive(shortaddr,RSSI);

            if(Menu_IsOverDensityed==TRUE&&reqtype==SSREQ_POLL)
            {
                app_chargeed_ssRsp_t Appdata;
                Appdata.msgtype = CHARGEED_SSRSP;
                Appdata.srcPan = CARD_NWK_ADDR;
                Appdata.urgent_type = RETREAT;
                Appdata.urgent_value = 0;


                MacParam_t param;
                param.cluster_id = CHARGEED_CLUSTERID;
                param.panID = CARD_NWK_ADDR;
                param.radius = 1;

                //MAC_UTIL_BuildandSendDataPAN ( &param, ( uint8 * ) &Appdata, sizeof(app_chargeed_ssRsp_t),
                //                              MAC_UTIL_UNICAST, shortaddr, MAC_TXOPTION_NO_CNF );
                Locate_Uart_BuildandSendDataPAN(( uint8 * ) &Appdata, sizeof ( Appdata ),shortaddr,CARD_NWK_ADDR);

            }
        }
        break;
    }

    case   SSREQ:
    {
        uint8 reqtype = AppPkt->app_ssReq.reqtype;
        app_ssReq_t  *app_ssReq = (app_ssReq_t *) (AppPkt);
        uint16  shortaddr = (uint16) MSGpkt->srcAddr.addr.shortAddr;

        if(app_ssReq->NWK_ADDR==CARD_NWK_ADDR)
        {

            static int8   RSSI;

            RSSI = CONV_LQI_TO_RSSI (MSGpkt->LinkQuality );

            menu_CardInfo_receive(shortaddr,RSSI);

            if(Menu_IsOverDensityed==TRUE&&reqtype==SSREQ_POLL)
            {
                app_Urgent_t Appdata;
                Appdata.msgtype = URGENT;
                Appdata.urgenttype = RETREAT;
                Appdata.value = 0;

                MacParam_t param;
                param.cluster_id = CARD_CLUSTERID;
                param.panID = CARD_NWK_ADDR;
                param.radius = 1;

                //MAC_UTIL_BuildandSendDataPAN ( &param, ( uint8 * ) &Appdata, sizeof(app_Urgent_t),
                //                              MAC_UTIL_UNICAST, shortaddr, MAC_TXOPTION_NO_CNF );
                Locate_Uart_BuildandSendDataPAN(( uint8 * ) &Appdata, sizeof ( Appdata ),shortaddr,CARD_NWK_ADDR);
            }
        }
        break;
    }


    }
    return;
}



/*********************************************************************
* @fn      GasMonitor_HandleKeys
*
* @brief   This function is used to handle key event.
*
* @param   keys - key.
*               shifts -
*
* @return  none
*/
void GasMonitor_HandleKeys(void)
{
    uint16 keys = GasMonitor_Keys & ((~P1) & KEY_MASK);

    HalResetBackLightEvent();
    if(keys)
    {
        if(keys & HAL_KEY_POWER) // process power on and off first
        {
            if(GasMonitor_TestLongPress(HAL_KEY_POWER, GASMONITOR_POWER_LONGPRESS_TIMEOUT))
            {
                GasMonitor_RingAndPowerOff(GASNODE_POWEROFF_SHUTDOWN);
            }
            else
            {

                Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);

            }
        }
        else if(GasMonitor_TestLongPress(HAL_KEY_HELP, GASMONITOR_HELP_LONGPRESS_TIMEOUT))
        {
            GasMonitor_SOS_Cnt = 0;
            GasMonitor_SOSAlarm_Cnt=0;
            menu_set_SOSresult(false);

            if(CurrentNodeID== MENU_ID_CARDSEARCH_RESULT
                    ||CurrentNodeID== MENU_ID_CARDSEARCH)
            {
                menu_CardSearch_end();
            }

            osal_stop_timerEx(GasMonitor_TaskID, GASMONITOR_SOSALARM_EVENT);

            if(ZSuccess!=osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_CALLHELP_EVENT, 5))
            {
                SystemReset();
            }
        }
        else
        {
            Menu_handle_key(keys, KEY_SHORT_PRESS);
        }
    }
    return;
}

void GasMonitor_StartMenuLibEvt (uint16 timeout)
{
    osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_MENULIB_EVENT, timeout);
}
void GasMonitor_StopMenuLibEvt (void)
{
    osal_stop_timerEx(GasMonitor_TaskID, GASMONITOR_MENULIB_EVENT);
    osal_unset_event(GasMonitor_TaskID, GASMONITOR_MENULIB_EVENT);
}

void GasMonitor_StartItselfMenuLibEvt (uint16 timeout)
{
    osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_ITSELFMENULIB_EVENT, timeout);
}
void GasMonitor_StopItselfMenuLibEvt(void)
{
    osal_unset_event(GasMonitor_TaskID, GASMONITOR_ITSELFMENULIB_EVENT);
    osal_stop_timerEx(GasMonitor_TaskID, GASMONITOR_ITSELFMENULIB_EVENT);
}

void GasMonitor_RingAndPowerOff(uint8 reason)
{

    osal_stop_timerEx(Hal_TaskID, GASMONITOR_UPDATEGAS_EVENT);  // stop gas ADC
    osal_unset_event(Hal_TaskID, GASMONITOR_UPDATEGAS_EVENT);
    Menu_handle_msg(MSG_POWEROFF_ANIM, NULL, 0);

    GasMonitor_PwrOffReason = reason;
    if(ZSuccess!=osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_POWEROFF_EVENT, 100))
    {
        SystemReset();
    }
}

void GasMonitor_UpdateLocInfo(LocRssiRecord_t* LocNode, const LocPos_t* newnode)
{
    int8 LastCnt = LocNodeCnt;
    if (LastCnt == 0)  // empty
    {
        //LocNode[LastCnt] = *newnode;
        LocNode[LastCnt].LocNode_ID = newnode->LocNode_ID;
        LocNode[LastCnt].meanRSSI = newnode->rssi;
        LocNode[LastCnt].RssiCnt = 1;
        LocNodeCnt++;
    }
    else			//
    {
        int8 pos_sameID = LOCNODENUM;

        for (int8 idx = LastCnt - 1; idx >= 0; --idx) //find if there is a same ID
        {
            if (newnode->LocNode_ID == LocNode[idx].LocNode_ID)
            {
                pos_sameID = idx;
                break;
            }
        }
        if(pos_sameID == LOCNODENUM)  // did not find a record
        {
            if(LastCnt < LOCNODENUM)  // buffer is not full
            {
                //LocNode[LastCnt] = *newnode;
                LocNode[LastCnt].LocNode_ID = newnode->LocNode_ID;
                LocNode[LastCnt].meanRSSI = newnode->rssi;
                LocNode[LastCnt].RssiCnt = 1;
                LocNodeCnt++;
            }
            else						// buffer is full
            {
                int8 pos_minID;
                pos_minID = LOCNODENUM-1;
                for(int8 idx=LOCNODENUM-1; idx>=0; --idx) // find min rssi
                {
                    if (LocNode[idx].meanRSSI< LocNode[pos_minID].meanRSSI)
                    {
                        pos_minID = idx;
                    }
                }
                //LocNode[pos_minID] = *newnode;
                if(newnode->rssi > LocNode[pos_minID].meanRSSI) // if bigger, replace the min node with new node
                {
                    LocNode[pos_minID].LocNode_ID = newnode->LocNode_ID;
                    LocNode[pos_minID].meanRSSI	  = newnode->rssi;
                    LocNode[pos_minID].RssiCnt       = 1;
                }
            }
        }
        else // find a record
        {
            //LocNode[pos_sameID] = *newnode;
            uint8 cnt = LocNode[pos_sameID].RssiCnt;

            /* new meanrssi = (meanrssi*cnt + newrssi)/(cnt+1);*/
            LocNode[pos_sameID].meanRSSI = (LocNode[pos_sameID].meanRSSI*cnt + (float)newnode->rssi)/(cnt+1);
            LocNode[pos_sameID].RssiCnt++;
        }
    }

#if 0
    if (pos_sameID == LOCNODENUM)
    {
        if (LocNodeCnt < LOCNODENUM)//No duplicated ID, and increment count.
        {
            LocNodeCnt++;
        }

        for (idx = LastCnt - 1; idx >= 0; --idx)
        {
            if (newnode->RSSI > LocNode[idx].RSSI)
            {
                if (idx+1 < LOCNODENUM )
                    LocNode[idx+1] = LocNode[idx];
            }
            else if (newnode->RSSI < LocNode[idx].RSSI)
            {
                if (idx+1 < LOCNODENUM )
                {
                    LocNode[idx+1] = *newnode;
                    break;
                }
            }
        }
        if (idx < 0)
            LocNode[idx+1] = *newnode;
    }
    else
    {
        if (newnode->RSSI < LocNode[pos_sameID].RSSI)
        {
            for (idx = pos_sameID+1; idx < LastCnt; ++idx)
            {
                if (newnode->RSSI < LocNode[idx].RSSI)
                    LocNode[idx-1] = LocNode[idx];
                else
                {
                    LocNode[idx-1] = *newnode;
                    break;
                }
            }
            if (idx == LastCnt)
                LocNode[idx-1] = *newnode;
        }
        else if (newnode->RSSI > LocNode[pos_sameID].RSSI)
        {
            for (idx = pos_sameID -1; idx >= 0; --idx )
            {
                if (newnode->RSSI > LocNode[idx].RSSI)
                    LocNode[idx+1] = LocNode[idx];
                else
                {
                    LocNode[idx+1] = *newnode;
                    break;
                }
            }
            if (idx < 0 )
                LocNode[idx+1] = *newnode;
        }
    }
#endif
}
void GasMonitor_LocSortByRSSI(LocRssiRecord_t* LocNode, int8 LastCnt)
{
    if(LastCnt < 2)
    {
        return;
    }
    for(int8 i = 0; i<LastCnt; i++)
    {
        for(int8 j=LastCnt-1; j>i; j--)
        {
            if(LocNode[j-1].meanRSSI< LocNode[j].meanRSSI)
            {
                LocRssiRecord_t loctmp;
                loctmp = LocNode[j-1] ;
                LocNode[j-1] = LocNode[j];
                LocNode[j]    = loctmp;
            }
        }
    }
}
void GasMonitor_ProcessGasDensity(void)
{

    Menu_UpdateGasDensity(GasMonitor_Density);

    /* over density alarm */
    if(GasMonitor_Density >= (menu_get_overalert_density()))
    {
        /* report gas density  */
        //if(osal_start_timerEx(Hal_TaskID, GASMONITOR_GASDENSITYALERT_EVENT, 10)!=ZSuccess)
        //{
        //	SystemReset();
        //}

        /* start beeper */
        if(GasMonitor_Density >=GAS_MAX_DENSITY)
        {
            HalStartBeeper(BEEP_TYPE_OVERALERT,3);
        }
        else if(GasMonitor_Density >=300)
        {
            HalStartBeeper(BEEP_TYPE_OVERALERT,5);
        }
        else if(GasMonitor_Density >= 200)
        {
            HalStartBeeper(BEEP_TYPE_OVERALERT,10);
        }
        else if(GasMonitor_Density >= 100)
        {
            HalStartBeeper(BEEP_TYPE_OVERALERT,20);
        }
        else if(GasMonitor_Density < 100)
        {
            HalStartBeeper(BEEP_TYPE_OVERALERT,30);
        }
        Menu_IsOverDensityed=true;


    }
    else
    {
        HalStopBeeper(BEEP_TYPE_OVERALERT,false);
        Menu_IsOverDensityed=false;
    }
}
uint8 GasMonitor_GasDensityReport(uint16 density)
{
    /* report the gas density */
    app_GasReport_t* app_GasReport = (app_GasReport_t*)osal_mem_alloc(sizeof ( app_GasReport_t)+LocNodeCnt*sizeof(LocPos_t));
    bool Ismemerr = true;
    if (app_GasReport)
    {
        GasMonitor_LocSortByRSSI(LocNodeList, LocNodeCnt);
        app_GasReport->msgtype = GASREPORT;
        app_GasReport->DevID = zgConfigPANID;
        app_GasReport->dstPan = 0xFFFF;
        app_GasReport->GasType = GAS_CH4;
        app_GasReport->GasDensity = density;
        app_GasReport->Type2 = NONE_TYPE;
        app_GasReport->value2 = 0;
        app_GasReport->Type3 = NONE_TYPE;
        app_GasReport->value3 = 0;
        app_GasReport->seq = GasMonitor_seq++;
        app_GasReport->GasThr = menu_get_overalert_density();
        app_GasReport->LocCnt = LocNodeCnt;

        LocPos_t *ptmp=(void *)(app_GasReport+1);
        for(uint8 i=0; i<LocNodeCnt; i++,ptmp++)
        {
            ptmp->LocNode_ID = LocNodeList[i].LocNode_ID;
            ptmp->rssi = (int_8) LocNodeList[i].meanRSSI;
        }
        //osal_memcpy((void *)(app_GasReport + 1), LocNodeList, LocNodeCnt*sizeof(LocPos_t));
        MacParam_t param;
        param.panID = 0xFFFF;
        param.cluster_id = GASMONITOR_CLUSTERID;
        param.radius = 0x01;
        //MAC_UTIL_BuildandSendDataPAN (&param, (uint8* )app_GasReport, sizeof(app_GasReport_t) + LocNodeCnt*sizeof(LocPos_t),
        //                             MAC_UTIL_UNICAST, 0, NULL );
        Locate_Uart_BuildandSendDataPAN(( uint8 * ) &app_GasReport, sizeof ( app_GasReport ),0,0xFFFF);

        osal_mem_free((void *)app_GasReport);
        Ismemerr = false;
    }

    /* clear the loc node RSSi information */
    osal_memset(LocNodeList, 0, LOCNODENUM*sizeof(LocNodeList[0]));
    LocNodeCnt = 0;

    return Ismemerr ? ZMemError : ZSuccess;
}

uint8 GasMonitor_RSSI2Level(int8 RSSI)
{
    uint8 len = sizeof(RSSI_table)/sizeof(RSSI_table[0]);
    uint8 i;

    for(i=0; i<len; i++)
    {
        if(RSSI < RSSI_table[i])
        {
            return i;
        }
    }
    return len;
}
uint16 GasMonitor_GetDensity(void)
{
    return GasMonitor_Density;
}

void GasMonitor_SOSAlarm_startsend(GasAlarmType Alarmtype)
{
    SOSAlarmtype=Alarmtype;

    GasMonitor_SOSAlarm_Cnt=0;

    if(ZSuccess!=osal_start_timerEx(GasMonitor_TaskID, GASMONITOR_SOSALARM_EVENT, 5))
    {
        SystemReset();
    }
}
void GasMonitor_SOSAlarm_endsend(void)
{
    osal_unset_event(GasMonitor_TaskID, GASMONITOR_CALLHELP_EVENT);
    osal_stop_timerEx(GasMonitor_TaskID, GASMONITOR_CALLHELP_EVENT);
    osal_unset_event(GasMonitor_TaskID, GASMONITOR_SOSALARM_EVENT);
    osal_stop_timerEx(GasMonitor_TaskID, GASMONITOR_SOSALARM_EVENT);
    GasMonitor_SOSAlarm_Cnt = 0;
    GasMonitor_SOS_Cnt=0;
    HalStopBeeper(BEEP_TYPE_URGENT, TRUE);
}

/**************************************************************************************************
*
* @fn      GasMonitor_ParseUartCmd
*
* @brief   parse the uart coming data
*
**************************************************************************************************/
Locate_Information_t *MSGpkt;
static void GasMonitor_ParseUartCmd(uint_8* data)
{
    /*CRC doesn't include the sync unit*/
    //uint_16 crc =  CRC16(data, len - 2, 0xFFFF);
    //bool f_CRC = TRUE;
    //uint_8 cmdType;
    //uint_8 querySeq;
    uint8 header[9];
    osal_memcpy(header,data,8);
    Uart_Header_t * pheader = (Uart_Header_t*)header;

#if 0
    if(crc != *((uint_16*)(data + len - 2)))
        f_CRC = FALSE;

    if(!f_CRC)
    {
        //MBUS_send_type = CR_EXCEPTION;
        //osal_start_timerEx(CR_TaskId, CR_UART_WRITE_EVENT, MBUS_FRAME_INTERVAL);
        return;
    }
#endif

    if(uartPortTemp == HAL_UART_PORT_0)
        HAL_TOGGLE_LED1();
    else
        HAL_TOGGLE_LED2();

#if 1
    if(pheader->cmdtype==WIRELESS)
    {
        afIncomingMSGPacket_t *MSGpkt;
        MSGpkt = (afIncomingMSGPacket_t *)(data+12);
        while ( MSGpkt )
        {
            switch ( MSGpkt->hdr.event )
            {
            case AF_INCOMING_MSG_CMD:
            {
                GasMonitor_ProcessMSGCB(MSGpkt);
                break;
            }
            case AF_DATA_CONFIRM_CMD:
            {
                break;
            }
            default:
                break;
            }
        }
    }
    else if(pheader->cmdtype==LOCATE)
    {
        //Locate_Information_t *MSGpkt;
        MSGpkt = (Locate_Information_t *)(data+8);
        Locate_isvalid=false;
        if(MSGpkt->isvalid)
        	{
			Locate_isvalid=true;
			Locate_update_count=0;	
        	}
	 	 else Locate_update_count++;	
	 if(Locate_update_count==3)Locate_update_count=0;			 
        Target_Name=MSGpkt->stationPandID;
        LocateDistance=MSGpkt->Locate_Distance;
        Rate = MSGpkt->success_rate;
        RSSI = MSGpkt->RSSI;
        
	 save_real_locatedistance=MSGpkt->Locate_Distance;	
        if((MENU_ID_LOCATE==CurrentNodeID)&&!Locate_update_count)GasMonitor_StartItselfMenuLibEvt(100);
    }
    //else if(pheader->cmdtype==SYNC_ACK)	
    //{
	//sync_flag=TRUE;
   // }
#endif
}



