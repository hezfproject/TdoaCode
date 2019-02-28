/**************************************************************************************************
Filename:       CR.c
Revised:        $Date: 2011/04/22 18:02:58 $
Revision:       $Revision: 1.9 $

Description:    This file contains the application that can be use to set a device as Location
node from MAC directly which will communicate with a FFD with full z-stack.

**************************************************************************************************/

/**************************************************************************************************
*                                           Includes
**************************************************************************************************/
#include <string.h>

/* Hal Driver includes */
#include "hal_types.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_assert.h"
#include "hal_uart.h"
#include "FlashUtil.h"
#include "MacUtil.h"
/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Nv.h"

/* App Protocol*/
#include "AppProtocolWrapper.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"

/* Application */
#include "SensorReader.h"
#include "MBusProto.h"
#include "CRC.h"
#include "MIC2000_CRC.h"
#include "SensorDefines.h"

/*My Sleep Util*/
#include "SleepUtil.h"

/* watchdog util */
#include "watchdogutil.h"

/*********************************************************************
* TYPEDEFS
*/

typedef struct
{
    sAddrExt_t ExitAddr;
} Dev_Info_t;

typedef enum
{
    SR_REPORT_DATA,
    SR_CLR_RESPOND,
    SR_SET_RESPOND,
    
    SR_EXCEPTION = 0xF0,
} eUartSendType;

/**************************************************************************************************
*                                           Constant
**************************************************************************************************/
#if (MBUS_BAUDRATE == 115200)
#define SR_APP_BAUD  HAL_UART_BR_115200
#endif

#if (MBUS_BAUDRATE == 9600)
#define SR_APP_BAUD  HAL_UART_BR_9600
#endif

// When the Rx buf space is less than this threshold, invoke the Rx callback.
#if !defined( SR_APP_THRESH )
#define SR_APP_THRESH  48
#endif

#if !defined( SR_APP_RX_MAX )
/* The generic safe Rx minimum is 48, but if you know your PC App will not
* continue to send more than a byte after receiving the ~CTS, lower max
* here and safe min in _hal_uart.c to just 8.
*/
#define SR_APP_RX_MAX  250
#endif

#if !defined( SR_APP_TX_MAX )
#define SR_APP_TX_MAX  250
#endif

// Millisecs of idle time after a byte is received before invoking Rx callback.
#if !defined( SR_APP_IDLE )
#define SR_APP_IDLE  6
#endif

#define SR_LED_UART0      HAL_LED_1
#define SR_LED_UART1      HAL_LED_3
#define SR_LED_AIR         HAL_LED_2

/*App Defines*/
#define READ_CMD_LEN (5+1+1+1+2+1) /*5 SYNC*/
#define SET_CMD_HDR_LEN (5+1+1+1+2)
#define SENSOR_DATALEN 171

/* Size table for MAC structures */
const CODE uint_8 SR_cbackSizeTable [] =
{
    0,                                               /* unused */
    sizeof(macMlmeAssociateInd_t),       /* MAC_MLME_ASSOCIATE_IND */
    sizeof(macMlmeAssociateCnf_t),       /* MAC_MLME_ASSOCIATE_CNF */
    sizeof(macMlmeDisassociateInd_t),    /* MAC_MLME_DISASSOCIATE_IND */
    sizeof(macMlmeDisassociateCnf_t),    /* MAC_MLME_DISASSOCIATE_CNF */
    sizeof(macMlmeBeaconNotifyInd_t),    /* MAC_MLME_BEACON_NOTIFY_IND */
    sizeof(macMlmeOrphanInd_t),           /* MAC_MLME_ORPHAN_IND */
    sizeof(macMlmeScanCnf_t),              /* MAC_MLME_SCAN_CNF */
    sizeof(macMlmeStartCnf_t),             /* MAC_MLME_START_CNF */
    sizeof(macMlmeSyncLossInd_t),        /* MAC_MLME_SYNC_LOSS_IND */
    sizeof(macMlmePollCnf_t),                /* MAC_MLME_POLL_CNF */
    sizeof(macMlmeCommStatusInd_t),      /* MAC_MLME_COMM_STATUS_IND */
    sizeof(macMcpsDataCnf_t),            /* MAC_MCPS_DATA_CNF */
    sizeof(macMcpsDataInd_t),            /* MAC_MCPS_DATA_IND */
    sizeof(macMcpsPurgeCnf_t),           /* MAC_MCPS_PURGE_CNF */
    sizeof(macEventHdr_t)                  /* MAC_PWR_ON_CNF */
};

/**************************************************************************************************
*                                        Local Variables
**************************************************************************************************/

