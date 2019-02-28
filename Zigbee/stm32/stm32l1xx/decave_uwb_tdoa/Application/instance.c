// -------------------------------------------------------------------------------------------------------------------
//
//  File: instance.c - application level message exchange for ranging demo
//
//  Copyright 2008 (c) DecaWave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author: Billy Verso, December 2008
//
// -------------------------------------------------------------------------------------------------------------------

#include "compiler.h"
#include "port.h"
#include "deca_device_api.h"
#include "deca_spi.h"
#include "printf_util.h"
#include "instance.h"
#include "timer_event.h"
#include "config.h"
#include "Application.h"
#include "bsmac_header.h"
#include "bsmac.h"
// -------------------------------------------------------------------------------------------------------------------


TDOA_INSTANCE_DATA_S gstTdoaInstData;
TDOA_INST_RXMSG_TO_CARDMSG_S gstTdoaCardMsg;
TDOA_INST_BETTER_SELECT_TABLE_S gstBetterTable;      //���⿨ѡ�����ŵ�ѧϰ�������������
TDOA_INST_SAVE_RSSI_MSG_S       gstTdoaSaveRssiMsg;  //ѧϰ�����д洢���յ��Ĵ��⿨������ ����������Ϣʱ���˲�����Ϣһ��������͸���վ
TDOA_INST_STANDARD_CARD_MSG_S   gstTdoaStdCardMsg;   //��վ���д洢��ȡ���ѧϰ��������⿨�Ĺ�ϵ �ɴ��ж�ʹ���ĸ�ѧϰ���Դ��⿨�������
TDOA_UWB_TIMESTAMP_MSG_ARRAY_S  gstTdoaMsgArray[TDOA_STANDCARD_MAX_NUM];
extern TDOA_UWB_MSG_PACK_SEND_S gTdoaSendPack;

uint8 gu8NblinkPack = 0;
uint8 gu8NblinkSend = 0;
uint8 gu8Dealtime = 0;

//The table below specifies the default TX spectrum configuration parameters... this has been tuned for DW EVK hardware units
const TDOA_INST_SPECTRUM_TX_CONFIG_S gstSpectrumTxConfig[8] =
{
    //Channel 0 ----- this is just a place holder so the next array element is channel 1
    {
            0x0,   //0
            {
                    0x0, //0
                    0x0 //0
            }
    },
    //Channel 1
    {
            0xc9,   //PG_DELAY
            {
                    0x75757575, //16M prf power
                    0x67676767 //64M prf power
            }

    },
    //Channel 2
    {
            0xc2,   //PG_DELAY
            {
                    0x75757575, //16M prf power
                    0x67676767 //64M prf power
            }
    },
    //Channel 3
    {
            0xc5,   //PG_DELAY
            {
                    0x6f6f6f6f, //16M prf power
                    0x8b8b8b8b //64M prf power
            }
    },
    //Channel 4
    {
            0x95,   //PG_DELAY
            {
                    0x5f5f5f5f, //16M prf power
                    0x9a9a9a9a //64M prf power
            }
    },
    //Channel 5
    {
            0xc0,   //PG_DELAY
            {
                    0x48484848, //16M prf power
                    0x85858585 //64M prf power
            }
    },
    //Channel 6 ----- this is just a place holder so the next array element is channel 7
    {
            0x0,   //0
            {
                    0x0, //0
                    0x0 //0
            }
    },
    //Channel 7
    {
            0x93,   //PG_DELAY
            {
                    0x92929292, //16M prf power
                    0xd1d1d1d1 //64M prf power
            }
    }
};

//these are default antenna delays for EVB1000, these can be used if there is no calibration data in the DW1000,
//or instead of the calibration data
const uint16 gau16RfDelays[2] = {
        (uint16) ((DWT_PRF_16M_RFDLY/ 2.0) * 1e-9 / DWT_TIME_UNITS),//PRF 16
        (uint16) ((DWT_PRF_64M_RFDLY/ 2.0) * 1e-9 / DWT_TIME_UNITS)
};

uint16 gu16InstanceTypeRecv = 0;
// -------------------------------------------------------------------------------------------------------------------
//      Data Definitions
// -------------------------------------------------------------------------------------------------------------------

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// NOTE: the maximum RX timeout is ~ 65ms
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// -------------------------------------------------------------------------------------------------------------------


TDOA_INST_CHANNEL_CONFIG_S gstChConfig[TDOA_CHANNEL_CONFIG_MODE_LEN] ={
//mode 1 - S1: 7 off, 6 off, 5 off
{
	3,              // channel  //����ʹ��1ͨ�� TDOAʹ��3ͨ��
	DWT_PRF_16M,    // prf
	DWT_BR_110K,//DWT_BR_6M8,    // datarate
	3,             // preambleCode
	DWT_PLEN_128,	// preambleLength
	DWT_PAC32,		// pacSize
	1		// non-standard SFD
},

 //mode 2
{
	2,              // channel
	DWT_PRF_16M,    // prf
	DWT_BR_6M8,    // datarate
	3,             // preambleCode
	DWT_PLEN_128,	// preambleLength
	DWT_PAC8,		// pacSize
	0		// non-standard SFD
},
//mode 3
{
	2,              // channel
	DWT_PRF_64M,    // prf
	DWT_BR_110K,    // datarate
	9,             // preambleCode
	DWT_PLEN_1024,	// preambleLength
	DWT_PAC32,		// pacSize
	1		// non-standard SFD
},

//mode 4
{
	2,              // channel
	DWT_PRF_64M,    // prf
	DWT_BR_6M8,    // datarate
	9,             // preambleCode
	DWT_PLEN_128,	// preambleLength
	DWT_PAC8,		// pacSize
	0		// non-standard SFD
},

//mode 5
{
	5,              // channel
	DWT_PRF_16M,    // prf
	DWT_BR_110K,    // datarate
	3,             // preambleCode
	DWT_PLEN_1024,	// preambleLength
	DWT_PAC32,		// pacSize
	1		// non-standard SFD
},

{
	5,              // channel
	DWT_PRF_16M,    // prf
	DWT_BR_110K,    // datarate
	3,             // preambleCode
	DWT_PLEN_64,	// preambleLength
	DWT_PAC32,		// pacSize
	1		// non-standard SFD
},

{
	5,              // channel
	DWT_PRF_16M,    // prf
	DWT_BR_6M8,    // datarate
	3,             // preambleCode
	DWT_PLEN_128,	// preambleLength
	DWT_PAC8,		// pacSize
	0		// non-standard SFD
},

//mode 7
{
	5,              // channel
	DWT_PRF_64M,    // prf
	DWT_BR_110K,    // datarate
	9,             // preambleCode
	DWT_PLEN_1024,	// preambleLength
	DWT_PAC32,		// pacSize
	1		// non-standard SFD
},

//mode 8
{
	5,              // channel
	DWT_PRF_64M,    // prf
	DWT_BR_6M8,    // datarate
	9,             // preambleCode
	DWT_PLEN_128,	// preambleLength
	DWT_PAC8,		// pacSize
	0		// non-standard SFD
}
};

// -------------------------------------------------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------------------------------------------------
#if IF_DESC(ע:�ṹ�庯�������ö���)//"�ṹ�庯�������ö���"
/*************************************************************
��������:TdoaGetLocalInstStructurePtr
��������:��ȡʵ��ȫ�ֽṹ����
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
TDOA_INSTANCE_DATA_S* TdoaGetLocalInstStructurePtr(void)
{
	return &gstTdoaInstData;
}

/*************************************************************
��������:TdoaGetLocalCardRssiStructurePtr
��������:��ȡѧϰ����ȡ���⿨��������ȫ�ֽṹ����
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
TDOA_INST_SAVE_RSSI_MSG_S* TdoaGetLocalCardRssiStructurePtr(void)
{
	return &gstTdoaSaveRssiMsg;
}

/*************************************************************
��������:TdoaGetLocalInstStructurePtr
��������:��ȡ��ȫ����Ϣ����
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
TDOA_INST_RXMSG_TO_CARDMSG_S* TdoaGetLocalCardStructurePtr(void)
{
	return &gstTdoaCardMsg;
}

/*************************************************************
��������:TdoaGetLocalInstStructurePtr
��������:��ȡTdoa���ȫ����Ϣ����
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* TdoaGetLocalMsgArrayStructurePtr(void)
{
//	return gstTdoaMsgArray;
}

/*************************************************************
��������:TdoaGetLocalInstStructurePtr
��������:��ȡTdoa���Ͱ�������
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
TDOA_UWB_MSG_PACK_SEND_S* TdoaGetLocalSendPackStructurePtr(void)
{
	return &gTdoaSendPack;
}

/*************************************************************
��������:TdoaGetLocalStdCardMsgStructurePtr
��������:��ȡѧϰ���Ѿ���Ӧ�Ĵ��⿨����Ϣ ��Ҫʱ�ṩ����վ���д��⿨���ʱѡ���Ӧ��ѧϰ��
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
TDOA_INST_STANDARD_CARD_MSG_S* TdoaGetLocalStdCardMsgStructurePtr(void)
{
	return &gstTdoaStdCardMsg;
}

/*************************************************************
��������:TdoaGetLocalBetterTableStructurePtr
��������:�γɴ��⿨ѡ�����ŵ�ѧϰ����ϵ��
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
TDOA_INST_BETTER_SELECT_TABLE_S* TdoaGetLocalBetterTableStructurePtr(void)
{
	return &gstBetterTable;
}

#endif

#if IF_DESC(ע:TDOA���ݷ��ͺͽ���)
/*************************************************************
��������:TDOATxCallback
��������:���ͻص����� ��ʱ����ע��ֻ��Ԥ��
		 �жϺ���: dwt_isr	
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TDOATxCallback(const dwt_callback_data_t *txd)
{
/*
	uint32 systicks3 = portGetTickCnt();

	// PrintfUtil_vPrintf("Tx end systicks = %d \n", systicks);
	uint16 u16SeqnumData;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
		
	u16SeqnumData = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_H] << 8; 	  //get the seqnum
	u16SeqnumData += pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_L];

	PrintfUtil_vPrintf("txcb %d %d\n", pstInstMsg->stInstBaseData.u16SeqNum, systicks3);
*/

	//���ͻص�������ʱ����ע ����Ҫ����
	return;
}