/*
Dev number and Extended address of the device.
*/
/* Coordinator and Device information */
//static uint16        SR_CoordShortAddr = SR_SHORTADDR_PANCOORD; /* Initial */
static uint16        SR_DevShortAddr   = 0xFFFF; /* Initial */
static uint8         SR_DeviceID = MBUS_IDMAX;


//static bool     SR_IsBatteryLow   =  FALSE;


/* Task ID */
uint_8 SR_TaskId;

/* Device Info from flash */
static Dev_Info_t SR_DevInfo;

#define UART_BUF_LEN 200

/* UART*/
static uint_8 uartReadBufLen0 = UART_BUF_LEN;
static uint_8 uartReadBuf0[UART_BUF_LEN];
static uint_8 uartReadLen0;

static uint_8 uartReadBufLen1 = UART_BUF_LEN;
static uint_8 uartReadBuf1[UART_BUF_LEN];
static uint_8 uartReadLen1;

/*Sensor data */
static MIC2000_data_t sensorData;

static uint8 sensorReadDataCmd[READ_CMD_LEN+2];
static uint8 sensorReadStatusCmd[READ_CMD_LEN+2];
static uint8 sensorSetCmd[SET_CMD_HDR_LEN+24]; /* only allow set one parameter one time */
static uint8 sensorSetCmdLen = 0;
static uint8 sendSeqNum = 0;
static eUartSendType MBUS_send_type;

static bool isFetchingData = FALSE;
static bool isSettingRegister = FALSE;
static bool isSetSuccess = FALSE;
static bool isFetchingSuccess = TRUE;

static uint8 counter_readFail = 0;


/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
/* Support */
static void         SR_ReadDevInfo();
static void SR_ParseUartCmd(uint_8* data, uint_16 len);
static bool SR_ParseSensorData(uint8* data, uint8 len);


/**************************************************************************************************
*
* @fn          SR_Init
*
* @brief       Initialize the application
*
* @param       taskId - taskId of the task after it was added in the OSAL task queue
*
* @return      none
*
**************************************************************************************************/
void SR_Init(uint_8 taskId)
{       
    halUARTCfg_t uartConfig0;
    halUARTCfg_t uartConfig1;
    /* Initialize the task id */
    SR_TaskId = taskId;


#if(defined WATCHDOG) && (WATCHDOG==TRUE)
	StartWatchDog ( DOGTIMER_INTERVAL_1S );
	osal_set_event ( SR_TaskId, SR_FEEDWATCHDOG_EVENT );
#endif

    SR_ReadDevInfo();
    
    /*init sensor reading uart config*/
    uartConfig0.configured           = TRUE;              // 2430 don't care.
    uartConfig0.baudRate             = HAL_UART_BR_4800;
    uartConfig0.flowControl          = FALSE;
    uartConfig0.flowControlThreshold = SR_APP_THRESH;
    uartConfig0.rx.maxBufSize        = SR_APP_RX_MAX;
    uartConfig0.tx.maxBufSize        = SR_APP_TX_MAX;
    uartConfig0.idleTimeout          = SR_APP_IDLE;   // 2430 don't care.
    uartConfig0.intEnable            = TRUE;              // 2430 don't care.
    uartConfig0.callBackFunc         = NULL;

    /*init MBus uart config*/
    uartConfig1.configured           = TRUE;              // 2430 don't care.
    uartConfig1.baudRate             = SR_APP_BAUD;
    uartConfig1.flowControl          = FALSE;
    uartConfig1.flowControlThreshold = SR_APP_THRESH;
    uartConfig1.rx.maxBufSize        = SR_APP_RX_MAX;
    uartConfig1.tx.maxBufSize        = SR_APP_TX_MAX;
    uartConfig1.idleTimeout          = SR_APP_IDLE;   // 2430 don't care.
    uartConfig1.intEnable            = TRUE;              // 2430 don't care.
    uartConfig1.callBackFunc         = NULL;
    

    /*Sensor port, must use uart0, becasue uart0 is configed as 2 stop bits*/
    HalUARTOpen(HAL_UART_PORT_0, &uartConfig0);

    /*Port ARM, must use uart1*/
    HalUARTOpen(HAL_UART_PORT_1, &uartConfig1);

/*********************************************
*  setup commands here
**********************************************/
    uint8 i;
    for(i=0;i<5;i++)
    {
        sensorReadDataCmd[i]=MIC2000_FRAME_PRILIMINARY;
        sensorReadStatusCmd[i]=MIC2000_FRAME_PRILIMINARY;
    }

    sensorReadDataCmd[5] = MIC2000_COLLECTOR_ADDR;
    sensorReadDataCmd[6] = MIC2000_FRAME_QUERY;  //read cmd
    sensorReadDataCmd[7] = 0x5; //len
    sensorReadDataCmd[8] = (MIC2000_REGADDR_SENSOR_NUM>>8)&0xFF;
    sensorReadDataCmd[9] = MIC2000_REGADDR_SENSOR_NUM & 0xFF;
    sensorReadDataCmd[10] = 166; //total len of all data

    sensorReadStatusCmd[5] = MIC2000_COLLECTOR_ADDR;
    sensorReadStatusCmd[6] = MIC2000_FRAME_QUERY;  //read cmd
    sensorReadStatusCmd[7] = 0x5; //len
    sensorReadStatusCmd[8] = (MIC2000_REGADDR_SENSOR_STATUS>>8)&0xFF;
    sensorReadStatusCmd[9] = MIC2000_REGADDR_SENSOR_STATUS & 0xFF;
    sensorReadStatusCmd[10] = 8; 
    
//////////////////////////////////////////////

    osal_start_timerEx(taskId, SR_UART0_READ_EVENT, 1000);
    osal_start_timerEx(taskId, SR_UART1_READ_EVENT, 1000);
    osal_start_timerEx(taskId, SR_UART0_FETCHDATA_EVENT, 1000);
    osal_start_timerEx(taskId, SR_UART1_WRITE_EVENT, 1000);
    osal_start_timerEx(taskId, SR_LED_CONTROL_EVENT, 2000);
}