/*************************************************************
��������:TDOARxTimeoutCallback
��������:���ճ�ʱ�ص����� ��ʱ����ע��ֻ��Ԥ��
         �жϺ���: dwt_isr
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TDOARxTimeoutCallback(const dwt_callback_data_t *rxd)
{
	return;
}

/*************************************************************
��������:TDOARxErrorCallback
��������:���մ���ص����� ��ʱ����ע��ֻ��Ԥ��		 
		 �жϺ���: dwt_isr
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TDOARxErrorCallback(const dwt_callback_data_t *rxd)
{
	return;
}

/*************************************************************
��������:TDOARxGoodCallback
��������:��ȷ���ջص�����
		 �жϺ���: dwt_isr
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TDOARxGoodCallback(const dwt_callback_data_t *pstRxCallBackData)
{
    uint8 u8RxEvent = 0;
	uint8 u8SpeedType = INST_MODE_TX_SPEED_BUTT; //���ֻ���տ췢(1)������(0)����Ϣ
	uint8 u8DelayTime = 0;
	uint8 au8RxTimeStamp[5]  = {0, 0, 0, 0, 0};
	uint16 u16SrcAddr = 0;
    uint16 u16Seqnum = 0;
    uint32 u32RxTimeStampTemp = 0;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
		
	//if we got a frame with a good CRC - RX OK
	if(pstRxCallBackData->event == DWT_SIG_RX_OKAY)
	{
		//at the moment using length and 1st byte to distinguish between different fame types and "blinks".
		switch(pstRxCallBackData->datalength)
		{
			case SIG_RX_ACK:
				u8RxEvent = SIG_RX_ACK;
				break;
 
			default:
				u8RxEvent = DWT_SIG_RX_OKAY;
				break;
		}
		
		if(u8RxEvent == DWT_SIG_RX_OKAY)
		{	
			dwt_readrxtimestamp(au8RxTimeStamp) ;
			u32RxTimeStampTemp =	au8RxTimeStamp[0] + (au8RxTimeStamp[1] << 8) + (au8RxTimeStamp[2] << 16) + (au8RxTimeStamp[3] << 24);
			pstInstMsg->stInstBaseData.u64RxTimeStamp = au8RxTimeStamp[4];
			pstInstMsg->stInstBaseData.u64RxTimeStamp <<= 32;
			pstInstMsg->stInstBaseData.u64RxTimeStamp += u32RxTimeStampTemp;
			pstInstMsg->u16RxMsgLength = pstRxCallBackData->datalength; //��׼��ܵ����ݳ���Ϊ127byte
			pstInstMsg->stInstBaseData.eTdoaInstState = TDOA_RX_WAIT_DATA;
			pstInstMsg->stInstBaseData.eDwEventType = u8RxEvent;
			pstInstMsg->stInstBaseData.u8InstEvent[0] = DWT_SIG_RX_OKAY;

			//��ȡ����֡���� ��mac֡���ݷ�װ����TdoaSetMacFrameData��u8MessageData[TDOA_INST_FRAME_TYPE_BIT]��Ϊ����֡����
			//���㷽��Ϊ �����ܳ���Ϊ 0-8byteΪ��������洢λ�� 9byte��ʼ��Ϊ6byte�����������飬����9�ֽ�Ϊ����֡����
			dwt_readrxdata((uint8 *)&pstInstMsg->stRxMsgFromDw, pstRxCallBackData->datalength, 0);  // Read Data Frame		

			//��¼��ǰ�¼����� DWT_SIG_RX_OKAY
		}
		
		//�����Ѿ���¼��ʵ�����ݽṹ�� 
		//���ݰ���: ���մ��⿨\�췢����ʱ�������������������ȡ������(����֡���͡�������Դ��ַ�����кš�������Դ����������)
	}
	
	u8SpeedType = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SPEED_TYPE_BIT];
	u16Seqnum = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_H] << 8;		  //get the seqnum
	u16Seqnum += pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_L];
    
	memcpy(&u16SrcAddr, &(pstInstMsg->stRxMsgFromDw.u8SourceAddr), ADDR_BYTE_SIZE_S);		
	u16SrcAddr	= u16SrcAddr & 0xffff;

    //PrintfUtil_vPrintf("%d, %d %d \n",u16SrcAddr, u16Seqnum, pstInstMsg->stRxMsgFromDw.u8MessageData[0]);
	//ֻ����TDOA blink���͵İ� ֻ���տ췢���������͵İ� ֻ����ָ���췢������������Ϣ��
	if ((pstInstMsg->stRxMsgFromDw.u8MessageData[0] == RTLS_TDOA_BLINK_SEND )&& 
		((u8SpeedType == INST_MODE_TX_SPEED_SLOW) || (u8SpeedType == INST_MODE_TX_SPEED_QUICK)))                                                                                                                                                                                
	{	
		gu8NblinkPack = 0;
		gu8NblinkSend = 0;
		event_timer_set(EVENT_RAGING_REPORT);
	    //PrintfUtil_vPrintf("%d, %d\n",u16SrcAddr, u16Seqnum);
	}
	else
	{
		//����ic����ȥ���������ջ���������
		dwt_sethsrb();
		if (gu8NblinkSend == 0)
		{
			//��blnik����blink���ѷ�����ֱ�ӿ�������
			dwt_rxenable(0);
		}
		else 
		{
			//��Ϊ���⿨������һ�������˯��ʱ�䣬��ֹ���ڴ��⿨̫�ർ�·��ͳ�ͻ
			u8DelayTime = rand()%10 + 20;
			//��blnik��������bilnk��δ������ʱ10ms�������� ��ʱ10ms��Ϊ�˵ȴ���Ϣ���������
			dwt_rxenable(u8DelayTime);
		}
		
		gu8NblinkPack = 1;
	}

	return;
}

/*************************************************************
��������:TdoaRxCardMsgProc
��������:��վ���յ���ǩ��Ϣ����н�����Ϣ�ļ����������
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaRxCardMsgProc(void)
{	
	uint8 u8StandCardLoop = 0; //���������Ϣ�ṹ�����еĿ췢������

	TDOA_INST_RXMSG_TO_CARDMSG_S* pstCardRxMsg = TdoaGetLocalCardStructurePtr();

	//�����豸�����ʷֱ���д���
	if (pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_QUICK)	//quick
	{
		//�췢���������
		TdoaRxQuickCardMsgProc(pstCardRxMsg);
	}
	else if (pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_SLOW)
	{
		//���� ���⿨�������
		TdoaRxSlowCardMsgProc(pstCardRxMsg);
	}
	else
	{		
		PrintfUtil_vPrintf("the TdoaRxCardMsgProc tag %d Speedtype was wrong u16Speedtype = %d\n", pstCardRxMsg->u16CardId, pstCardRxMsg->u16Speedtype);
        //���tdoa���Ѿ������˴������Բ���Ҫ������ʱ����
        gu8NblinkSend = 1;
        //���ǽ������ʹ�������д�������¼� �����˳����ݼ��
        event_timer_unset(EVENT_RAGING_REPORT);
        event_timer_set(EVENT_RAGING_REPORT);
        return;
	}
    #if 0
	for (u8StandCardLoop = 0; u8StandCardLoop < TDOA_STANDCARD_MAX_NUM; u8StandCardLoop++) //�������еĿ췢���γɵ����ݰ�
	{
    	PrintfUtil_vPrintf("%d %d %d %d %d %d\n", u8StandCardLoop,
                                     gstTdoaMsgArray[u8StandCardLoop].stPreQuickCardMsg.u16CardId,
                                     gstTdoaMsgArray[u8StandCardLoop].stLastQuickCardMsg.u16CardId,
                                     gstTdoaMsgArray[u8StandCardLoop].stTdoaMsg[0].u16TestCardID,
                                     gstTdoaMsgArray[u8StandCardLoop].u16TdoaMsgNum,//���⿨����
                                     gstTdoaMsgArray[u8StandCardLoop].u16MsgSendFlag);
    }
    #endif
    
	/*********************	������͹���   ************************ 
	��Ϊ TdoaCardMsdBuildUnity �����n ��С��һ��ÿ�γ�һ��������ÿ�η���Ƶ��̫�� 
	����n̫С���˷ѣ�������ϰ����ֺ��鷳������nֵ���ܱ仯�ϴ� ��	
	*********************  ������͹���   ************************/
	
	//ע�⣺��һ��ʼ��С��û�ﵽ25�����涼�Ǵ���Լ����ͳ�ȥ��������С������ 
	/********************  ��ʱ�ͷ�msg_arr[k] ***************************/	
	for (u8StandCardLoop = 0; u8StandCardLoop < TDOA_STANDCARD_MAX_NUM; u8StandCardLoop++) //�������еĿ췢���γɵ����ݰ�
	{
		if (gstTdoaMsgArray[u8StandCardLoop].u16MsgSendFlag == TRUE) 		//���Է��Ͳ���� 
		{
			TdoaMsgPackInsetUartBuff(&gstTdoaMsgArray[u8StandCardLoop]);
		}
	}

	return;
}

/*************************************************************
��������:TdoaSetMacFrameData
��������:�����¼���Ϣ֡ͷ��������
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaInstConfigFrameHeader(TDOA_INSTANCE_DATA_S *pstInstMsg, int iIfAckRequst)
{	 	
    uint16 u16PanId = 0;
    
    //���豸Ϊ���⿨����Ҫ�����ݷ�����վ��ѧϰ�� ѧϰ����Ҫ������վ ��վ��ʱ����Ҫ�������ݷ�����ʱ�������趨
    if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_TEST)
    {
        u16PanId = TDOA_INST_PANID_ANCHOR; //��վ��panid��ѧϰ����panid��ͬ ֻ��Ҫ��������Ϊ��վ��panid
    }
    else if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_STANDARD)
    {
        u16PanId = TDOA_INST_PANID_ANCHOR; //��վ��panid 
    }
    
	//���÷���֡��panid �����豸����Ҫ��ͬһpanid�����й���
	pstInstMsg->stTxMsgToPack.u8PanID[0] = (u16PanId) & 0xff;  //main station
	pstInstMsg->stTxMsgToPack.u8PanID[1] = (u16PanId >> 8) & 0xff;	

	//����֡���� frame type (0-2), SEC (3), Pending (4), ACK (5), PanIDcomp(6)
	pstInstMsg->stTxMsgToPack.u8FrameCtrl[0] = 0x1 /*frame type 0x1 == data*/ | 0x40 /*PID comp*/;
	pstInstMsg->stTxMsgToPack.u8FrameCtrl[0] |= (iIfAckRequst ? 0x20 : 0x00);

	//���е�ַ���ͱ�ʶ �˴�Ϊ�̵�ַ
    pstInstMsg->stTxMsgToPack.u8FrameCtrl[1] = 0x8 /*dest short address (16bits)*/ | 0x80 /*src short address (16bits)*/;

	//�����豸��Ŀ�ĵ�ַ��Դ��ַ
	memcpy(&pstInstMsg->stTxMsgToPack.u8DestAddr, &pstInstMsg->stInstBaseData.u16DestAddress, ADDR_BYTE_SIZE_S);
	memcpy(&pstInstMsg->stTxMsgToPack.u8SourceAddr, &pstInstMsg->stInstBaseData.u16OwnAddress, ADDR_BYTE_SIZE_S);

	return;
}

/*************************************************************
��������:TdoaSetMacFrameData
��������:�������ݷ���ǰ����mac֡���ݺ���������
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaSetMacFrameData(TDOA_INSTANCE_DATA_S *pstInstMsg, int iFrameDataLen, int iFrameType, int iIfAck)
{

	//��¼��������֡���� ��ctrl[0]��Ӧ
	pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_TYPE_BIT] = iFrameType;
	//��¼���ͻ��������� �ɱ䳤�������ݳ��� + ֡�����Լ�Ŀ�ĵ�ַ��Դ��ַ���� + ֡У�鳤��
	pstInstMsg->u16TxMsgLength = iFrameDataLen + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC;

	//�����¼�������Ϣ֡ͷ
    TdoaInstConfigFrameHeader(pstInstMsg, iIfAck); //set up frame header (with/without ack request)

	//������Ϣȷ�ϵȴ�ʱ��
    if(iIfAck == TDOA_INST_ACK_REQUESTED)
	{
        pstInstMsg->u8WaitAck = DWT_RESPONSE_EXPECTED;
	}
	//��¼��ǰ�¼���Ϣ�Ƿ���Ҫ�ȴ���Ӧ
  //  pstInstMsg->u8AckExpected = iIfAck ; //used to ignore unexpected ACK frames

	return;
}

/*************************************************************
��������:TdoaSendTagPoll
��������:���⿨�Ϳ췢������ѯ��Ϣ����֡��װ��ͨ��dw1000����������
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
int TdoaInstSendTagPoolPacket(TDOA_INSTANCE_DATA_S *pstInstMsg, int iTxDelayed)
{
    int result = 0;

	//������������д�뵽�����������������ڷ������Ĵ�����¼���ͳ�����ΪУ�鴦��
    dwt_writetxdata(pstInstMsg->u16TxMsgLength, (uint8 *)&pstInstMsg->stTxMsgToPack, 0) ;   // write the frame data
    dwt_writetxfctrl(pstInstMsg->u16TxMsgLength, 0);//�����в��
	
	//�ж��Ƿ���Ҫ������ʱ���� ���͹��̲�������ʱ����
    if(iTxDelayed)
    {
        PrintfUtil_vPrintf("TdoaInstSendTagPoolPacket dwt_setdelayedtrxtime \n");
        dwt_setdelayedtrxtime(iTxDelayed) ;
    }
	
	//�ж���Ϣ���ͺ��Ƿ���Ҫ���еȴ���Ϣȷ�� poll��Ϣ��������Ϣȷ��
    if(pstInstMsg->u8WaitAck)
    {
        PrintfUtil_vPrintf("TdoaInstSendTagPoolPacket dwt_setrxtimeout \n");
        //if the ACK is requested there is a 5ms timeout to stop RX if no ACK coming
        dwt_setrxtimeout(5000);  //units are us - wait for 5ms after RX on
    }

    //�������ݷ��� poll��Ϣ�����������ͣ�����Ҫ�ȴ���ʱ
    if (dwt_starttx(iTxDelayed | pstInstMsg->u8WaitAck))  // delayed start was too late
    {    
		PrintfUtil_vPrintf("TdoaInstSendTagPoolPacket dwt_starttx error\n");
        result = 1; //late/error
    }
    
    return result;                                              // state changes
    // after sending we should return to TX ON STATE ?
}

/*************************************************************
��������:TdoaSendTagPoll
��������:���⿨�Ϳ췢��������ѯ��Ϣ��Dw1000������
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaSendTagPoll(void)
{
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	uint8 u8SendLen = 0;
	uint32 systicks = portGetTickCnt();
    
	//�����豸�����������÷��ͻ�����
	if (pstInstMsg->stInstBaseData.eTxSpeedType == INST_MODE_TX_SPEED_SLOW)
	{
		pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SPEED_TYPE_BIT] = INST_MODE_TX_SPEED_SLOW;
	}
	else if (pstInstMsg->stInstBaseData.eTxSpeedType == INST_MODE_TX_SPEED_QUICK)
	{
		pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SPEED_TYPE_BIT] = INST_MODE_TX_SPEED_QUICK;
	}
    
    //Ϊ�����Ӵ��⿨��ѧϰ�����ж����� ����panid��Ϣ����
    pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_PANID_BIT_H] = (pstInstMsg->stInstBaseData.u16PanId >> 8) & 0xff;
	pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_PANID_BIT_L] = (pstInstMsg->stInstBaseData.u16PanId) & 0xff;

	//������Ϣ�����к� Ϊ������ԭ�е��˿�����Tdoa����rssi��վ�ĸ�ʽ ʹ�õ���͵���λ�洢���к�
	pstInstMsg->stInstBaseData.u16SeqNum ++;
	pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_H] = (pstInstMsg->stInstBaseData.u16SeqNum >> 8) & 0xff;
	pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_L] = (pstInstMsg->stInstBaseData.u16SeqNum) & 0xff;
	pstInstMsg->stTxMsgToPack.u8SeqNum = (uint8)pstInstMsg->stInstBaseData.u16SeqNum;
		
	//����ʵ�����͵�Ŀ�ĵ�ַ �˺���ֻ�д��⿨�Ϳ췢���ᴥ�� Ŀ�ĵ�ַ��������Ϊ�㲥ȫf 
	pstInstMsg->stInstBaseData.u16DestAddress = TDOA_INST_SEND_POLL_DEST_ADDR;

	//���豸mac�����÷���֡���ݼ����� ����ƼĴ�����ctrl[1]��Ӧ
	u8SendLen = TDOA_SEND_CARD_SAVE_START_INDEX;
    //��Ϊѧϰ������д��⿨����ֵ���
    TdoaInstFillTestTagRssiToSendMsgProc(pstInstMsg, TDOA_SEND_CARD_SAVE_START_INDEX, &u8SendLen);
    
	TdoaSetMacFrameData(pstInstMsg, u8SendLen, RTLS_TDOA_BLINK_SEND, !TDOA_INST_ACK_REQUESTED); // ��չ�����ֽڴ�����к���6��Ϊ8

	//������Ϣ����
	if(TdoaInstSendTagPoolPacket(pstInstMsg, DWT_START_TX_IMMEDIATE))
	{
		//������ʧ�ܣ��򴥷���ѯ�����¼����ⲿ����������ѯ�˴�����Ҫ�ٴ�����
        PrintfUtil_vPrintf("tdoa blink send fail %d \n", 
        pstInstMsg->stInstBaseData.u16SeqNum);
		//��ǰ���кŽ��м�һ����
		pstInstMsg->stInstBaseData.u16SeqNum --;

	}
    else
    {
        //ѧϰ��
        if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_STANDARD)
        {
            //����֮��׼�����մ��⿨��������Ϣ
            pstInstMsg->stInstBaseData.eTdoaInstState = TDOA_RX_WAIT_DATA;
        }
        else //���⿨
        {     
            //�����ͳɹ������������к�
        	 PrintfUtil_vPrintf("tdoa blink %d ,%d, %d\n", 
        		pstInstMsg->stInstBaseData.u16OwnAddress,
        		pstInstMsg->stInstBaseData.u16SeqNum, systicks);
        }
    }
    
	//���ͳɹ�����������ʾ
	dwt_setleds(2);

	//�����ͳɹ�������Ҫ���ж����������ѯ����Ϊ����������

	return;
}
/*************************************************************
��������:TdoaInstRxMsgToCardMsgProc
��������:

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-09-05

**************************************************************/
void TdoaInstRxRssiLevel(void)
{	
	double dRxRssiLever=0;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	
	dwt_readdignostics(&pstInstMsg->stDwDeviceLogDate.stDwRxDiag);
	double CIR = (double)pstInstMsg->stDwDeviceLogDate.stDwRxDiag.maxGrowthCIR;
	double NPC = (double)pstInstMsg->stDwDeviceLogDate.stDwRxDiag.rxPreamCount;
	
	dRxRssiLever = 10 * log10((CIR*131072)/(NPC*NPC))- 115.72  ;//121.74 -----64MHz  pow(2, 17)

	pstInstMsg->stInstBaseData.i8Rssi = (int8)dRxRssiLever;

	return;
}