/**************************************************************************************************
*
* @fn          SR_ProcessEvent
*
* @brief       This routine handles events
*
* @param       taskId - ID of the application task when it registered with the OSAL
*              events - Events for this task
*
* @return      16bit - Unprocessed events
*
**************************************************************************************************/
uint16 SR_ProcessEvent(uint_8 taskId, uint16 events)
{
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
	if ( events & SR_FEEDWATCHDOG_EVENT )
	{
		osal_start_timerEx ( SR_TaskId, SR_FEEDWATCHDOG_EVENT, 300 );
		FEEDWATCHDOG();
		return events ^ SR_FEEDWATCHDOG_EVENT;
	}
#endif    

    if(events & SR_LED_CONTROL_EVENT)
    {
        HalLedSet(SR_LED_AIR, HAL_LED_MODE_OFF);
        HalLedSet(SR_LED_UART0, HAL_LED_MODE_OFF);
        HalLedSet(SR_LED_UART1, HAL_LED_MODE_OFF);

#ifdef DEBUG_CR
        if(passedSeconds) passedSeconds ++;
#endif
        
        osal_start_timerEx(taskId, SR_LED_CONTROL_EVENT, 2000);
        return events ^ SR_LED_CONTROL_EVENT;
    }

    if(events & SR_RESET_STATUS_EVENT)
    {
        /*do not read correct data in time; still need send response*/
        if(isSettingRegister)
        {
            MBUS_send_type = SR_SET_RESPOND;
            isSetSuccess = FALSE;
            osal_set_event(SR_TaskId, SR_UART1_WRITE_EVENT);
        }        

        /*do not read correct data in time*/
        isFetchingData = FALSE;

        return events ^ SR_RESET_STATUS_EVENT;
    }
    
    if(events & SR_UART0_READ_EVENT)
    {  
        uint_8 len;

        len = HalUARTRead(HAL_UART_PORT_0, uartReadBuf0+uartReadLen0, uartReadBufLen0 - uartReadLen0);

        if(len == 0 && uartReadLen0 == 0)
        {
            osal_start_timerEx(taskId, SR_UART0_READ_EVENT, 100);
            return events ^ SR_UART0_READ_EVENT;
        }

        /*update timer, reset states in 200ms*/
        osal_start_timerEx(taskId, SR_RESET_STATUS_EVENT, 200);

        uartReadLen0 += len;

        /*reading finish*/
        if(len == 0 && uartReadLen0 != 0) 
        {
            if(SR_ParseSensorData(uartReadBuf0, uartReadLen0))
            {
                HAL_TOGGLE_LED1();
            }
            osal_memset(uartReadBuf0, 0, UART_BUF_LEN);
            uartReadLen0 = 0;
        }
        
        osal_start_timerEx(taskId, SR_UART0_READ_EVENT, 100);
        return events ^ SR_UART0_READ_EVENT;
    }

    if(events & SR_UART0_FETCHDATA_EVENT)
    {
        if(isSettingRegister)
        {
            /*try 20ms later*/
            osal_start_timerEx(taskId, SR_UART0_FETCHDATA_EVENT, 20);
            return events ^ SR_UART0_FETCHDATA_EVENT;
        }
        
        uint_16 txAvail = Hal_UART_TxBufLen(HAL_UART_PORT_0);

        if(txAvail != SR_APP_TX_MAX-1) 
        {
            HalUARTFlushTxBuf(HAL_UART_PORT_0);
        }

        /*start fetching*/
        isFetchingData = TRUE;

        /*increase failure counter, if not success*/
        if(!isFetchingSuccess) 
        {
            /*Do not let it back to 0*/
            if(counter_readFail < 255) 
                counter_readFail++;
        }
        else
        {
            counter_readFail = 0;
        }

        /*if keep failing longer than 10s, clear all history data*/
        if(counter_readFail >= 5)
        {
            osal_memset(&sensorData, 0, sizeof(sensorData));
        }
        
        isFetchingSuccess = FALSE;        

        uint16 crc;
        crc = MICcrc(&sensorReadDataCmd[5], READ_CMD_LEN-5);
        sensorReadDataCmd[READ_CMD_LEN] = (crc>>8) & 0xFF;
        sensorReadDataCmd[READ_CMD_LEN+1] = crc & 0xFF;
        
        HalUARTWrite(HAL_UART_PORT_0, sensorReadDataCmd, READ_CMD_LEN+2);

        /*reset status in 1000ms*/
        osal_start_timerEx(taskId, SR_RESET_STATUS_EVENT, 200);

        /*read registers every 2000ms*/
        osal_start_timerEx(taskId, SR_UART0_FETCHDATA_EVENT, 2000);
        return events ^ SR_UART0_FETCHDATA_EVENT;
    }

    if(events & SR_UART0_FETCHSTATUS_EVENT)
    {
        uint_16 txAvail = Hal_UART_TxBufLen(HAL_UART_PORT_0);

        if(txAvail != SR_APP_TX_MAX-1) 
        {
            HalUARTFlushTxBuf(HAL_UART_PORT_0);
        }

        uint16 crc;
        crc = MICcrc(&sensorReadStatusCmd[5], READ_CMD_LEN-5);
        sensorReadStatusCmd[READ_CMD_LEN] = (crc>>8) & 0xFF;
        sensorReadStatusCmd[READ_CMD_LEN+1] = crc & 0xFF;
        
        HalUARTWrite(HAL_UART_PORT_0, sensorReadStatusCmd, READ_CMD_LEN+2);

        /*reset states in 200ms*/
        osal_start_timerEx(taskId, SR_RESET_STATUS_EVENT, 200);
        
        return events ^ SR_UART0_FETCHSTATUS_EVENT;
    }    

    if(events & SR_UART0_SET_EVENT)
    {
        if(isFetchingData)
        {
            osal_start_timerEx(taskId, SR_UART0_SET_EVENT, 20);
            return events ^ SR_UART0_SET_EVENT;
        }
        
        uint_16 txAvail = Hal_UART_TxBufLen(HAL_UART_PORT_0);

        if(txAvail != SR_APP_TX_MAX-1) 
        {
            HalUARTFlushTxBuf(HAL_UART_PORT_0);
        }

        isSettingRegister = TRUE;

        uint16 crc;
        uint8 sync[5] = {0xFE, 0xFE, 0xFE, 0xFE, 0xFE};

        /*
        sensorSetCmd[2] = 5;
        sensorSetCmd[3] = 0x01;
        sensorSetCmd[4] = 0x03;
        sensorSetCmd[5] = 0x8;
        sensorSetCmdLen=6;
        */
        
        crc = MICcrc(sensorSetCmd, sensorSetCmdLen);
        HalUARTWrite(HAL_UART_PORT_0, sync, 5);
        HalUARTWrite(HAL_UART_PORT_0, sensorSetCmd, sensorSetCmdLen);
        HalUARTWrite(HAL_UART_PORT_0, ((uint8*)&crc + 1), 1);
        HalUARTWrite(HAL_UART_PORT_0, ((uint8*)&crc), 1);

        /*reset states in 200ms*/
        osal_start_timerEx(taskId, SR_RESET_STATUS_EVENT, 200);
        
        return events ^ SR_UART0_SET_EVENT;
    }        


    /*MBus*/
    if(events & SR_UART1_READ_EVENT)
    {  
        uint_8 len;
        static bool fIgnore = FALSE;
        mbus_hdr_mstr_t *hdr = (mbus_hdr_mstr_t*) uartReadBuf1;
        
        len = HalUARTRead(HAL_UART_PORT_1, uartReadBuf1+uartReadLen1, uartReadBufLen1 - uartReadLen1);

        uartReadLen1 += len;

        // too long to be a possible cmd from master
        if((uartReadLen1 > sizeof(mbus_hdr_mstr_t)+2 && hdr->cmd != MBUS_CMD_EXECUTE) && len > 0)
        {
            fIgnore = TRUE;
        }

        //continue ignoring till len == 0
        if(fIgnore)
        {
            uartReadLen1 = 0;
        }

        if(len == 0)
        {
            fIgnore = FALSE;            
            HalUARTFlushRxBuf(HAL_UART_PORT_1);
        }
        else{
            asm("nop");
        }
        
#ifdef DEBUG_CR
        else
        {
            readNum++;
        }
#endif

        uint_8 sizet= sizeof(mbus_hdr_mstr_t)+2;

        if((uartReadLen1 == sizet || hdr->cmd == MBUS_CMD_EXECUTE) && len== 0)
        {
            SR_ParseUartCmd(uartReadBuf1, uartReadLen1);
            osal_memset((uint8*)uartReadBuf1, 0, UART_BUF_LEN);
            uartReadLen1 = 0;
        }
        else if(uartReadLen1 > 0 && len == 0)
        {
            uartReadLen1 = 0;
        }
        
        osal_start_timerEx(taskId, SR_UART1_READ_EVENT, MBUS_FRAME_TIMEOUT);
        return events ^ SR_UART1_READ_EVENT;
    }

    if(events & SR_UART1_WRITE_EVENT)
    {
        if(MBUS_send_type == SR_REPORT_DATA)
        {
            uint_16 txAvail = Hal_UART_TxBufLen(HAL_UART_PORT_1);
            static uint_16 sentLen = 0;
            static bool bFirstSend = TRUE;
            static uint_16 crc = 0xFFFF;
            uint8 len=0;

            //First sending but something still left in TX
            if(bFirstSend && txAvail != SR_APP_TX_MAX-1)
            {
                HalUARTFlushTxBuf(HAL_UART_PORT_1);
            }

            if(bFirstSend)
            {
                mbus_hdr_slv_t mbusHead;
                mbus_tlv_t mbusTlv;

                osal_memcpy(mbusHead.sync, MBUS_SYNC, sizeof(mbusHead.sync));
                mbusHead.cmd = MBUS_CMD_RSP;
				
                MBUS_SET_SLAVE_VERSION(mbusHead.frame_control, MBUS_PROTO_VERSION);
                MBUS_SET_SLAVE_SEQ(mbusHead.frame_control, sendSeqNum);
				
                mbusHead.slv_id = SR_DeviceID;
                mbusHead.data_len = sizeof(sensorData)+ sizeof(mbus_tlv_t);

                mbusTlv.len = sizeof(sensorData);
                mbusTlv.type = MBUS_TLV_MIC2000_DATA;                

                crc = CRC16((uint_8*)(&mbusHead), sizeof(mbusHead), crc);
                HalUARTWrite(HAL_UART_PORT_1, ((uint8*)&mbusHead), sizeof(mbusHead));
                txAvail -= sizeof(mbusHead);
                
                crc = CRC16((uint_8*)(&mbusTlv), sizeof(mbusTlv), crc);
                HalUARTWrite(HAL_UART_PORT_1, ((uint8*)&mbusTlv) , sizeof(mbusTlv));
                txAvail -= sizeof(mbusTlv);
                
            }

            if((sizeof(sensorData) - sentLen -2 /*CRC*/ ) > txAvail)
            {
                len = txAvail;
                sentLen += txAvail;                
                bFirstSend = FALSE;

                crc = CRC16((((uint8*)&sensorData) + sentLen), sizeof(len), crc);
                HalUARTWrite(HAL_UART_PORT_1, ((uint8*)&sensorData) + sentLen, len);

                /*txbuf is full filled, wait txbuf to be empty*/
                uint_32 bits = SR_APP_TX_MAX;
                bits*=8*1000;
                osal_start_timerEx(taskId, SR_UART1_WRITE_EVENT, bits/MBUS_BAUDRATE);
            }
            else /*Finish sending*/
            {
                len = (sizeof(sensorData) - sentLen);
                crc = CRC16((((uint8*)&sensorData) + sentLen), len, crc);
                
                HalUARTWrite(HAL_UART_PORT_1, ((uint8*)&sensorData) + sentLen, len);
                HalUARTWrite(HAL_UART_PORT_1, (uint8*)&crc, 2);

                //reset status
                bFirstSend = TRUE;
                sentLen = 0;
                crc = 0xFFFF;
            }

            return events ^ SR_UART1_WRITE_EVENT;
        }

        if(MBUS_send_type == SR_CLR_RESPOND)
        {
            uint_16 txAvail = Hal_UART_TxBufLen(HAL_UART_PORT_1);

            uint_16 crc = 0xFFFF;

            if(txAvail != SR_APP_TX_MAX-1)
            {
                HalUARTFlushTxBuf(HAL_UART_PORT_1);
            }
            
            mbus_hdr_slv_t mbusHead;

            osal_memcpy(mbusHead.sync, MBUS_SYNC, sizeof(mbusHead.sync));
            mbusHead.cmd = MBUS_CMD_RSP;
            MBUS_SET_SLAVE_VERSION(mbusHead.frame_control, MBUS_PROTO_VERSION);
            
            mbusHead.slv_id = SR_DeviceID;
            mbusHead.data_len = 0;

            /*seq is always 0 in this mode*/
            MBUS_SET_SLAVE_SEQ(mbusHead.frame_control, 0);

            crc = CRC16((uint_8*)(&mbusHead), sizeof(mbusHead), crc);
            HalUARTWrite(HAL_UART_PORT_1, (uint_8*)(&mbusHead), sizeof(mbusHead));            
            HalUARTWrite(HAL_UART_PORT_1, (uint_8*)(&crc), sizeof(crc));

            return events ^ SR_UART1_WRITE_EVENT;            
            
        }        

        if(MBUS_send_type == SR_SET_RESPOND)
        {
            uint_16 txAvail = Hal_UART_TxBufLen(HAL_UART_PORT_1);

            uint_16 crc = 0xFFFF;

            if(txAvail != SR_APP_TX_MAX-1)
            {
                HalUARTFlushTxBuf(HAL_UART_PORT_1);
            }
            
            mbus_hdr_slv_t mbusHead;

            osal_memcpy(mbusHead.sync, MBUS_SYNC, sizeof(mbusHead.sync));
            mbusHead.cmd = isSetSuccess? MBUS_CMD_EXECUTE_SUCCESS: MBUS_CMD_EXECUTE_FAIL;
            MBUS_SET_SLAVE_VERSION(mbusHead.frame_control, MBUS_PROTO_VERSION);
            mbusHead.slv_id = SR_DeviceID;
            mbusHead.data_len = 0;

            /*seq is always 0 in this mode*/
            MBUS_SET_SLAVE_SEQ(mbusHead.frame_control, 0);

            crc = CRC16((uint_8*)(&mbusHead), sizeof(mbusHead), crc);
            HalUARTWrite(HAL_UART_PORT_1, (uint_8*)(&mbusHead), sizeof(mbusHead));            
            HalUARTWrite(HAL_UART_PORT_1, (uint_8*)(&crc), sizeof(crc));

            return events ^ SR_UART1_WRITE_EVENT;            
            
        }        
        

        if(MBUS_send_type == SR_EXCEPTION)
        {
            uint_16 crc;
            uint_16 txAvail = Hal_UART_TxBufLen(HAL_UART_PORT_1);            

            HAL_ASSERT(txAvail == SR_APP_TX_MAX -1 );

            mbus_hdr_slv_t mbusHead;
            mbusHead.cmd = MBUS_CMD_EXCEPCTION;
            mbusHead.data_len = 0;
            mbusHead.slv_id = SR_DeviceID;
			
            MBUS_SET_SLAVE_VERSION(mbusHead.frame_control, MBUS_PROTO_VERSION);
            /*seq is always 0 in this mode*/
            MBUS_SET_SLAVE_SEQ(mbusHead.frame_control, 0);

            HalUARTWrite(HAL_UART_PORT_1, (uint_8*)(&mbusHead), sizeof(mbusHead));
            txAvail -= sizeof(mbusHead);
            crc = CRC16((uint_8*)(&mbusHead), sizeof(mbusHead), 0xFFFF);

            HalUARTWrite(HAL_UART_PORT_1, (uint_8*)(&crc), sizeof(crc));
        }
        
        return events ^ SR_UART1_WRITE_EVENT;
    }

    return 0;

}