/*************************************************************
��������:TdoaSaveTestRssiFormStandardCardMsg
��������:��վ��ѧϰ������Ϣ�л�ȡ��ǰ�Ĵ��⿨������ֵ

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaSaveTestRssiFormStandardCardMsg(void) 
{
	uint16	u16SrcAddr;
	uint16	u16Seqnum;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	
	//���л�������ʼ��
	memset(&gstTdoaCardMsg, 0, sizeof(TDOA_INST_RXMSG_TO_CARDMSG_S));
	
	//��ȡ�豸������ֵ
	TdoaInstRxRssiLevel();
	// ��ȡ������Ϣ��Դ��ַ
	memcpy(&u16SrcAddr, &(pstInstMsg->stRxMsgFromDw.u8SourceAddr), ADDR_BYTE_SIZE_S);
	u16Seqnum = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_H] << 8;		  //get the seqnum
	u16Seqnum += pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_L];
	
	gstTdoaCardMsg.u16CardId = u16SrcAddr & 0xffff;
	gstTdoaCardMsg.u16Seqnum = u16Seqnum;
	gstTdoaCardMsg.u16Speedtype = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SPEED_TYPE_BIT];
	gstTdoaCardMsg.u64RxCardTimestamp = pstInstMsg->stInstBaseData.u64RxTimeStamp;
	gstTdoaCardMsg.i8Rssi = pstInstMsg->stInstBaseData.i8Rssi;

	return;
}

/*************************************************************
��������:TdoaInstGetRssiFormRxMsg
��������:�ɻ�վ����ѧϰ������Ϣ�н�����Ӧ���⿨��������Ϣ

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstGetRssiFormRxMsg(TDOA_INSTANCE_DATA_S* pstInstMsg, 
                                        TDOA_INST_SAVE_RSSI_MSG_S* pstCardRssiMsg) 
{
    uint8  u8Loop = 0;
    uint8  u8TestTagCount = 0;
    
    //��ȡ��Ϣ�д��⿨�ĸ���
    u8TestTagCount = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_SEND_CARD_SAVE_START_INDEX - 1];
    pstCardRssiMsg->u8CardCount = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_SEND_CARD_SAVE_START_INDEX - 1];
	// ��ȡ������Ϣ��Դ��ַ
	memcpy(&pstCardRssiMsg->u16StdCardId, &(pstInstMsg->stRxMsgFromDw.u8SourceAddr), ADDR_BYTE_SIZE_S);

    for (u8Loop = 0; u8Loop < u8TestTagCount; u8Loop++)
    {
        memcpy(&pstCardRssiMsg->stTestRssiData[u8Loop], 
               &pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_SEND_CARD_SAVE_START_INDEX + u8Loop*sizeof(TDOA_INST_RSSI_DATA_S)],
               sizeof(TDOA_INST_RSSI_DATA_S));
        #if 0
        PrintfUtil_vPrintf("[%d] [%d]  [%d] [%d] rssi %i \n", u8Loop, pstCardRssiMsg->u8CardCount,
            pstCardRssiMsg->stTestRssiData[u8Loop].u16TestCardId,
            pstCardRssiMsg->u16StdCardId,
            pstCardRssiMsg->stTestRssiData[u8Loop].i8Rssi);
        #endif
    }
        
    return;
}

/*************************************************************
��������:TdoaInstSaveRssiFormRxMsg
��������:��վ�ɽ��յ�ѧϰ����Ϣ���ж�Ӧ���⿨���ݵĴ洢

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstSaveRssiFormRxMsg(TDOA_INST_SAVE_RSSI_MSG_S* pstStdCardMsg, 
                                         TDOA_INST_SAVE_RSSI_MSG_S* pstCardRssiMsg) 
{    
    uint8 u8Dealtime = 0;
    
    u8Dealtime = pstStdCardMsg->u8Dealtime;
    //��վÿ�ν��ն�Ӧѧϰ�������ݶ�������ȫ���Ǹ��� ����Ҫ�����ϻ������򻯴����߼�
    memcpy(pstStdCardMsg, pstCardRssiMsg, sizeof(TDOA_INST_SAVE_RSSI_MSG_S));
    pstStdCardMsg->u8Dealtime = u8Dealtime + 1; //�����յ��췢��������¼ �����ж��Ƿ���Ҫ������߿췢��
    
    return;
}

/*************************************************************
��������:TdoaInstExistStdCardSaveMsg
��������:��վ�Ѿ���¼ѧϰ��������¼�¼ѧϰ�����Ӧ���⿨����Ϣ����

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstExistStdCardSaveMsg(TDOA_INSTANCE_DATA_S* pstInstMsg, 
                                          TDOA_INST_STANDARD_CARD_MSG_S* pstStdCardMsg,
                                          uint8  u8CardPosition) 
{
    TDOA_INST_SAVE_RSSI_MSG_S stCardRssiMsg;

    //��ʼ���ṹ��
    memset(&stCardRssiMsg, 0, sizeof(TDOA_INST_SAVE_RSSI_MSG_S));
    //��ȡ��Ϣ�ж�Ӧ�Ĵ��⿨��Ϣ
    TdoaInstGetRssiFormRxMsg(pstInstMsg, &stCardRssiMsg);

    //����ȡ�Ĵ��⿨��Ϣ���Ѵ洢�Ĵ��⿨��Ϣ���бȶ� ������ˢ�£������������������¼
    TdoaInstSaveRssiFormRxMsg(&pstStdCardMsg->stStdCardMsg[u8CardPosition], &stCardRssiMsg);
    
    #if 0
    PrintfUtil_vPrintf("[%d] u8Dealtime [%d] \n", 
        pstStdCardMsg->stStdCardMsg[u8CardPosition].u16StdCardId, 
        pstStdCardMsg->stStdCardMsg[u8CardPosition].u8Dealtime);
    #endif
    
    return;
}

/*************************************************************
��������:TdoaInstNoExistStdCardSaveMsg
��������:��վδ��¼ѧϰ��������¼�¼ѧϰ�����Ӧ���⿨����Ϣ����

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstNoExistStdCardSaveMsg(TDOA_INSTANCE_DATA_S* pstInstMsg, 
                                              TDOA_INST_STANDARD_CARD_MSG_S* pstStdCardMsg,
                                              uint8  u8CardPosition)  
{
    uint8 u8Loop = 0;
    TDOA_INST_SAVE_RSSI_MSG_S stCardRssiMsg;

    //��ʼ���ṹ��
    memset(&stCardRssiMsg, 0, sizeof(TDOA_INST_SAVE_RSSI_MSG_S));
    //��ȡ��Ϣ�ж�Ӧ�Ĵ��⿨��Ϣ
    TdoaInstGetRssiFormRxMsg(pstInstMsg, &stCardRssiMsg);
    
    //���Ѿ��ﵽѧϰ���Ľ��������޳���һ��
    if (u8CardPosition >= TDOA_STANDCARD_MAX_NUM)
    {
        for (u8Loop = 1; u8Loop < TDOA_STANDCARD_MAX_NUM; u8Loop++)
        {
            memcpy(&pstStdCardMsg->stStdCardMsg[u8Loop-1], 
                   &pstStdCardMsg->stStdCardMsg[u8Loop],
                   sizeof(TDOA_INST_SAVE_RSSI_MSG_S));
        }
        
        //����ȡ�Ĵ��⿨��Ϣ���Ѵ洢�Ĵ��⿨��Ϣ���бȶ� ������ˢ�£������������������¼
        TdoaInstSaveRssiFormRxMsg(&pstStdCardMsg->stStdCardMsg[TDOA_STANDCARD_MAX_NUM - 1], &stCardRssiMsg); 
        pstStdCardMsg->stStdCardMsg[TDOA_STANDCARD_MAX_NUM - 1].u8Dealtime = 1; //���е�һ�ε����¼
    }
    else
    {
        //����ȡ�Ĵ��⿨��Ϣ���Ѵ洢�Ĵ��⿨��Ϣ���бȶ� ������ˢ�£������������������¼
        TdoaInstSaveRssiFormRxMsg(&pstStdCardMsg->stStdCardMsg[u8CardPosition], &stCardRssiMsg); 
        pstStdCardMsg->u8StdCardCount = pstStdCardMsg->u8StdCardCount + 1;
    }
    
    return;
}

/*************************************************************
��������:TdoaInstBetterSelectStdCardCheck
��������:�жϵ�ǰ�췢���Ƿ�������ѡ��

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
int TdoaInstBetterSelectStdCardCheck(uint16 u16TestCardId, uint16 u16StdCardId)  
{
    uint8 u8Loop = 0;
    TDOA_INST_BETTER_SELECT_TABLE_S* pstBetterStdCardTable = TdoaGetLocalBetterTableStructurePtr();

     #if 0
     //ѭ�����д洢���⿨��ѧϰ����Ӧ��ϵ
     for (u8Loop = 0; u8Loop < pstBetterStdCardTable->u8CardCount; u8Loop++)
     {
         
        PrintfUtil_vPrintf("Std check [%d] [%d] %d [%d] [%d] rssi %i \n", u8Loop, pstBetterStdCardTable->u8CardCount,
            u16TestCardId, 
            pstBetterStdCardTable->stBetterTable[u8Loop].u16TestCardId,
            pstBetterStdCardTable->stBetterTable[u8Loop].u16StdCardId,
            pstBetterStdCardTable->stBetterTable[u8Loop].i8Rssi);
       
    }
    #endif
    
    //ѭ�����д洢���⿨��ѧϰ����Ӧ��ϵ
    for (u8Loop = 0; u8Loop < pstBetterStdCardTable->u8CardCount; u8Loop++)
    {
        #if 0 
        PrintfUtil_vPrintf("[%d] [%d] %d [%d] [%d] rssi %i \n", u8Loop, pstBetterStdCardTable->u8CardCount,
            u16TestCardId, 
            pstBetterStdCardTable->stBetterTable[u8Loop].u16TestCardId,
            pstBetterStdCardTable->stBetterTable[u8Loop].u16StdCardId,
            pstBetterStdCardTable->stBetterTable[u8Loop].i8Rssi);
        #endif
        
        if (pstBetterStdCardTable->stBetterTable[u8Loop].u16TestCardId == u16TestCardId)
        {
            
            if (pstBetterStdCardTable->stBetterTable[u8Loop].u16StdCardId == u16StdCardId)
            {
                return TRUE;
            }

            break;
        }
    }
    
    return FALSE;
}

/*************************************************************
��������:TdoaInstSaveBetterSelectTableOneStdCard
��������:����ѧϰ��ʱ�洢����ѡ���

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstSaveBetterSelectTableOneStdCard(TDOA_INST_STANDARD_CARD_MSG_S* pstStdCardMsg)  
{
    uint8 u8Loop = 0;
    TDOA_INST_BETTER_SELECT_TABLE_S* pstBetterStdCardTable = TdoaGetLocalBetterTableStructurePtr();

    //ֻ��һ��ѧϰ��ʱֱ�Ӷ�ȡ��һ���洢λ�õ�����
    pstBetterStdCardTable->u8CardCount = pstStdCardMsg->stStdCardMsg[0].u8CardCount;

    //ѭ�����д洢���⿨��ѧϰ����Ӧ��ϵ
    for (u8Loop = 0; u8Loop < pstBetterStdCardTable->u8CardCount; u8Loop++)
    {
        pstBetterStdCardTable->stBetterTable[u8Loop].u16TestCardId = pstStdCardMsg->stStdCardMsg[0].stTestRssiData[u8Loop].u16TestCardId;
        pstBetterStdCardTable->stBetterTable[u8Loop].u16StdCardId  = pstStdCardMsg->stStdCardMsg[0].u16StdCardId;
        pstBetterStdCardTable->stBetterTable[u8Loop].i8Rssi        = pstStdCardMsg->stStdCardMsg[0].stTestRssiData[u8Loop].i8Rssi;
    }
    
    return;
}

/*************************************************************
��������:TdoaInstSaveBetterSelectTableOneStdCard
��������:���ѧϰ��ʱ�洢����ѡ���

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstSaveBetterSelectTableMultiStdCard(TDOA_INST_STANDARD_CARD_MSG_S* pstStdCardMsg)  
{
    uint8 u8Loop = 0;
    uint8 u8Loopi = 0; //ѭ��ѧϰ������
    uint8 u8Loopj = 0; //ѭ��ѧϰ����Ӧ�Ĵ��⿨
    uint8 u8Loopk = 0; //ѭ�����е�����ѡ���������ݵĸ���
    uint8 u8CardExistFlag = FALSE; //��ʼ�������ڱ�־
    
    TDOA_INST_BETTER_SELECT_TABLE_S stBetterTableTemp;
    TDOA_INST_BETTER_SELECT_TABLE_S* pstBetterStdCardTable = TdoaGetLocalBetterTableStructurePtr();

    //�����ű��е����ݿ�������ʱ���з������
    memset(&stBetterTableTemp, 0, sizeof(TDOA_INST_BETTER_SELECT_TABLE_S));
    memcpy(&stBetterTableTemp, pstBetterStdCardTable, sizeof(TDOA_INST_BETTER_SELECT_TABLE_S));

    //������ѡ���û����������г�ʼ�� ����������ʹ�����������������ݽ��жԱ�
    if (stBetterTableTemp.u8CardCount == 0)
    {
        //ֻ��һ��ѧϰ��ʱֱ�Ӷ�ȡ��һ���洢λ�õ�����
        stBetterTableTemp.u8CardCount = pstStdCardMsg->stStdCardMsg[0].u8CardCount;

        //ѭ�����д洢���⿨��ѧϰ����Ӧ��ϵ
        for (u8Loop = 0; u8Loop < stBetterTableTemp.u8CardCount; u8Loop++)
        {
            stBetterTableTemp.stBetterTable[u8Loop].u16TestCardId = pstStdCardMsg->stStdCardMsg[0].stTestRssiData[u8Loop].u16TestCardId;
            stBetterTableTemp.stBetterTable[u8Loop].u16StdCardId  = pstStdCardMsg->stStdCardMsg[0].u16StdCardId;
            stBetterTableTemp.stBetterTable[u8Loop].i8Rssi        = pstStdCardMsg->stStdCardMsg[0].stTestRssiData[u8Loop].i8Rssi;
        }
    }

    #if 0
    uint8 u8LoopTest1 = 0;
    uint8 u8LoopTest2 = 0;

    for (u8LoopTest1 = 0; u8LoopTest1 < pstStdCardMsg->u8StdCardCount; u8LoopTest1++)
    {
        for (u8LoopTest2 = 0; u8LoopTest2 < pstStdCardMsg->stStdCardMsg[u8LoopTest1].u8CardCount; u8LoopTest2++)
        {
          PrintfUtil_vPrintf("[%d][%d] [%d][%d] [%d][%d] rssi %i \n", u8LoopTest1, pstStdCardMsg->u8StdCardCount,
            u8LoopTest2, pstStdCardMsg->stStdCardMsg[u8LoopTest1].u8CardCount,
            pstStdCardMsg->stStdCardMsg[u8LoopTest1].u16StdCardId,
            pstStdCardMsg->stStdCardMsg[u8LoopTest1].stTestRssiData[u8LoopTest2].u16TestCardId,
            pstStdCardMsg->stStdCardMsg[u8LoopTest1].stTestRssiData[u8LoopTest2].i8Rssi);
        }
    }
    #endif
    
    //�Ե�һ��ѧϰ��Ϊ��׼ ������ѧϰ�����յ���Ϣ���жԱȴ洢
    for (u8Loopi = 0; u8Loopi < pstStdCardMsg->u8StdCardCount; u8Loopi++)
    {
        //ѭ��ѧϰ���еĴ��⿨��Ϣ�����µ����ݽ��бȶ�
        for (u8Loopj = 0; u8Loopj < pstStdCardMsg->stStdCardMsg[u8Loopi].u8CardCount; u8Loopj++)
        {
            //��ʼ�����ڱ�־
            u8CardExistFlag = FALSE;
            for (u8Loopk = 0; u8Loopk < stBetterTableTemp.u8CardCount; u8Loopk ++)
            {
                //�ж��Ƿ��Ѿ��м�¼
                if (pstStdCardMsg->stStdCardMsg[u8Loopi].stTestRssiData[u8Loopj].u16TestCardId == stBetterTableTemp.stBetterTable[u8Loopk].u16TestCardId)
                {
                     #if 0
                      PrintfUtil_vPrintf("i:[%d][%d] [%d][%d] [%d][%d] [%d][%d]rssi %i [%d][%d]rssi %i\n",
                        u8Loopi, pstStdCardMsg->u8StdCardCount, 
                        u8Loopj, pstStdCardMsg->stStdCardMsg[u8Loopi].u8CardCount,
                        u8Loopk, stBetterTableTemp.u8CardCount,
                        pstStdCardMsg->stStdCardMsg[u8Loopi].u16StdCardId,
                        pstStdCardMsg->stStdCardMsg[u8Loopi].stTestRssiData[u8Loopj].u16TestCardId,
                        pstStdCardMsg->stStdCardMsg[u8Loopi].stTestRssiData[u8Loopj].i8Rssi,
                        stBetterTableTemp.stBetterTable[u8Loopk].u16StdCardId,
                        stBetterTableTemp.stBetterTable[u8Loopk].u16TestCardId,
                        stBetterTableTemp.stBetterTable[u8Loopk].i8Rssi);
                      #endif
                      
                    //���������˵�����⿨�Ѿ����� ����������û�и��¾��������Ӵ��⿨
                    u8CardExistFlag = TRUE;
                    //�����յ�������ǿ����и�������ѡ����е�ѧϰ�� ����ֵԽС˵������Խ��
                    if ((pstStdCardMsg->stStdCardMsg[u8Loopi].stTestRssiData[u8Loopj].i8Rssi - 
                        stBetterTableTemp.stBetterTable[u8Loopk].i8Rssi) > 0)
                    {
                        stBetterTableTemp.stBetterTable[u8Loopk].i8Rssi = pstStdCardMsg->stStdCardMsg[u8Loopi].stTestRssiData[u8Loopj].i8Rssi;
                        stBetterTableTemp.stBetterTable[u8Loopk].u16StdCardId = pstStdCardMsg->stStdCardMsg[u8Loopi].u16StdCardId;
                        
                        #if 0
                        PrintfUtil_vPrintf("i[%d]j[%d]k[%d] StdCardtime[%d] time[%d] [%d][%d]rssi %i\n", 
                            u8Loopi, u8Loopj, u8Loopk,
                            stBetterTableTemp.u8Dealtime,
                            pstStdCardMsg->stStdCardMsg[u8Loopi].u8Dealtime,
                            stBetterTableTemp.stBetterTable[u8Loopk].u16TestCardId,
                            stBetterTableTemp.stBetterTable[u8Loopk].u16StdCardId,
                            stBetterTableTemp.stBetterTable[u8Loopk].i8Rssi);
                        #endif
                        break;
                    }
                }
                //�����⿨δ���洢������ѡ�����������Ӵ洢
                if (u8CardExistFlag != TRUE)
                {
                    //������ֵ�������򲻽��д洢 ������ѭ��������һ������ֵ���ж�
                    if (pstStdCardMsg->stStdCardMsg[u8Loopi].stTestRssiData[u8Loopj].i8Rssi == 0)
                    {
                        continue;
                    }
                    
                    //�������ݴ��������¼ �����ж��Ƿ����ѧϰ������⿨��Ϣ�ṹ�ĳ�ʼ��
                    stBetterTableTemp.u8Dealtime = stBetterTableTemp.u8Dealtime + 1;
    
                    //��δ�ﵽ�洢���� ��ֱ�ӽ��д洢
                    if (stBetterTableTemp.u8CardCount < TDOA_MSG_TESTCARD_MAX_NUM)
                    {
                        stBetterTableTemp.stBetterTable[stBetterTableTemp.u8CardCount].u16TestCardId = pstStdCardMsg->stStdCardMsg[u8Loopi].stTestRssiData[u8Loopj].u16TestCardId;
                        stBetterTableTemp.stBetterTable[stBetterTableTemp.u8CardCount].u16StdCardId = pstStdCardMsg->stStdCardMsg[u8Loopi].u16StdCardId;
                        stBetterTableTemp.stBetterTable[stBetterTableTemp.u8CardCount].i8Rssi = pstStdCardMsg->stStdCardMsg[u8Loopi].stTestRssiData[u8Loopj].i8Rssi;
                        stBetterTableTemp.u8CardCount = stBetterTableTemp.u8CardCount + 1;
                    }
                    else if (stBetterTableTemp.u8CardCount >= TDOA_MSG_TESTCARD_MAX_NUM) //���Ѿ��ﵽ�˴洢���� ���޳�����һ�����д洢
                    {
                        //���Ѿ��ﵽ�˴洢���� ���޳�����һ�����д洢
                        for (u8Loop = 1; u8Loop < stBetterTableTemp.u8CardCount; u8Loop++)
                        {
                            memcpy(&stBetterTableTemp.stBetterTable[u8Loop-1], 
                                   &stBetterTableTemp.stBetterTable[u8Loop],
                                   sizeof(TDOA_INST_RELATION_TABLE_S)
                                   );
                        }
                        
                        stBetterTableTemp.stBetterTable[TDOA_MSG_TESTCARD_MAX_NUM - 1].u16TestCardId = pstStdCardMsg->stStdCardMsg[u8Loopi].stTestRssiData[u8Loopj].u16TestCardId;
                        stBetterTableTemp.stBetterTable[TDOA_MSG_TESTCARD_MAX_NUM - 1].u16StdCardId = pstStdCardMsg->stStdCardMsg[u8Loopi].u16StdCardId;
                        stBetterTableTemp.stBetterTable[TDOA_MSG_TESTCARD_MAX_NUM - 1].i8Rssi = pstStdCardMsg->stStdCardMsg[u8Loopi].stTestRssiData[u8Loopj].i8Rssi;
                    }
                }
            }
        }
    }
    
    //�������ű�����
    memcpy(pstBetterStdCardTable, 
           &stBetterTableTemp,
           sizeof(TDOA_INST_RELATION_TABLE_S)
           );
    
    //�������������ѧϰ���Ĵ���˵�������Ѿ�������ȡ ���Խ������ݵĳ�ʼ�� ��֤���ݵ�ʵʱ�� �����������췢��ʱ�Ž��г�ʼ��
    if (pstBetterStdCardTable->u8Dealtime >= TDOA_STANDCARD_MAX_NUM)
    {        
        //��ʼ��ѧϰ������⿨����Ϣ�ṹ ��֤ÿ�����ڶ�����ȡ���µ�����
        TdoaStdCardMsgBuffInit();
        pstBetterStdCardTable->u8Dealtime = 0;
    }
        
    return;
}

/*************************************************************
��������:TdoaInstBetterSelectTableByRssi
��������:ͨ����֪��ѧϰ������⿨��������Ӧ��ϵ�γ����ŵ�ѧϰ��ѡ���

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstBetterSelectTableByRssi(TDOA_INST_STANDARD_CARD_MSG_S* pstStdCardMsg)  
{
    //��ֻ��һ��ѧϰ�����ý��жԱ�ѡ��
    if (pstStdCardMsg->u8StdCardCount == 1)
    {
        //���д��⿨��ѧϰ����ϵ��Ӧ
        TdoaInstSaveBetterSelectTableOneStdCard(pstStdCardMsg);
    }
    else if (pstStdCardMsg->u8StdCardCount > 1)
    {
        TdoaInstSaveBetterSelectTableMultiStdCard(pstStdCardMsg);
    }
    
    return;
}

/*************************************************************
��������:TdoaInstSaveStdCardMsgProc
��������:��վ��ȡѧϰ����Ϣ��Զ�Ӧ�Ĵ��⿨��Ϣ���д洢

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstSaveStdCardMsgProc(TDOA_INSTANCE_DATA_S* pstInstMsg)
{
	uint16	u16SrcAddr;
    uint8  u8Loop = 0;
    uint8  u8LoopNext = 0;
    uint8  u8CardPosition = 0;
    uint8  u8TestTagCount = 0;
    uint8  u8StdExistFlag = FALSE;
    uint8  u8Speedtype = 0;  //���⿨����Ϊ1 ѧϰ������Ϊ2
    
    TDOA_INST_STANDARD_CARD_MSG_S* pstStdCardMsg = TdoaGetLocalStdCardMsgStructurePtr();

    //��ȡ��ϢԴ�Ŀ�����
    u8Speedtype = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SPEED_TYPE_BIT];
	// ��ȡ������Ϣ��Դ��ַ
	memcpy(&u16SrcAddr, &(pstInstMsg->stRxMsgFromDw.u8SourceAddr), ADDR_BYTE_SIZE_S);
    //��ȡ��Ϣ�д��⿨�ĸ���
    u8TestTagCount = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_SEND_CARD_SAVE_START_INDEX - 1];
    
    //�����յĲ���ѧϰ����Ϣ�򲻽��д洢����
    if (u8Speedtype != INST_MODE_TX_SPEED_QUICK)
    {
        return;
    }
    
    //�������ڴ��⿨��Ϣ�򲻽��д洢����
    if (u8TestTagCount <= 0)
    {
        return;
    }
    
    //�ж��Ƿ��Ѿ��洢�˶�Ӧ��ѧϰ��
    for (u8Loop = 0; u8Loop < pstStdCardMsg->u8StdCardCount; u8Loop++)
    {
        if (pstStdCardMsg->stStdCardMsg[u8Loop].u16StdCardId == u16SrcAddr)
        {
            u8CardPosition = u8Loop;
            u8StdExistFlag = TRUE;
        }
    }
    
    #if 0
    PrintfUtil_vPrintf("u16SrcAddr [%d] u8StdExistFlag[%d] u8CardPosition [%d] pstStdCardMsg->u8StdCardCount[%d]\n", 
            u16SrcAddr,
            u8StdExistFlag, 
            u8CardPosition,
            pstStdCardMsg->u8StdCardCount);
    #endif
    
    //û����һ�����ݽ���һ��������Ϣ�����ϻ�ʱ���¼
    if ((gu8Dealtime%TDOA_STANDCARD_MAX_NUM) == 0)
    {
        //��¼֮ǰ�Ƚ��д�����¼
        for (u8Loop = 0; u8Loop < pstStdCardMsg->u8StdCardCount; u8Loop++)
        {
            pstStdCardMsg->stStdCardMsg[u8Loop].u8LastDealtime = pstStdCardMsg->stStdCardMsg[u8Loop].u8Dealtime;
        }
    }
    gu8Dealtime ++;
    
    //ѧϰ���Ѿ�����¼
    if (u8StdExistFlag == TRUE)
    {
        TdoaInstExistStdCardSaveMsg(pstInstMsg, pstStdCardMsg, u8CardPosition);
    }
    else //ѧϰ��δ����¼
    {
        TdoaInstNoExistStdCardSaveMsg(pstInstMsg, pstStdCardMsg, pstStdCardMsg->u8StdCardCount);
    }
    
    #if 0
    PrintfUtil_vPrintf("\n");
    uint8 u8Loop1 = 0;
    for (u8Loop1 = 0; u8Loop1 < pstStdCardMsg->u8StdCardCount; u8Loop1++)
    {
        PrintfUtil_vPrintf("[%d][%d] Addr [%d] [%d][%d] [%d]\n", 
                            u8Loop1,
                            pstStdCardMsg->u8StdCardCount,
                            pstStdCardMsg->stStdCardMsg[u8Loop1].u16StdCardId,
                            pstStdCardMsg->stStdCardMsg[u8Loop1].u8Dealtime,
                             pstStdCardMsg->stStdCardMsg[u8Loop1].u8LastDealtime,
                            gu8Dealtime);

    }
    PrintfUtil_vPrintf("\n");
    
    #endif
    
    //ͨ���жϴ��ڵ�ʱ�����ڽ����ظ����� ���ߴ��������޳�
    for (u8Loop = 0; u8Loop < pstStdCardMsg->u8StdCardCount; u8Loop++)
    {
        if ((pstStdCardMsg->stStdCardMsg[u8Loop].u8Dealtime -
            pstStdCardMsg->stStdCardMsg[u8Loop].u8LastDealtime) == 0)
        {
            //���������ƶ�
            for (u8LoopNext = u8Loop+1; u8LoopNext < pstStdCardMsg->u8StdCardCount; u8LoopNext++)
            {
                memcpy(&pstStdCardMsg->stStdCardMsg[u8LoopNext-1], 
                       &pstStdCardMsg->stStdCardMsg[u8LoopNext],
                       sizeof(TDOA_INST_SAVE_RSSI_MSG_S));
            }
            pstStdCardMsg->u8StdCardCount = pstStdCardMsg->u8StdCardCount - 1;
        }
    }
    
    //�γ�����ѡ��� ����ԽǿԽ���׽���������
    TdoaInstBetterSelectTableByRssi(pstStdCardMsg);

	return;
}

/*************************************************************
��������:TdoaInstRxMsgToCardMsgProc
��������:��ʵ�����ɽ�������������ȡ�����ݸ�ֵ������ȫ�ֻ�������Ϊ���������׼��

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstRxMsgToCardMsgProc(void)
{
	uint16	u16SrcAddr;
	uint16	u16Seqnum;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	TDOA_INST_RXMSG_TO_CARDMSG_S* pstTdoaCardMsg = NULL;

    //��ʼ������ѧϰ���ʹ��⿨������Ϣ����Ϣ�ṹ
    TdoaCardMsgBuffInit();
    pstTdoaCardMsg = TdoaGetLocalCardStructurePtr();
    
	//��ȡ�豸������ֵ
	TdoaInstRxRssiLevel();
	// ��ȡ������Ϣ��Դ��ַ
	memcpy(&u16SrcAddr, &(pstInstMsg->stRxMsgFromDw.u8SourceAddr), ADDR_BYTE_SIZE_S);
	u16Seqnum = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_H] << 8;		  //get the seqnum
	u16Seqnum += pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_L];
	
	pstTdoaCardMsg->u16CardId = u16SrcAddr & 0xffff;
	pstTdoaCardMsg->u16Seqnum = u16Seqnum;
	pstTdoaCardMsg->u16Speedtype = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SPEED_TYPE_BIT];
	pstTdoaCardMsg->u64RxCardTimestamp = pstInstMsg->stInstBaseData.u64RxTimeStamp;
	pstTdoaCardMsg->i8Rssi = pstInstMsg->stInstBaseData.i8Rssi;

    //��վ�յ�ѧϰ����Ϣ���ȡ��Ӧ���յĴ��⿨��Ϣ���ж�Ӧ��Ϣ�洢
    TdoaInstSaveStdCardMsgProc(pstInstMsg);

	return;
}

/*************************************************************
��������:TdoaInstFillTestTagRssiToSendMsgProc
��������:ѧϰ�������⿨������Ϣ�����͵���Ϣ�д���

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstFillTestTagRssiToSendMsgProc(TDOA_INSTANCE_DATA_S* pstInstMsg, uint8 u8StartIndex, uint8* pu8SendLen)
{
    uint8 u8Loop = 0;
    uint8 u8TagCount = 0;
    uint8 u8FillMsgIndex = 0;
    //�洢���⿨�Ѿ�ѧϰ�����յ����⿨��Ӧ������ֵ
    TDOA_INST_SAVE_RSSI_MSG_S* pstTdoaSaveRssiMsg =  TdoaGetLocalCardRssiStructurePtr();

    //ֻ��ѧϰ�����д��������� ��ѧϰ�������д���
    if (pstInstMsg->stInstBaseData.eTdoaInstMode != TDOA_INST_TAG_STANDARD)
    {
       return;
    }

   //��ʼ���������
    u8FillMsgIndex = u8StartIndex;
    
    //��ȡ��ǰ�洢�Ĵ��⿨����
    u8TagCount = pstTdoaSaveRssiMsg->u8CardCount;
    
    //����������Ҫ���͵Ĵ��⿨�򲻽��д���
    if (u8TagCount <= 0)
    {
        return;
    }
    
    //��15���ֽڴ洢ѧϰ������
    pstInstMsg->stTxMsgToPack.u8MessageData[u8FillMsgIndex -1] = u8TagCount;
    
    #if 1
    for (u8Loop = 0; u8Loop < u8TagCount; u8Loop++)
    {
        PrintfUtil_vPrintf("[%d][%d] [%d][%d] rssi %i \n", 
                            u8Loop, u8TagCount,
                            pstInstMsg->stInstBaseData.u16OwnAddress,
                            pstTdoaSaveRssiMsg->stTestRssiData[u8Loop].u16TestCardId,
                            pstTdoaSaveRssiMsg->stTestRssiData[u8Loop].i8Rssi);
    }
    PrintfUtil_vPrintf("\n");
    #endif
    
    //�������⿨�����洢�� ���ظ��洢
    for (u8Loop = 0; u8Loop < u8TagCount; u8Loop++)
    {
        memcpy(&pstInstMsg->stTxMsgToPack.u8MessageData[u8FillMsgIndex + u8Loop* sizeof(TDOA_INST_RSSI_DATA_S)], &pstTdoaSaveRssiMsg->stTestRssiData[u8Loop], sizeof(TDOA_INST_RSSI_DATA_S));
        //PrintfUtil_vPrintf("Sd %d %d %d\n", u16TagDevId, u8Loop, pInst->macdata_msdu[u8FillMsgIndex -1]);
    }

    //�ж�ѧϰ���洢���⿨�����Ƿ��Ѿ������⿨���� ��������д洢�ṹ�ĳ�ʼ��
    if (pstTdoaSaveRssiMsg->u8Dealtime >= TDOA_MSG_TESTCARD_MAX_NUM)
    {
        //��ʼ��ѧϰ���洢���⿨Rssiֵ�Ĵ洢�ṹ
        TdoaStdCardSaveRssiBuffInit();  
    }
    
    //��ʼ���λ�ü������Ĵ��⿨������
    *pu8SendLen = u8StartIndex + u8TagCount * sizeof(TDOA_INST_RSSI_DATA_S);
        
	return;
}

/*************************************************************
��������:TdoaInstSaveTestTagRssiProc
��������:ѧϰ���洢���⿨������ֵ����

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstSaveTestTagRssiProc(uint16 u16OwnAddr, uint16 u16TestTagAddr, int i8Rssi)
{
    uint8 u8Loop = 0;
    uint8 u8CardPosition = 0;
    uint8 u8ExistFlag = FALSE;
    //�洢���⿨�Ѿ�ѧϰ�����յ����⿨��Ӧ������ֵ
    TDOA_INST_SAVE_RSSI_MSG_S* pstTdoaSaveRssiMsg =  TdoaGetLocalCardRssiStructurePtr();

    //����Ƿ��Ѿ��д洢���Ѿ��洢ֻ����ˢ�£�û�д洢������µļ�¼
    for (u8Loop = 0; u8Loop < pstTdoaSaveRssiMsg->u8CardCount; u8Loop++)
    {
        if (pstTdoaSaveRssiMsg->stTestRssiData[u8Loop].u16TestCardId == u16TestTagAddr)
        {
            u8CardPosition = u8Loop;
            u8ExistFlag = TRUE;
        }
    }

    //ѧϰ��������һ�����ݵĴ洢�����������һ�� ��Ϊ�ж��Ƿ���Ҫˢ�����ݵı�־
    pstTdoaSaveRssiMsg->u8Dealtime = pstTdoaSaveRssiMsg->u8Dealtime + 1;
        
    //�����������и��� ����������������´洢
    if (u8ExistFlag == TRUE)
    {
        pstTdoaSaveRssiMsg->stTestRssiData[u8CardPosition].i8Rssi = i8Rssi;
    }
    else
    {
        if (pstTdoaSaveRssiMsg->u8CardCount < TDOA_MSG_TESTCARD_MAX_NUM)
        {
            pstTdoaSaveRssiMsg->u16StdCardId = u16OwnAddr;
            pstTdoaSaveRssiMsg->stTestRssiData[pstTdoaSaveRssiMsg->u8CardCount].u16TestCardId = u16TestTagAddr;
            pstTdoaSaveRssiMsg->stTestRssiData[pstTdoaSaveRssiMsg->u8CardCount].i8Rssi    = i8Rssi;
            pstTdoaSaveRssiMsg->u8CardCount = pstTdoaSaveRssiMsg->u8CardCount + 1; //�յ��ô��⿨������1
        }
        else if (pstTdoaSaveRssiMsg->u8CardCount >= TDOA_MSG_TESTCARD_MAX_NUM)
        {
            for (u8Loop = 1; u8Loop < pstTdoaSaveRssiMsg->u8CardCount; u8Loop++)
            {
                memcpy(&pstTdoaSaveRssiMsg->stTestRssiData[u8Loop-1], 
                       &pstTdoaSaveRssiMsg->stTestRssiData[u8Loop],
                       sizeof(TDOA_INST_RSSI_DATA_S));
            }
            
            pstTdoaSaveRssiMsg->u16StdCardId = u16OwnAddr;
            pstTdoaSaveRssiMsg->stTestRssiData[TDOA_MSG_TESTCARD_MAX_NUM-1].u16TestCardId = u16TestTagAddr;
            pstTdoaSaveRssiMsg->stTestRssiData[TDOA_MSG_TESTCARD_MAX_NUM-1].i8Rssi    = i8Rssi;
            pstTdoaSaveRssiMsg->u8CardCount = TDOA_MSG_TESTCARD_MAX_NUM; //���Ĵ洢�������ܳ����洢�ṹ�±�

        }
    }
    
	return;
}

/*************************************************************
��������:TdoaInstRxTestMsgProc
��������:���մ��⿨�Ĺ���ֵ�жϴ��⿨��ѧϰ���ľ���

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstRxTestMsgProc(void)
{
    uint16	u16OwnAddr;
	uint16	u16SrcAddr;
    uint16	u16Speedtype;
    uint16	u16PanId;
    int     i8Rssi = 0;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

    //�����յĿ����Ǵ��⿨�򲻽��������Ĵ洢
    u16Speedtype = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SPEED_TYPE_BIT];
    u16PanId = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_PANID_BIT_H] << 8;
	u16PanId += pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_PANID_BIT_L];

    if ((u16Speedtype != INST_MODE_TX_SPEED_SLOW) || 
        (u16PanId != TDOA_INST_PANID_TAG))
    {
        return;
    }
    
	//��ȡ�豸������ֵ
	TdoaInstRxRssiLevel();
	// ��ȡ������Ϣ��Դ��ַ
	u16OwnAddr = pstInstMsg->stInstBaseData.u16OwnAddress;
	memcpy(&u16SrcAddr, &(pstInstMsg->stRxMsgFromDw.u8SourceAddr), ADDR_BYTE_SIZE_S);
    i8Rssi = pstInstMsg->stInstBaseData.i8Rssi;
               
    //PrintfUtil_vPrintf("%d %d rssi %i \n", u16OwnAddr, u16SrcAddr, pstInstMsg->stInstBaseData.i8Rssi);

    //�洢���⿨�Ѿ�ѧϰ�����յ����⿨��Ӧ������ֵ
    TdoaInstSaveTestTagRssiProc(u16OwnAddr, u16SrcAddr, i8Rssi);
        
	return;
}

/*************************************************************
��������:AppInstanceInit
��������:�豸ʵ����ʼ��   
1.��ʱ����ע����ʱ��Ӱ�� 
2.��������������˫������
3.������Ӧ��ʱ��������

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstRunState(void)
{
	int iTdoaInstMode    = 0; //ʵ����ɫ����
	int iTdoaInstState   = 0; //ʵ����ǰ����״̬
	int iDwEventType 	 = 0; //ʵ����ǰ�¼�����
	int iRxMsgFrameType  = 0; //��������֡����
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

	iTdoaInstMode   = pstInstMsg->stInstBaseData.eTdoaInstMode;
	iTdoaInstState  = pstInstMsg->stInstBaseData.eTdoaInstState;
	iDwEventType    = pstInstMsg->stInstBaseData.eDwEventType;
	iRxMsgFrameType = pstInstMsg->stRxMsgFromDw.u8MessageData[0];

	//�ж�ʵ����ǰ����״̬
    switch (iTdoaInstState)
    {
    	case TDOA_INIT: //��ʼ��״̬���и����豸���ͷֱ��������
			switch(iTdoaInstMode)
			{
				case TDOA_INST_TAG_TEST:
				case TDOA_INST_TAG_STANDARD:
					TdoaTagInit();
					break;
					
				case TDOA_INST_ANCHOR:
					TdoaAnchorInit();

					break;

	                default: //������������д���
	                break;
			}
			break; 
			
		case TDOA_RX_WAIT_DATA:
			switch(iDwEventType) //ʵ���¼�����
			{
				case DWT_SIG_RX_OKAY:

					switch(iRxMsgFrameType) //ʵ������֡����
					{
						case RTLS_TDOA_BLINK_SEND:
							if (iTdoaInstMode == TDOA_INST_ANCHOR)
							{
								//��ʵ�����ɽ�������������ȡ�����ݸ�ֵ������ȫ�ֻ�����
								TdoaInstRxMsgToCardMsgProc();
								//�޸�ʵ��״̬
								pstInstMsg->stInstBaseData.eTdoaInstState = TDOA_RX_WAIT_DATA;
								//������ݻ���������û�н���ʱʹ����һ�����ݽ��д���
								memset(&pstInstMsg->stRxMsgFromDw, 0, sizeof(TDOA_DW_TRX_MSG_S));
							    //pstInstMsg->stInstBaseData.eDwEventType = DWT_SIG_RX_NOERR; //�ı����״̬���ȴ���һ�ν���
								//�����豸��������¼�
								event_timer_set(EVENT_CHECKTDOA_REVMSG_EVENT); 
								//�������������
								pstInstMsg->stInstBaseData.i8LostPollPackCount = 0;
							}
							else if (iTdoaInstMode == TDOA_INST_TAG_STANDARD)
                            {
                                //��ȡ���⿨������ֵ ������ֵ���͸���վ ����վ�յ����ѧϰ��ʱ��Ϣʱ ��վ��������ֵ�����ж�ʹ����һ��ѧϰ��
                                TdoaInstRxTestMsgProc();
                            }
                            //�޸Ľ���״̬ ����״ֻ̬ͨ���жϽ����޸�
                            pstInstMsg->stInstBaseData.eTdoaInstState = TDOA_RX_DATA_DONE;
							break;

		                default: //������������д���
							//�����豸���� ��������ʱ�Ϳ�ʼ���� �����Զ�����
						    //PrintfUtil_vPrintf("4\n");

							// PrintfUtil_vPrintf("other RxMsgFrameType\n");
                            //new123 ���翪���ᵼ��˫���������ݲ�����ȷ��ȡ							dwt_rxenable(0); 
                            //   dwt_rxenable(0); //Ϊ������췢�����գ�����ʱ����ʱ������Ϊ�췢���ķ�������
			                break;
							
					}
					break;
					 
                default: //������������д���
					//�����豸���� ��������ʱ�Ϳ�ʼ���� �����Զ�����
					PrintfUtil_vPrintf("other DwEventType\n");
					dwt_rxenable(0); 
					//���ж�����ͳ��
					pstInstMsg->stInstBaseData.i8LostPollPackCount ++;
	                break;
			}
			break;

			default: //������������д���
				PrintfUtil_vPrintf("other TdoaInstState\n");
				if (TDOA_INST_ANCHOR == TDOA_INST_ANCHOR)
				{
					dwt_rxenable(0); 	
					//���ж�����ͳ��
					//pstInstMsg->stInstBaseData.i8LostPollPackCount ++;
				}
				
				break;
	}

	return;
}
#endif

#if IF_DESC(ע:TDOA�豸��ʼ��)
/*************************************************************
��������:Dw1000Init
��������:dw1000��ʼ��
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
int Dw1000Init( void )
{
    int result;
    uint32 u32DevID ;

	reset_DW1000();
	//����SPI���� ���ٽ���оƬ�Ķ�д ���ٽ������ݵĴ���
    port_set_dw1000_slowrate();  //max SPI before PLLs configured is ~4M

	//���Ի�ȡDW1000�豸ID������ȡʧ�����Ի��Ѳ���
	u32DevID = dwt_readdevid() ;
	PrintfUtil_vPrintf("u32DevID = %d Dw1000Init 1111\n", u32DevID);
	if(DWT_DEVICE_ID != u32DevID) //if the read of device ID fails, the DW1000 could be asleep
	{
		port_wakeup_dw1000();

		u32DevID = dwt_readdevid() ;
		// SPI not working or Unsupported Device ID
		if(DWT_DEVICE_ID != u32DevID)
		{
			return(-1) ;
		}
		//clear the sleep bit - so that after the hard reset below the DW does not go into sleep
		dwt_softreset();
	}
	
	 //reset the DW1000 by driving the RSTn line low
	 reset_DW1000();

	//������DW1000�շ�����ͨ�ţ�����ȡ��DEV_ID�Ĵ�������ַ0x00������֤IC�Ƿ���֧��
    result = dwt_initialise(DWT_LOADUCODE | DWT_LOADTXCONFIG | DWT_LOADANTDLY| DWT_LOADXTALTRIM) ;
	PrintfUtil_vPrintf("result = %d Dw1000Init 2222\n", result);

    //��λled�����ź� 
    dwt_setleds(2) ; //configure the GPIOs which control the leds on EVBs

    if (DWT_SUCCESS != result)
    {
        return (-1) ;   // device initialise has failed
    }

    //�����ж�
	dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | ( DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO /*| DWT_INT_RXPTO*/), 1);

    //enable TX, RX states on GPIOs 6 and 5
//new123    dwt_setlnapamode(1,1);
	
	//��ʼ��dw1000�¼�����
    dwt_configeventcounters(TDOA_EABLE); //enable and clear - NOTE: the counters are not preserved when in DEEP SLEEP
	
	port_set_dw1000_fastrate();

	return 0;
}

/*************************************************************
��������:TdoaTagInit
��������:

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-09-05

**************************************************************/
void TdoaTagInit(void)
{
	int iTdoaInstMode    = 0; //ʵ����ɫ����
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

	//���ý���֡�Ĺ���
	dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN); //allow data, ack frames;

    //�����豸���͵Ĳ�ͬ����Ϊ��ͬ��panid
    if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_TEST)
    {
    	pstInstMsg->stInstBaseData.u16PanId = TDOA_INST_PANID_TAG; //���ô��⿨��panid
    }
    else if (pstInstMsg->stInstBaseData.eTdoaInstMode == TDOA_INST_TAG_TEST)
    {
    	pstInstMsg->stInstBaseData.u16PanId = TDOA_INST_PANID_TAG_STANDARD; //����ѧϰ����panid
    }
    
	//�����豸��panid
	dwt_setpanid(pstInstMsg->stInstBaseData.u16PanId);

	//�����豸�ĵ�ַ
	//set source address into the message structure
	memcpy(&pstInstMsg->stTxMsgToPack.u8SourceAddr, &pstInstMsg->stInstBaseData.u16OwnAddress, ADDR_BYTE_SIZE);
	//dwt_setaddress16(pstInstMsg->stInstBaseData.u16OwnAddress); 

	//�����Զ�����
	dwt_setautorxreenable(pstInstMsg->stInstBaseData.u8RxAutoreEnable); //not necessary to auto RX re-enable as the receiver is on for a short time (Tag knows when the response is coming)

	//�رս�����˫ͨ��������
	#if (DOUBLE_RX_BUFFER == 1)
		dwt_setdblrxbuffmode(0); //disable double RX buffer
    #endif

	//�����Զ�Ӧ��
    #if (ENABLE_AUTO_ACK == 1)
		dwt_enableautoack(ACK_RESPONSE_TIME); //wait for 5 symbols before replying with the ACK
    #endif

	return;
}