/**************************************************************************************************
*
* @fn          MAC_CbackEvent
*
* @brief       This callback function sends MAC events to the application.
*              The application must implement this function.  A typical
*              implementation of this function would allocate an OSAL message,
*              copy the event parameters to the message, and send the message
*              to the application's OSAL event handler.  This function may be
*              executed from task or interrupt context and therefore must
*              be reentrant.
*
* @param       pData - Pointer to parameters structure.
*
* @return      None.
*
**************************************************************************************************/
void MAC_CbackEvent(macCbackEvent_t *pData)
{

    macCbackEvent_t *pMsg = NULL;

    uint_8 len = SR_cbackSizeTable[pData->hdr.event];

    switch (pData->hdr.event)
    {
    case MAC_MLME_BEACON_NOTIFY_IND:

        len += sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength +
            MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
        if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            /* Copy data over and pass them up */
            osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
            pMsg->beaconNotifyInd.pPanDesc = (macPanDesc_t *) ((uint_8 *) pMsg + sizeof(macMlmeBeaconNotifyInd_t));
            osal_memcpy(pMsg->beaconNotifyInd.pPanDesc, pData->beaconNotifyInd.pPanDesc, sizeof(macPanDesc_t));
            pMsg->beaconNotifyInd.pSdu = (uint_8 *) (pMsg->beaconNotifyInd.pPanDesc + 1);
            osal_memcpy(pMsg->beaconNotifyInd.pSdu, pData->beaconNotifyInd.pSdu, pData->beaconNotifyInd.sduLength);
        }
        break;

    case MAC_MCPS_DATA_IND:
        pMsg = pData;
        break;
    default:
        if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            osal_memcpy(pMsg, pData, len);
        }
        break;
    }

    if (pMsg != NULL)
    {
        osal_msg_send(SR_TaskId, (byte *) pMsg);
    }
}

/**************************************************************************************************
*
* @fn      MAC_CbackCheckPending
*
* @brief   Returns the number of indirect messages pending in the application
*
* @param   None
*
* @return  Number of indirect messages in the application
*
**************************************************************************************************/
uint_8 MAC_CbackCheckPending(void)
{
    return (0);
}

void SR_ReadDevInfo()
{
#if 0
    /* Make a fake DEV Info*/
    SR_DevInfo.ExitAddr[0] = 0x00;
    SR_DevInfo.ExitAddr[1] = 0xFF;
    SR_DevInfo.ExitAddr[2] = 0xFF;
    SR_DevInfo.ExitAddr[3] = 0xFF;
    SR_DevInfo.ExitAddr[4] = 0x00;
    SR_DevInfo.ExitAddr[5] = 0x00;
    SR_DevInfo.ExitAddr[6] = 0xFF;
    SR_DevInfo.ExitAddr[7] = 0x00;
#else

    uint8 aExtendedAddress[8];    

    // Initialize NV System
    osal_nv_init( NULL );

    // Initialize extended address in NV
    osal_nv_item_init( ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL );
    osal_nv_read( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, &aExtendedAddress);
        
    Dev_Info_t* p = (Dev_Info_t *)(&aExtendedAddress);
    SR_DevInfo = *p;
    
    //SR_version = SR_DevInfo.ExitAddr[EXT_MACADDR_VERSION];
    SR_DevShortAddr = BUILD_UINT16(SR_DevInfo.ExitAddr[EXT_MACADDR_DEVID_LBYTE],SR_DevInfo.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);
    HAL_ASSERT(SR_DevShortAddr > 31000 && SR_DevShortAddr < 31255);
    SR_DeviceID = SR_DevShortAddr - 31000;

    HAL_ASSERT(SR_DevInfo.ExitAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_SENSORREADER);

#endif

}