/*************************************************************
��������:TdoaAnchorInit
��������:

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-09-05

**************************************************************/
void TdoaAnchorInit(void)
{
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

	pstInstMsg->stInstBaseData.u16PanId = TDOA_INST_PANID_ANCHOR ;//;0xdeca  0xdddd 0xeeee
	//�����豸panid
	dwt_setpanid(pstInstMsg->stInstBaseData.u16PanId);
	
	//�����豸�ĵ�ַ  ������Ҫ�ýṹ���Ա�滻ȫ�ֱ���u16ShortAddr
	dwt_setaddress16(pstInstMsg->stInstBaseData.u16OwnAddress); 
	
	//set source address into the message structure �����滻��
	memcpy(&pstInstMsg->stTxMsgToPack.u8SourceAddr, &pstInstMsg->stInstBaseData.u16OwnAddress, ADDR_BYTE_SIZE);

	//���÷��ͺ���������
	dwt_setrxaftertxdelay(WAIT_FOR_RESPONSE_DLY); //set the RX after TX delay time

	//�����Զ����չ���
	#if (DECA_BADF_ACCUMULATOR == 0) //can use RX auto re-enable when not logging/plotting errored frames
		pstInstMsg->stInstBaseData.u8RxAutoreEnable = 1;
	#endif

	//dwt_setdblrxbuffmode���ö�Ӧ�ֶ�Ϊ0ʱΪ����������˫������
     #if (DOUBLE_RX_BUFFER == 0)
		dwt_setdblrxbuffmode(1); //enable double RX buffer 
     #endif
	 
	 //�����Զ�����ʹ�ó���:������һ������֡�󣬿�������˫�������л�������ָ�룬�����Զ�����ʹ����֡����ʧ 
	 pstInstMsg->stInstBaseData.u8RxAutoreEnable = 1;
	 dwt_setautorxreenable(pstInstMsg->stInstBaseData.u8RxAutoreEnable);

	 //���ý��ղ���ʱ
	 dwt_setrxtimeout(0);
	 
	 //������������������
	 dwt_rxenable(0) ; //������������������
	
	return;

}