/**************************************************************************************************
*
* @fn      SR_ParseUartCmd
*
* @brief   parse the uart coming data
*
**************************************************************************************************/
static void SR_ParseUartCmd(uint_8* data, uint_16 len)
{
    /*CRC doesn't include the sync unit*/
    uint_16 crc =  CRC16(data, len-2, 0xFFFF);
    bool f_CRC=TRUE;
    uint_8 cmdType;
    uint_8 querySeq;
    mbus_hdr_mstr_t *hdr =  ((mbus_hdr_mstr_t*)data);

    if(crc != *((uint_16*)(data+len-2))) 
        f_CRC=FALSE;    

    if(!f_CRC)
    {
        //MBUS_send_type = SR_EXCEPTION;
        //osal_start_timerEx(SR_TaskId, SR_UART_WRITE_EVENT, MBUS_FRAME_INTERVAL);
        return;
    }
    
    HAL_TOGGLE_LED2();

    if(((mbus_hdr_mstr_t*)data)->slv_id != SR_DeviceID) return;
    if(MBUS_GET_MASTER_VERSION(((mbus_hdr_mstr_t*)data)->frame_control) < MBUS_PROTO_VERSION) return;

    cmdType = hdr->cmd;
    querySeq = MBUS_GET_MASTER_SEQ(hdr->frame_control); 
    
    switch(cmdType)
    {
        case(MBUS_CMD_CLR):
        {
            sendSeqNum = 0;
            MBUS_send_type = SR_CLR_RESPOND;
            osal_start_timerEx(SR_TaskId, SR_UART1_WRITE_EVENT, MBUS_FRAME_INTERVAL);
            break;
        }

        case(MBUS_CMD_QRY):
        {                
            MBUS_send_type = SR_REPORT_DATA;
            sendSeqNum = querySeq;
            osal_start_timerEx(SR_TaskId, SR_UART1_WRITE_EVENT, 40);
            break;
        }

        case(MBUS_CMD_EXECUTE):
        {
            mbus_tlv_t* tlv = (mbus_tlv_t*)( hdr+1);
            if(MBUS_GET_MASTER_SEQ(hdr->frame_control) && tlv->type == MBUS_TLV_MIC2000_SET && tlv->len <= SET_CMD_HDR_LEN +24)
            {
                osal_start_timerEx(SR_TaskId, SR_UART0_SET_EVENT, 10);
                osal_memcpy((uint8*)sensorSetCmd, (uint8*)(tlv+1), tlv->len);
                sensorSetCmdLen = tlv->len;                    
            }
        }

        default:
            break;
    }
    
}