/*************************************************************
��������:TdoaInstChannelConfig
��������:����ͨ��ģʽ���� ģʽ����ֻ�е��β���Ҫ����ͨ��ģʽ�ṹ�崫��
����˵��:uint8 usModeNum ģʽ����
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaInstChannelConfig(uint8 usModeNum)
{
	uint32 u32Txpower = 0;
	dwt_config_t stChannelCfg;
    int iUseNvmData = DWT_LOADANTDLY | DWT_LOADXTALTRIM;
	
	//����ͨ������
    gstTdoaInstData.stDwChannelCfg.u8ChannelNum      = gstChConfig[usModeNum].u8ChannelNumber ;
    gstTdoaInstData.stDwChannelCfg.u8Prf             = gstChConfig[usModeNum].u8PulseRepFreq ;
    gstTdoaInstData.stDwChannelCfg.u8TxPreambLength  = gstChConfig[usModeNum].u8PreambleLen ;
    gstTdoaInstData.stDwChannelCfg.u8RxPAC           = gstChConfig[usModeNum].u8PacSize ;
    gstTdoaInstData.stDwChannelCfg.u8TxCode          = gstChConfig[usModeNum].u8PreambleCode ;
    gstTdoaInstData.stDwChannelCfg.u8RxCode          = gstChConfig[usModeNum].u8PreambleCode ;
    gstTdoaInstData.stDwChannelCfg.u8NsSFD           = gstChConfig[usModeNum].u8NsSFD;
    gstTdoaInstData.stDwChannelCfg.u8DataRate        = gstChConfig[usModeNum].u8DataRate ;
    gstTdoaInstData.stDwChannelCfg.u8PhrMode         = DWT_PHRMODE_STD;
    gstTdoaInstData.stDwChannelCfg.u16SfdTO          = DWT_SFDTOC_DEF; //default value

	gstTdoaInstData.stDwChannelCfg.u8SmartPowerEn = 0;

    //configure the channel parameters
    memset(&stChannelCfg, 0, sizeof(stChannelCfg));
	memcpy(&stChannelCfg, &gstTdoaInstData.stDwChannelCfg, sizeof(dwt_config_t));

	//ͨ��ʵ�����ý����豸ͨ������
	dwt_configure(&stChannelCfg, iUseNvmData);

	//ͨ��ʵ����Ϣ���з������ߺ͹�������
    gstTdoaInstData.stDwInstTxCfg.u8PGdly = gstSpectrumTxConfig[gstChConfig[usModeNum].u8ChannelNumber].u8PGdelay ;
	//��ȡ����У׼ֵ ����ȡУ׼ֵʧ����ʹ��Ĭ��ֵ
	u32Txpower = dwt_getotptxpower(gstTdoaInstData.stDwChannelCfg.u8Prf, gstTdoaInstData.stDwChannelCfg.u8ChannelNum);
	if((u32Txpower == 0x0) || (u32Txpower == 0xFFFFFFFF)) //if there are no calibrated values... need to use defaults
    {
        u32Txpower = gstSpectrumTxConfig[gstChConfig[usModeNum].u8ChannelNumber].u32TxPwr[gstTdoaInstData.stDwChannelCfg.u8Prf - DWT_PRF_16M];
    }
	#if 0    //�ο�ֵ 0x751F1F75
	if(type ==1)
		Station_Power=0x75848475;  //8.0db
	else if(type ==2)
		Station_Power=0x75858575;  //8.5db
	else if(type==3)
		Station_Power=0x75878775;  //9.5db
	else if(type==4)
		Station_Power=0x75898975;  //10.5db //Ĭ��
	else if(type==5)
		Station_Power=0x758B8B75;  //11.5db
	else if(type==6)
		Station_Power=0x758D8D75;  //12.5db
	else if(type==7)
		Station_Power=0x758F8F75;  //13.5db
	else if(type==8)
		Station_Power=0x75919175;  //14.5db
	else if(type==9)
		Station_Power=0x75959575;  //16.5db
	else if(type==10)
		Station_Power=0x75999975;  //18.5db
	else
		return 0;
	#endif
	//���÷��͹���
	//���ʹ�����ܵ�Դ����ֱ��ʹ�ô�NVM��ȡ��ֵ
	//���ʹ�����ܵ�Դ���û���Ҫȷ��ÿ1ms������һ֡������Υ��TXƵ�׹���
	gstTdoaInstData.stDwInstTxCfg.u32Power = 0x751F1F75; //-42db
	dwt_setsmarttxpower(gstTdoaInstData.stDwChannelCfg.u8SmartPowerEn);
	//configure the tx spectrum parameters (power and PG delay)
    dwt_configuretxrf((dwt_txconfig_t *)&gstTdoaInstData.stDwInstTxCfg);

	//����Ƿ�ʹ�ô�NVM��ȡ�������ӳ�У׼ֵ
    if((iUseNvmData & DWT_LOADANTDLY) == 0)
    {
        gstTdoaInstData.u16AntennaTxDelay = gau16RfDelays[gstTdoaInstData.stDwChannelCfg.u8Prf - DWT_PRF_16M];
    }
    else
    {
        //��ȡOTPУ׼�����ȡ�������ӳ�
        gstTdoaInstData.u16AntennaTxDelay = dwt_readantennadelay(gstTdoaInstData.stDwChannelCfg.u8Prf) >> 1;

        // if nothing was actually programmed then set a reasonable value anyway
		if (gstTdoaInstData.u16AntennaTxDelay == 0)
		{
			gstTdoaInstData.u16AntennaTxDelay = gau16RfDelays[gstTdoaInstData.stDwChannelCfg.u8Prf - DWT_PRF_16M];
		}
    }
	
	// -------------------------------------------------------------------------------------------------------------------
	//���������ӳ٣����ý��պͷ��͵������ӳ���ͬ
	dwt_setrxantennadelay(gstTdoaInstData.u16AntennaTxDelay);
	dwt_settxantennadelay(gstTdoaInstData.u16AntennaTxDelay);
	PrintfUtil_vPrintf("u16AntennaTxDelay = %d \n", gstTdoaInstData.u16AntennaTxDelay);

	//�˴�ֻ��Ҫ����ʱ������ע����ʱ�� ���߷��ͽ���У׼�ݲ���ע
	
	return;
}

/*************************************************************
��������:TdoaInstanceBaseMsginit
��������:�豸ʵ��������Ϣ��ʼ��
����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstanceBaseMsginit(int iTdoaInstMode)
{
	int iTxSpeedType = INST_MODE_TX_SPEED_BUTT;

	//����ʵ������ȫ�ֳ�ʼ��
	memset(&gstTdoaInstData, 0, sizeof(TDOA_INSTANCE_DATA_S));	
	
	//��ʼ��ʵ��������Ϣ 
	gstTdoaInstData.stInstBaseData.eTdoaInstMode    = iTdoaInstMode;
	gstTdoaInstData.stInstBaseData.eTdoaInstState   = TDOA_INIT;
	gstTdoaInstData.stInstBaseData.eDwEventType     = DWT_SIG_RX_BUTT;

	gstTdoaInstData.stInstBaseData.u16OwnAddress    = DEV_ADDRESS_CARDID;
	gstTdoaInstData.stInstBaseData.u16SeqNum 	    = 0;
	gstTdoaInstData.stInstBaseData.u64RxTimeStamp   = 0;
	gstTdoaInstData.stInstBaseData.u8InstEventCnt   = 0;
	gstTdoaInstData.stInstBaseData.u8InstEvent[0]   = 0;
	gstTdoaInstData.stInstBaseData.u8InstEvent[1]   = 0;
	gstTdoaInstData.stInstBaseData.u8RxAutoreEnable = 0;
	gstTdoaInstData.stInstBaseData.i8Rssi 		    = 0;
	gstTdoaInstData.u16TxMsgLength                  = 0;
	gstTdoaInstData.u16RxMsgLength                  = 0;
	//gstTdoaInstData.u8AckExpected                 = 0;
	gstTdoaInstData.u8WaitAck                       = 0;
		
	//����ʵ���豸������������
	if(iTdoaInstMode == TDOA_INST_TAG_TEST)
	{
		iTxSpeedType = INST_MODE_TX_SPEED_SLOW;
	}
	else if (iTdoaInstMode == TDOA_INST_TAG_STANDARD)
	{
		iTxSpeedType = INST_MODE_TX_SPEED_QUICK;
	}
	
	//�����豸��������panid �����ǻ�վ��panid����Ϊ0xdeca ���ǻ�վ�ڵ���panidΪ0xeeee
	gstTdoaInstData.stInstBaseData.u16PanId 	  = TDOA_INST_PANID_TAG;
	if (iTdoaInstMode == TDOA_INST_ANCHOR)
	{	
		gstTdoaInstData.stInstBaseData.u16PanId 	  = TDOA_INST_PANID_ANCHOR; //���úͿ�ͬ������
	}
	
	gstTdoaInstData.stInstBaseData.eTxSpeedType   = iTxSpeedType;

	//�����жϻص�����ע��
    dwt_setcallbacks(TDOATxCallback, TDOARxGoodCallback); //zanshizhushi
    
	//���л�վ��ʼ��
	if (iTdoaInstMode == TDOA_INST_ANCHOR)
	{
	//	TdoaAnchorInit();
	}
	else if ((iTdoaInstMode == TDOA_INST_TAG_TEST) || (iTdoaInstMode == TDOA_INST_TAG_STANDARD))
	{
		//���д��⿨��ʼ��
	//	TdoaTagInit();
	}
	//�����豸���Ͷ�Ӧ�ⲿLED�豸���в�ͬ��������ʾ ��Ҫ����ʵ��Ӳ���޸�
	//dwt_setleds(iTdoaInstMode);

    return;
}

/*************************************************************
��������:TdoaStdCardMsgBuffInit
��������:ѧϰ������⿨��Ϣ�ṹ��ʼ��

����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaStdCardMsgBuffInit(void)
{
    uint8 u8StdCardCount = 0;
    TDOA_INST_SAVE_RSSI_MSG_S  stStdCardMsg;

    memset(&stStdCardMsg, 0, sizeof(TDOA_INST_SAVE_RSSI_MSG_S));
    //�����������ǰ�������ݻ�ȡ�쳣�������µ�һ������
    u8StdCardCount = gstTdoaStdCardMsg.u8StdCardCount;
    if ((u8StdCardCount-1) >= 0)
    {
        memcpy(&stStdCardMsg, &gstTdoaStdCardMsg.stStdCardMsg[u8StdCardCount-1], sizeof(TDOA_INST_SAVE_RSSI_MSG_S));
    }
    //��ʼ���洢��ѧϰ������⿨����Ϣ�ṹ
    memset(&gstTdoaStdCardMsg, 0, sizeof(TDOA_INST_STANDARD_CARD_MSG_S));

   if ((u8StdCardCount-1) >= 0)
    {
        memcpy(&gstTdoaStdCardMsg.stStdCardMsg[0], &stStdCardMsg, sizeof(TDOA_INST_SAVE_RSSI_MSG_S));
        gstTdoaStdCardMsg.stStdCardMsg[0].u8Dealtime = 0;
        gstTdoaStdCardMsg.u8StdCardCount = 1;
    } 
    
    return;
}

/*************************************************************
��������:TdoaBetterTableBuffInit
��������:����ѡ�����Ϣ�ṹ��ʼ��

����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaBetterTableBuffInit(void)
{
    memset(&gstBetterTable, 0, sizeof(TDOA_INST_BETTER_SELECT_TABLE_S));
    
    return;
}

/*************************************************************
��������:TdoaCardMsgBuffInit
��������:��վ�洢ѧϰ�����ߴ��⿨�Ļ�����Ϣ����Ϣ�ṹ

����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaCardMsgBuffInit(void)
{
    memset(&gstTdoaCardMsg, 0, sizeof(TDOA_INST_RXMSG_TO_CARDMSG_S));
    
    return;
}

/*************************************************************
��������:TdoaStdCardSaveRssiBuffInit
��������:��ʼ��ѧϰ���洢���⿨����Ϣ�ṹ

����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaStdCardSaveRssiBuffInit(void)
{
    memset(&gstTdoaSaveRssiMsg, 0, sizeof(TDOA_INST_SAVE_RSSI_MSG_S));
    
    return;
}

/*************************************************************
��������:TdoaMsgBuffInit
��������:��ʼ�����ݵĻ�����

����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaMsgBuffInit(int iTdoaInstMode)
{
    if (iTdoaInstMode == TDOA_INST_ANCHOR)
    {
    	//��ʼ�����������
        TdoaBuildPackBuffInit();
        //��ʼ����վ�洢ѧϰ������⿨��Ӧ��Ϣ�Ĵ洢�ṹ
        TdoaStdCardMsgBuffInit();       
        //��ʼ������ѡ���
        TdoaBetterTableBuffInit();   
        //��ʼ������ѧϰ���ʹ��⿨������Ϣ����Ϣ�ṹ
        TdoaCardMsgBuffInit();
    }
    else if (iTdoaInstMode == TDOA_INST_TAG_STANDARD)
    {
        //��ʼ��ѧϰ���洢���⿨Rssiֵ�Ĵ洢�ṹ
        TdoaStdCardSaveRssiBuffInit();  
    }

	return;
}

/*************************************************************
��������:AppInstanceInit
��������:�豸ʵ����ʼ��   
1.��ʱ����ע����ʱ��Ӱ�� 
2.��������������˫������
3.������Ӧ��ʱ��������

����˵��:void
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void AppInstanceInit(int iTdoaInstMode)
{
	//����ʵ������������
	TdoaInstanceBaseMsginit(iTdoaInstMode);
		
	//����ʵ��ͨ������ ��ǰĬ��ʹ��0ͨ��
	TdoaInstChannelConfig(TDOA_CHANNEL_CONFIG_MODE_ONE);

	//ȥʹ���豸���������ջ����� �ر��˽��գ����߼���Ҫ�޸ģ�����Ӧ��һֱ���� �п��ܵ����豸���ߺ󱻻���
	//Appsleepinit(); 
	//���л�������ʼ��
	TdoaMsgBuffInit(iTdoaInstMode);
    
	//���ÿ��ķ�������,������Ϣ��ѯ�����¼�
    DecaTdoaEquipEventSet();

	return;
}
#endif

#if IF_DESC(ע:TDOA�����򴮿ڷ���)
/***********************************************************************
����:TdoaMsgPackInsetUartBuff �����ݷ��ͳ�ȥ������������ݹ���ֱ�ӷ��ͳ�ȥ�����Ƚ�С
	����뵽pack�д���һ����ʱ���ͳ�ȥ
����:
	pack :�����͵İ��ռ�
	msg_array :�����뵽pack������
	time :����pack���Ĵ���
����: ��
***********************************************************************/
void TdoaMsgPackInsetUartBuff(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray)
{
	int iLoop = 0;
	uint16 u16TestCardNum = 0;
	u16TestCardNum = pstTdoaMsgArray->u16TdoaMsgNum; //��ȡ�ÿ췢����������а����Ĵ��⿨����
	TDOA_UWB_MSG_PACK_SEND_S* pstTdoaSendPack =  TdoaGetLocalSendPackStructurePtr();

	//�жϸ������Ƿ��Ƿ���Է���
	if (pstTdoaMsgArray->u16MsgSendFlag != TRUE)
	{
		return;
	}
	
	//���з������ݽṹ��ʼ��
	 TdoaDataBuffClear();
	
	for (iLoop = 0; iLoop < u16TestCardNum; iLoop++)
	{ 
		memcpy(&pstTdoaSendPack->stTdoaMsg[iLoop], &pstTdoaMsgArray->stTdoaMsg[iLoop], sizeof(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S));
		pstTdoaSendPack->u16PackSendCount++;
	}
    
	//if(gTdoaSendPack.u16PackSendCount > 30)  //ԭ����Ҫ������ֵ����30���ܴ��������¼� ��ǰ��ֻҪ������������д������ڼ����д��ڷ�������
		event_timer_set(EVENT_UART_SEND);
	
	TdoaClearMsgArray(pstTdoaMsgArray);            //��ոĻ�׼��ǩ��Ԫ��ԭ���ѷ��͵�����

	return;
}