static bool SR_DataRemoveHeader(uint8* data, uint8 len, uint8* index)
{
    while(data[*index] != MIC2000_FRAME_PRILIMINARY)
    {
        *index+=1;
        if(*index == len) return FALSE;        
    }

    while(data[*index] == MIC2000_FRAME_PRILIMINARY)
    {
        *index+=1;
        if(*index == len) return FALSE;                
    }    

    if(len - *index < 7) return FALSE;

    return TRUE;
}


static bool SR_ParseSensorData(uint8* data, uint8 len)
{
    uint8 index=0;
    
    while(SR_DataRemoveHeader(data, len, &index))
    {
        //header has been removed    
        uint8 datalen;
        uint8 cmd = data[index+1];
        static uint16 crc;
        

        // remove command itself 
        if(cmd == MIC2000_FRAME_QUERY || cmd == MIC2000_FRAME_SET) continue;

        if(cmd == MIC2000_FRAME_QUERY_SUCCESS && isFetchingData)
        {
            /* datalen is length of command+data*/
            datalen= data[index+2]+ 1/*Address byte*/;
            if(datalen != len -index -2/*2-byte crc*/) return FALSE;

            /*crc is for command+data*/
            crc = MICcrc(&(data[index]), datalen);
            if((uint8)(crc&0xFF) != data[len-1] || (uint8)((crc&0xFF00) >>8) != data[len-2]) return FALSE;

            if(datalen == SENSOR_DATALEN) /*fetch data*/
            {
                sensorData.sensor_num = data[index+5]; /*addr : 0x103*/
                osal_memcpy(&sensorData.density, &data[index+10], (unsigned int)(&sensorData+1) - (unsigned int)&sensorData.density);

                /*fetch status 200ms later*/
                osal_start_timerEx(SR_TaskId, SR_UART0_FETCHSTATUS_EVENT, 50);
            }
            else if(datalen == 13)
            {
                osal_memcpy(&sensorData.sensor_status, &data[index+5], 8);
                
                /*data fetching is done*/
                isFetchingData = FALSE;

                /*Set success, then failure counter would not increase*/
                isFetchingSuccess = TRUE;
            }
            /*if len doesn't match, return FALSE*/
            else
            {
                /*data fetching is done*/
                isFetchingData = FALSE;
                return FALSE;
            }

            return TRUE;
        }

        if(cmd == MIC2000_FRAME_SET_SUCCESS && isSettingRegister)
        {
            /* datalen is length of command+data*/
            datalen= 6;
            if(len - index -2 != 6/*2-byte crc*/) return FALSE;

            /*crc is for command+data*/
            crc = MICcrc(&(data[index]), 6);
            if(crc&0xFF != data[len-1] || ((crc&0xFF00) >>8) != data[len-2]) return FALSE;

            MBUS_send_type = SR_SET_RESPOND;
            isSetSuccess = TRUE;
            osal_start_timerEx(SR_TaskId, SR_UART1_WRITE_EVENT, 10);

            isSettingRegister = FALSE;

            return TRUE;
        }
        else if(cmd == MIC2000_FRAME_SET_ERROR && isSettingRegister)
        {
            MBUS_send_type = SR_SET_RESPOND;
            isSetSuccess = FALSE;
            osal_start_timerEx(SR_TaskId, SR_UART1_WRITE_EVENT, 10);

            isSettingRegister = FALSE;
        }
    }

    return FALSE;    
}


/**************************************************************************************************
**************************************************************************************************/