/*************************************************************
��������:TdoaDataBuffClear
��������:���з��͵����ڵ����ݰ���ʼ��
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaDataBuffClear(void)
{
	int i=0;
	TDOA_UWB_MSG_PACK_SEND_S* pstTdoaSendPack =  TdoaGetLocalSendPackStructurePtr();

	for (i = 0; i < TDOA_PACK_SEND_MAX_NUM; i++)
	{
		memset(&pstTdoaSendPack->stTdoaMsg[i], 0, sizeof(TDOA_UWB_TIMESTAMP_PACK_S));
	}
	pstTdoaSendPack->u16PackSendCount=0;

	return;
}

/*************************************************************
��������:TdoaSendCardReportToUart
��������:���������������Сstm32����stm32����
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaSendCardReportToUart(void)
{
	uint16 i = 0;
	uint16 j = 0;
	uint16 u16SendGroupNum  = 0; //�������
	uint16 u16MaxSendNum    = 0; //����һ��������͵����ݶ�Ӧ���⿨����
	uint16 u16TestTagNum    = 0; //��Ϣ���еĴ��⿨����
	uint16 u16GroupStartNum = 0; //��ǰ���ڷ������������ʼ��ַ
	uint16 u16SendDataLen   = 0; //���ڷ��͵����ݳ��� �������ݳ��� + Э��ͷ����
	APP_UWB_TDOA_DITANCE_S stAppTdoaData;

	TDOA_UWB_MSG_PACK_SEND_S* pstTdoaSendPack =  TdoaGetLocalSendPackStructurePtr();

	//��ȡ��ǰ������Ϣ���еĴ��⿨����
	u16TestTagNum = pstTdoaSendPack->u16PackSendCount;

	//��û�д��⿨���������½��д��ڷ���
	if(u16TestTagNum <= 0)
	{
		return;
	}
    
	/* ���ڴ���Ϊ���ֽڷ��ͣ���һ���Է������ݹ���ᵼ�´��ڷ��ͱ��������Դ˴��������ݳ��ȼ�飬
	   ����Ϣ���д��ڵĴ��⿨��Ϣ����8������Ҫ���з����˷�����Ϣ���ܱ�֤���ڷ��͵İ�ȫ��
	   ÿ����෢��8�Ŵ��⿨����Ϣ
	*/
	u16MaxSendNum = TDOA_UART_SEND_MAX_TAG_NUM;
	u16SendDataLen = u16MaxSendNum * sizeof(APP_UWB_TDOA_TIMESTAMP_S) + sizeof(APP_HEADER_S);
	while(u16TestTagNum > u16MaxSendNum)
	{
		u16GroupStartNum = u16SendGroupNum * u16MaxSendNum;
		u16SendGroupNum ++; //�����������
		
		//���д��ڷ�����������װ ÿ����װ8�Ŵ��⿨�����ݽ��з���
		for (i = u16GroupStartNum, j = 0; 
		    (i < u16GroupStartNum + u16MaxSendNum) && (i < u16TestTagNum); 
			 i++, j++)
		{
			memcpy(&stAppTdoaData.stAppTdoaMsg[j], &pstTdoaSendPack->stTdoaMsg[i], sizeof(APP_UWB_TDOA_TIMESTAMP_S));
		}
        
        //���Э������ͷ
        UwbBsmacBlidPacketHeadProc(APP_PROTOCOL_TYPE_UWB_CARD, APP_UWB_MSG_TDOA, u16SendDataLen - sizeof(APP_HEADER_S), &stAppTdoaData.stAppHead);

        //���д�����Ϣ����ǰ�ĸ�ʽ��װ ÿ����װ�Ļ�վ����ͬ������ֻȡ��һ����Ϣ�еĵ�һ����վ����Ϣ
    	UwbBsmacSendPacketProc((uint8 *)(&stAppTdoaData), stAppTdoaData.stAppTdoaMsg[0].u16StationID, u16SendDataLen, BSMAC_FRAME_TYPE_DATA);

		//��������δ���͵Ĵ��⿨���� 
		u16TestTagNum -= u16MaxSendNum;
	}

	//����Ƿ�����δ���͵Ĵ��⿨���� ���Ѿ���������ֱ�ӽ��з���
	if(u16TestTagNum == 0)
	{
		return;
	}
	
	//�����з��鷢�ͺ����д��⿨����δ���� ����Ҫ������ȷƫ�ƻ�ȡδ���͵Ĵ��⿨����
	u16GroupStartNum = u16SendGroupNum * u16MaxSendNum;  //��û�н��з��鷢�ʹ˹�ʽ��ƫ��Ҳ��ȷ
	//��ʣ��Ĵ��⿨���ݽ�����װ
	for (i = u16GroupStartNum, j = 0; 
	     i < u16TestTagNum; 
		 i++, j++)
	{
		memcpy(&stAppTdoaData.stAppTdoaMsg[j],&pstTdoaSendPack->stTdoaMsg[u16GroupStartNum+i],sizeof(APP_UWB_TDOA_TIMESTAMP_S));
	}
		 
	//���ڷ��͵����ݳ���	 
	u16SendDataLen =  u16TestTagNum * sizeof(APP_UWB_TDOA_TIMESTAMP_S) + sizeof(APP_HEADER_S);

    //���Э������ͷ
    UwbBsmacBlidPacketHeadProc(APP_PROTOCOL_TYPE_UWB_CARD, APP_UWB_MSG_TDOA, u16SendDataLen - sizeof(APP_HEADER_S), &stAppTdoaData.stAppHead);

    //���д�����Ϣ����ǰ�ĸ�ʽ��װ ÿ����װ�Ļ�վ����ͬ������ֻȡ��һ����Ϣ�еĵ�һ����վ����Ϣ
	UwbBsmacSendPacketProc((uint8 *)(&stAppTdoaData), stAppTdoaData.stAppTdoaMsg[0].u16StationID, u16SendDataLen, BSMAC_FRAME_TYPE_DATA);

	TdoaDataBuffClear();
	//gu8NblinkSend = 1;

#if 1
//#ifdef	 PRINTF_vReportCardDistance_SendData
	/*************************������Ϣ��ط���*****************************/
    TDOA_INST_BETTER_SELECT_TABLE_S* pstBetterStdCardTable = TdoaGetLocalBetterTableStructurePtr();

    //ѭ����ȡ��Ӧ�Ĵ��⿨��Ӧ��ѧϰ�����յ�������ֵ
    for (i = 0; i < pstBetterStdCardTable->u8CardCount; i++)
    {
        if (pstBetterStdCardTable->stBetterTable[i].u16TestCardId == stAppTdoaData.stAppTdoaMsg[0].u16TestCardID)
        {
            stAppTdoaData.stAppTdoaMsg[0].i8Rssi = pstBetterStdCardTable->stBetterTable[i].i8Rssi;
            break;
        }
    }
    
    for (i = 0; i < u16TestTagNum; i++)
    {
		PrintfUtil_vPrintf("%d %d,%d,%d,%d,%i %d\n", 
        u16TestTagNum,
		stAppTdoaData.stAppTdoaMsg[0].u16StationID,
		stAppTdoaData.stAppTdoaMsg[0].u16TestCardID,
		stAppTdoaData.stAppTdoaMsg[0].u16StandardCardID,
		stAppTdoaData.stAppTdoaMsg[0].u8Status,
		stAppTdoaData.stAppTdoaMsg[0].i8Rssi,
		stAppTdoaData.stAppTdoaMsg[0].u16Cardseqnum);
    }

#endif

    return;
}

#endif


/* ==========================================================

Notes:

Previously code handled multiple instances in a single console application

Now have changed it to do a single instance only. With minimal code changes...(i.e. kept [instance] index but it is always 0.

Windows application should call instance_init() once and then in the "main loop" call instance_run().

*/
