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


// -------------------------------------------------------------------------------------------------------------------


TDOA_INSTANCE_DATA_S gstTdoaInstData;
TDOA_INST_RXMSG_TO_CARDMSG_S gstTdoaCardMsg;
TDOA_UWB_TIMESTAMP_MSG_ARRAY_S gstTdoaMsgArray[TDOA_STANDCARD_MAX_NUM];
extern TDOA_UWB_MSG_PACK_SEND_S gTdoaSendPack;
uint16 u16HdlTxFrameCnt = 0;

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
	3,              // channel
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
	uint8 au8RxTimeStamp[5]  = {0, 0, 0, 0, 0};
    uint32 u32RxTimeStampTemp = 0;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

	event_timer_set(EVENT_RAGING_REPORT);
		
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
	
#if 1
	if (pstInstMsg->stRxMsgFromDw.u8MessageData[0] == 33)
	{	
		uint16 u16Seqnum = 0;
		uint16 u16SrcAddr = 0;
		uint16 u16DestAddr = 0;

		memcpy(&u16SrcAddr, &(pstInstMsg->stRxMsgFromDw.u8SourceAddr), ADDR_BYTE_SIZE_S);		
		memcpy(&u16DestAddr, &(pstInstMsg->stRxMsgFromDw.u8DestAddr), ADDR_BYTE_SIZE_S);		

		u16SrcAddr  = u16SrcAddr & 0xffff;
		u16DestAddr = u16DestAddr & 0xffff;
		
		u16Seqnum = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_H] << 8;		  //get the seqnum
		u16Seqnum += pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_L];

	//	PrintfUtil_vPrintf("100001\n");
#if 0
//		PrintfUtil_vPrintf("dw1000_new u16SrcAddr = %d to u16DesAddress = %d  u8RxEvent = %d datalength = %d	u8MessageData[0] = %d u16Seqnum = %d TDOARxGoodCallback\n", 
//						u16SrcAddr, pstInstMsg->stInstBaseData.u16OwnAddress, u8RxEvent, pstRxCallBackData->datalength, pstInstMsg->stRxMsgFromDw.u8MessageData[0], u16Seqnum);
		PrintfUtil_vPrintf("\n/************ TDOARxGoodCallback RxData **************/\n", pstInstMsg->stRxMsgFromDw.u8FrameCtrl[0]);
		PrintfUtil_vPrintf("u8FrameCtrl[0] =  %d\n", pstInstMsg->stRxMsgFromDw.u8FrameCtrl[0]);
		PrintfUtil_vPrintf("u8FrameCtrl[1] =  %d\n", pstInstMsg->stRxMsgFromDw.u8FrameCtrl[1]);
		PrintfUtil_vPrintf("u8SeqNum       =  %d\n", pstInstMsg->stRxMsgFromDw.u8SeqNum);
		PrintfUtil_vPrintf("u8PanID[0]     =  %d\n", pstInstMsg->stRxMsgFromDw.u8PanID[0]);
		PrintfUtil_vPrintf("u8PanID[1]     =  %d\n", pstInstMsg->stRxMsgFromDw.u8PanID[1]);
		PrintfUtil_vPrintf("u8DestAddr     =  %d\n", u16DestAddr);
		PrintfUtil_vPrintf("u8SourceAddr   =  %d\n", u16SrcAddr);
		PrintfUtil_vPrintf("u8MessageData[0] =  %d\n", pstInstMsg->stRxMsgFromDw.u8MessageData[0]);
		PrintfUtil_vPrintf("u8MessageData[1] =  %d\n", pstInstMsg->stRxMsgFromDw.u8MessageData[1]);
		PrintfUtil_vPrintf("u8MessageData[2] =  %d\n", pstInstMsg->stRxMsgFromDw.u8MessageData[2]);
		PrintfUtil_vPrintf("u8MessageData[3] =  %d\n", pstInstMsg->stRxMsgFromDw.u8MessageData[3]);
		PrintfUtil_vPrintf("u8MessageData[4] =  %d\n", pstInstMsg->stRxMsgFromDw.u8MessageData[4]);
		PrintfUtil_vPrintf("u8MessageData[5] =  %d\n", pstInstMsg->stRxMsgFromDw.u8MessageData[5]);
		PrintfUtil_vPrintf("u8MessageData[6] =  %d\n", pstInstMsg->stRxMsgFromDw.u8MessageData[6]);
		PrintfUtil_vPrintf("u16Seqnum        =  %d\n", u16Seqnum);
		PrintfUtil_vPrintf("u8Fcs[0]         =  %d\n", pstInstMsg->stRxMsgFromDw.u8Fcs[0]);
		PrintfUtil_vPrintf("u8Fcs[1]         =  %d\n", pstInstMsg->stRxMsgFromDw.u8Fcs[1]);
		PrintfUtil_vPrintf("\n/************ TDOARxGoodCallback RxData end **************/\n", pstInstMsg->stRxMsgFromDw.u8FrameCtrl[0]);
#endif
	}
 #endif	

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
	//PrintfUtil_vPrintf("100004\n");

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
		PrintfUtil_vPrintf("the TdoaRxCardMsgProc Speedtype was wrong u16Speedtype = %d\n", pstCardRxMsg->u16Speedtype);
	}
	//PrintfUtil_vPrintf("100005\n");
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
			//PrintfUtil_vPrintf("100006\n");
		}
	}

	return;
}

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
//    port_set_dw1000_slowrate();  //max SPI before PLLs configured is ~4M

	//���Ի�ȡDW1000�豸ID������ȡʧ�����Ի��Ѳ���
	u32DevID = dwt_readdevid() ;
	PrintfUtil_vPrintf("u32DevID = %d Dw1000Init 1111\n", u32DevID);
	if(DWT_DEVICE_ID != u32DevID) //if the read of device ID fails, the DW1000 could be asleep
	{
//		port_wakeup_dw1000();

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
	
//	port_set_dw1000_fastrate();

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

	pstInstMsg->stInstBaseData.u16PanId = 0xeeee ;//;0xdeca  0xdddd 0xeeee
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
	dwt_setautorxreenable(pstInstMsg->stInstBaseData.u8RxAutoreEnable);

	//���ý�����˫������
     #if (DOUBLE_RX_BUFFER == 1)
		dwt_setdblrxbuffmode(0); //enable double RX buffer
     #endif

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
	//���÷��͹���
	//���ʹ�����ܵ�Դ����ֱ��ʹ�ô�NVM��ȡ��ֵ
	//���ʹ�����ܵ�Դ���û���Ҫȷ��ÿ1ms������һ֡������Υ��TXƵ�׹���
	gstTdoaInstData.stDwInstTxCfg.u32Power = 0x751F1F75;
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
		gstTdoaInstData.stInstBaseData.u16PanId 	  = TDOA_INST_PANID_TAG; //���úͿ�ͬ������
	}
	
	gstTdoaInstData.stInstBaseData.eTxSpeedType   = iTxSpeedType;

	//�����жϻص�����ע��
    dwt_setcallbacks(TDOATxCallback, TDOARxGoodCallback); //zanshizhushi
    
	//���л�վ��ʼ��
	if (iTdoaInstMode == TDOA_INST_ANCHOR)
	{
		TdoaAnchorInit();
	}
	else if ((iTdoaInstMode == TDOA_INST_TAG_TEST) || (iTdoaInstMode == TDOA_INST_TAG_STANDARD))
	{
		//���д��⿨��ʼ��
		TdoaTagInit();
	}
	//�����豸���Ͷ�Ӧ�ⲿLED�豸���в�ͬ��������ʾ ��Ҫ����ʵ��Ӳ���޸�
	//dwt_setleds(iTdoaInstMode);

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
	(void)Dw1000Init();  //Dw1000ģ���ʼ�� ��ʱ���ⲻӰ���������̱��룬������Ҫ�ŵ�board.c��

	//����ʵ������������
	TdoaInstanceBaseMsginit(iTdoaInstMode);
		
	//����ʵ��ͨ������ ��ǰĬ��ʹ��0ͨ��
	TdoaInstChannelConfig(TDOA_CHANNEL_CONFIG_MODE_ONE);

	//ȥʹ���豸���������ջ����� �ر��˽��գ����߼���Ҫ�޸ģ�����Ӧ��һֱ���� �п��ܵ����豸���ߺ󱻻���
	//Appsleepinit(); 
	//��ʼ�����������
  //   TdoaBuildPackBuffInit();
	
	//���ÿ��ķ�������,������Ϣ��ѯ�����¼�
  //  DecaTdoaEquipEventSet();

	//����У��
	//Tdoa_check_data();

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
	//���÷���֡��panid �����豸����Ҫ��ͬһpanid�����й���
	pstInstMsg->stTxMsgToPack.u8PanID[0] = (pstInstMsg->stInstBaseData.u16PanId) & 0xff;  //main station
	pstInstMsg->stTxMsgToPack.u8PanID[1] = (pstInstMsg->stInstBaseData.u16PanId >> 8) & 0xff;	

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
        dwt_setdelayedtrxtime(iTxDelayed) ;
    }
	
	//�ж���Ϣ���ͺ��Ƿ���Ҫ���еȴ���Ϣȷ�� poll��Ϣ��������Ϣȷ��
    if(pstInstMsg->u8WaitAck)
    {
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

	//�����豸�����������÷��ͻ�����
	if (pstInstMsg->stInstBaseData.eTxSpeedType == INST_MODE_TX_SPEED_SLOW)
	{
		pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SPEED_TYPE_BIT] = TDOA_INST_MODE_SPEED_SLOW;
	}
	else if (pstInstMsg->stInstBaseData.eTxSpeedType == INST_MODE_TX_SPEED_QUICK)
	{
		pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SPEED_TYPE_BIT] = TDOA_INST_MODE_SPEED_QUCIK;
	}

	//������Ϣ�����к�
	pstInstMsg->stInstBaseData.u16SeqNum ++;
	pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_H] = (pstInstMsg->stInstBaseData.u16SeqNum >> 8) & 0xff;
	pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_L] = (pstInstMsg->stInstBaseData.u16SeqNum) & 0xff;
	pstInstMsg->stTxMsgToPack.u8SeqNum = (uint8)pstInstMsg->stInstBaseData.u16SeqNum;
		
	//����ʵ�����͵�Ŀ�ĵ�ַ �˺���ֻ�д��⿨�Ϳ췢���ᴥ�� Ŀ�ĵ�ַ��������Ϊ�㲥ȫf 
	pstInstMsg->stInstBaseData.u16DestAddress = TDOA_INST_SEND_POLL_DEST_ADDR;
	//pstInstMsg->stTxMsgToPack.u8DestAddr = TDOA_INST_SEND_POLL_DEST_ADDR;

	//���豸mac�����÷���֡���ݼ����� ����ƼĴ�����ctrl[1]��Ӧ
	TdoaSetMacFrameData(pstInstMsg, 6, RTLS_DEMO_MSG_TAG_POLL, !TDOA_INST_ACK_REQUESTED);

	//������Ϣ����
	if(TdoaInstSendTagPoolPacket(pstInstMsg, DWT_START_TX_IMMEDIATE))
	{
		//������ʧ�ܣ��򴥷���ѯ�����¼����ⲿ����������ѯ�˴�����Ҫ�ٴ�����

		//��ǰ���кŽ��м�һ����
		pstInstMsg->stInstBaseData.u16SeqNum --;

	}
	
	//���ͳɹ�����������ʾ
	dwt_setleds(2);


	uint16 u16SeqnumData;
		
	u16SeqnumData = pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_H] << 8; 	  //get the seqnum
	u16SeqnumData += pstInstMsg->stRxMsgFromDw.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_L];
	uint32 systicks = portGetTickCnt();

	PrintfUtil_vPrintf("poll u16SeqNum = %d u16OwnAddress = %d u16DestAddress = %d systicks = %d \n", 
		pstInstMsg->stInstBaseData.u16SeqNum, 
		pstInstMsg->stInstBaseData.u16OwnAddress, 
		pstInstMsg->stInstBaseData.u16DestAddress,
		systicks);

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
/*
	PrintfUtil_vPrintf("u16CardId = %d u16OwnAddress = %d u16Seqnum = %d\n",
		gstTdoaCardMsg.u16CardId, 
		pstInstMsg->stInstBaseData.u16OwnAddress,
		u16Seqnum);
*/
	return;
}

/*************************************************************
��������:TdoaInstRxMsgToCardMsgProcNew
��������:��ʵ�����ɽ�������������ȡ�����ݸ�ֵ������ȫ�ֻ�������Ϊ���������׼��

����˵��:void
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaInstRxMsgToCardMsgProcNew(void)
{
	uint16	u16SrcAddr;
	uint16	u16Seqnum;
	//TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	instance_data_t* pstInstMsg = TdoaGetOldLocalInstStructurePtr();

	//��ȡ�豸������ֵ
	rx_power_lever(pstInstMsg);
	
	// ��ȡ������Ϣ��Դ��ַ
	memcpy(&u16SrcAddr, &(pstInstMsg->rxmsg.sourceAddr), ADDR_BYTE_SIZE_S);
	u16Seqnum = pstInstMsg->rxmsg.messageData[TDOA_INST_FRAME_SEQNUM_BIT_H] << 8;		  //get the seqnum
	u16Seqnum += pstInstMsg->rxmsg.messageData[TDOA_INST_FRAME_SEQNUM_BIT_L];
	
	PrintfUtil_vPrintf("u16SrcAddr = %d u16Seqnum = %d\n", 
						u16SrcAddr, u16Seqnum);
	gstTdoaCardMsg.u16CardId = u16SrcAddr & 0xffff;
	gstTdoaCardMsg.u16Seqnum = u16Seqnum;
	gstTdoaCardMsg.u16Speedtype = pstInstMsg->rxmsg.messageData[TDOA_INST_FRAME_SPEED_TYPE_BIT];
	gstTdoaCardMsg.u64RxCardTimestamp = pstInstMsg->rxu.rxTimeStamp;
	gstTdoaCardMsg.i8Rssi = pstInstMsg->i8rssi;


	//����ʮ���TDOA�Ĳ��,����ʮ�����ڷ���TDOA poll�����в�� u8TdoaRxMsgCount

/*
	PrintfUtil_vPrintf("u16CardId = %d u16OwnAddress = %d u16Seqnum = %d\n",
		gstTdoaCardMsg.u16CardId, 
		pstInstMsg->stInstBaseData.u16OwnAddress,
		u16Seqnum);
*/
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

	//PrintfUtil_vPrintf("iTdoaInstMode = %d eTdoaInstState = %d iDwEventType = %d\n",
	//	iTdoaInstMode, iTdoaInstState, iDwEventType);
	//PrintfUtil_vPrintf("100002 iTdoaInstState = %d iDwEventType = %d iRxMsgFrameType = %d\n", iTdoaInstState, iDwEventType, iRxMsgFrameType);

	//�ж�ʵ����ǰ����״̬
    switch (iTdoaInstState)
    {
    	case TDOA_INIT: //��ʼ��״̬���и����豸���ͷֱ��������
			switch(iTdoaInstMode)
			{
				case TDOA_INST_TAG_TEST:
				case TDOA_INST_TAG_STANDARD:
				//	TdoaTagInit();
					break;
					
				case TDOA_INST_ANCHOR:
					//TdoaAnchorInit();
				//	PrintfUtil_vPrintf("TDOA_INST_ANCHOR iTdoaInstMode = %d eTdoaInstState = %d iDwEventType = %d\n",
				//		iTdoaInstMode, iTdoaInstState, iDwEventType);

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
						case RTLS_DEMO_MSG_TAG_POLL:
							if (iTdoaInstMode == TDOA_INST_ANCHOR)
							{
								//PrintfUtil_vPrintf("100003\n");
								//��ʵ�����ɽ�������������ȡ�����ݸ�ֵ������ȫ�ֻ�����
								TdoaInstRxMsgToCardMsgProc();
								//�޸�ʵ��״̬
								pstInstMsg->stInstBaseData.eTdoaInstState = TDOA_RX_WAIT_DATA;
								//������ݻ���������û�н���ʱʹ����һ�����ݽ��д���
								memset(&pstInstMsg->stRxMsgFromDw, 0, sizeof(TDOA_DW_TRX_MSG_S));
							//	pstInstMsg->stInstBaseData.eDwEventType = DWT_SIG_RX_NOERR; //�ı����״̬���ȴ���һ�ν���
								//�����豸��������¼�
								event_timer_set(EVENT_CHECKTDOA_REVMSG_EVENT); 
							}
							break;

		                default: //������������д���
							//�����豸���� ��������ʱ�Ϳ�ʼ���� �����Զ�����
							// PrintfUtil_vPrintf("other RxMsgFrameType\n");
							dwt_rxenable(0); 
			                break;
							
					}
					break;
					 
                default: //������������д���
					//�����豸���� ��������ʱ�Ϳ�ʼ���� �����Զ�����
					PrintfUtil_vPrintf("other DwEventType\n");
					dwt_rxenable(0); 
	                break;
			}
			break;

			default: //������������д���
				PrintfUtil_vPrintf("other TdoaInstState\n");
				if (TDOA_INST_ANCHOR == TDOA_INST_ANCHOR)
				{
					dwt_rxenable(0); 	
				}
				break;
	}

	return;
}

#if 1

/*************************************************************
��������:TdoaWriteDataToStm32
��������:��������stm���ش���оƬ���ڷ���
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaWriteDataToStm32(uint8* pTdoaPackBuf, uint16 u16TdoaPackLen)
{
	uint8 i = 0;
	
	//�������ݺ�����У��
	if(pTdoaPackBuf == NULL || u16TdoaPackLen == 0)
	{
		return;
	}
	
	PrintfUtil_vPrintf(" write len = %d\n", u16TdoaPackLen);
	for (i = 0; i < u16TdoaPackLen; i++)
	{
		USART_SendData(USART1,(unsigned char)( *(pTdoaPackBuf + i))); /* Loop until the end of transmission */

		//PrintfUtil_vPrintf(" write \n");
		while (USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET);
	}
}

/*************************************************************
��������:TdoaBsmacBuildPacketMacHead
��������:���д��ڷ������ݱ���Macͷ������װ
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaBsmacBuildPacketMacHead(TDOA_BSMAC_PACKET_HEADER_S *pstTdoaBsmacHead, 
								 uint16 u16TdoaDataLen, 
								 uint8 u8FrameType)
{
	uint16 u16StationPanId; //�ݲ���ע����ֵ
	uint16 u16HdlRxFrameCnt;

	//����Ŀ���ַ��Դ��ַ���
	pstTdoaBsmacHead->u8SrcAddrH = (u16StationPanId >> 8) & 0xff;
	pstTdoaBsmacHead->u8SrcAddrL = (u16StationPanId) & 0xff;			  // source mac address
	pstTdoaBsmacHead->u8DstAddrH = 0;									  // dst address is usel
	pstTdoaBsmacHead->u8DstAddrL = 0;
	
	//���д��ڱ���mac������Ϣͷ���
	pstTdoaBsmacHead->u8PreambleH = BSMAC_PREAMBLE_H; 
	pstTdoaBsmacHead->u8PreambleL = BSMAC_PREAMBLE_L; 
	
	BSMAC_SET_DEVICETYPE(pstTdoaBsmacHead->u8frameControl, BSMAC_DEVICE_TYPE_LOC);     // I am location module
	BSMAC_SET_RDY(pstTdoaBsmacHead->u8frameControl, 1);								   // always ready
	BSMAC_SET_FRAMETYPE(pstTdoaBsmacHead->u8frameControl, u8FrameType);
	BSMAC_SET_PRIORITY(pstTdoaBsmacHead->u8frameControl, 1);
	
	//���ݲ�ͬ��֡���ͽ���macͷ���ݵķ�װ
	if (u8FrameType == BSMAC_FRAME_TYPE_ACK) // for ack, use recieved frame_cnt
	{
		//��Ӧ֡���ݳ���
		pstTdoaBsmacHead->u8FrameCountH = (u16HdlRxFrameCnt & 0xff00) >> 8;
		pstTdoaBsmacHead->u8FrameCountL = u16HdlRxFrameCnt  & 0xff;
	}
	else
	{
		//����֡���ݳ���
		pstTdoaBsmacHead->u8FrameCountH = ( u16HdlTxFrameCnt & 0xff00) >> 8; // framecnt_h
		pstTdoaBsmacHead->u8FrameCountL = u16HdlTxFrameCnt& 0xff; 			// framecnt_l
		u16HdlTxFrameCnt++;
	}
		
	return;	
}
/*************************************************************
��������:TdoaBsmacBuildPacketNetHead
��������:���д��ڷ������ݱ�������ͷ������װ
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaBsmacBuildPacketNetHead(TDOA_NET_PACKET_HEADER_S *pstTdoaNetHead, 
                                 uint16 u16TdoaDataLen, 
                                 uint16 u16NetType)
{
	uint16 u16ArmId = 0xFFFF; //�㲥

	//���д��ڱ������粿����Ϣͷ���
	pstTdoaNetHead->u8Type = u16NetType; //������������
	pstTdoaNetHead->u8Ttl  = 1; 
	pstTdoaNetHead->u16Src  = gstTdoaInstData.stInstBaseData.u16OwnAddress; 
	pstTdoaNetHead->u16Dst  = u16ArmId; 
	pstTdoaNetHead->u16Len  = u16TdoaDataLen; 

	return;
}

/*************************************************************
��������:TdoaBsmacBuildPacketData
��������:���а����������װ
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaBsmacBuildPacketData(uint8* pAppTdoaData, 
							  uint16 u16TdoaDataLen, 
							  uint8  u8FrameType,
							  TDOA_BSMAC_BUILD_PACK_S *pstTdoaBsmacPack,
							  uint16 *pu16TdoaDataTxLen)
{
	uint16 u16TxLen = 0;
	uint16 u16Crc   = 0;
	uint8*  pbuf;
	
	/* �������ݼ�⣬��Ϊack֡�����򲻽��д��� ack do not need payload, Live may have payload */
    if (u8FrameType != BSMAC_FRAME_TYPE_ACK)
    {
        memcpy((void*)pstTdoaBsmacPack->u8PackDataBuff, pAppTdoaData, u16TdoaDataLen);
    }

    //������������У׼ LIVE packet needs to be a long frame
    if (u8FrameType == BSMAC_FRAME_TYPE_LIVE)
    {
        u16TdoaDataLen = BSMAC_MAX_TX_PAYLOAD_LEN;
    }
    else if(u8FrameType == BSMAC_FRAME_TYPE_ACK)
    {
        u16TdoaDataLen = 0;
    }

	//��¼��ǰ�����͵����ݳ��� У�鳤�Ȳ�����bmacͷ���ȣ�
	//���շ���ǰ��������bamcͷ���ȱ�֤���ݷ�����ȷ
    u16TxLen = u16TdoaDataLen + BSMAC_FOOTER_LEN + sizeof(TDOA_NET_PACKET_HEADER_S); // length = payload+footer
	pstTdoaBsmacPack->stBsmacPackHead.u8DataLenH = (u16TxLen >> 8) & 0xff; //
	pstTdoaBsmacPack->stBsmacPackHead.u8DataLenL = u16TxLen & 0xff; //
	
	pbuf = (uint8*)pstTdoaBsmacPack; //����crcУ��ǰ�Ƚ��е��ֽ�ǿת�ٽ���ƫ�ƣ�
									 //�������ڶ��������ַ��ƫ�ƴ��󣬶�Ӧ���ݻ����
	//����CRCУ��λ��¼ ���з������ݵ�У��
    u16Crc = CRC16((unsigned char *)(pbuf + 2), 
	  			    u16TdoaDataLen + BSMAC_HEADER_LEN + sizeof(TDOA_NET_PACKET_HEADER_S) - 2, 
	  			    0xffff);   // caculate header and payload   
	  			    
	//�˴�Ϊ�˱�����ԭ�н���һֱ�������ṹ���Ա
	pbuf[u16TdoaDataLen + BSMAC_HEADER_LEN+sizeof(TDOA_NET_PACKET_HEADER_S)] = (u16Crc >> 8) & 0xff;
	pbuf[u16TdoaDataLen + BSMAC_HEADER_LEN+sizeof(TDOA_NET_PACKET_HEADER_S) + 1] = u16Crc & 0xff;
	
	//���з������ݳ��ȸ���
	*pu16TdoaDataTxLen = sizeof(bsmac_header_t) + u16TxLen;

	return;
}

/*************************************************************
��������:TdoaBsmacBuildPacket
��������:��TDOA����ͨ�����ڷ��͸����ش���stmоƬ
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
uint8 TdoaBsmacBuildPacket( uint8 * pAppTdoaData,
                            uint16  u16TdoaDataLen,
                            uint8   u8FrameType)
{
	uint16 u16TxLen = 0;
	TDOA_BSMAC_BUILD_PACK_S stTdoaBsmacPack;

    // �������ݳ�����Ч��У��
    if ((u16TdoaDataLen == 0) || (u16TdoaDataLen > 512) || (pAppTdoaData == NULL))
    {
        EDBG(PrintfUtil_vPrintf("Build Failed pbuf %X len%d\n", pAppTdoaData, u16TdoaDataLen);)
        return 0;
    }
	
	//���нṹ���ݳ�ʼ��
	memset(&stTdoaBsmacPack, 0, sizeof(stTdoaBsmacPack));
	
	//������Ϣ������ͷ������װ
	TdoaBsmacBuildPacketNetHead(&stTdoaBsmacPack.stNetPackHead, u16TdoaDataLen, NWK_DATA);
	
	//������Ϣ��Macͷ������װ
	TdoaBsmacBuildPacketMacHead(&stTdoaBsmacPack.stBsmacPackHead, u16TdoaDataLen, u8FrameType);

	//���а�������װ
	TdoaBsmacBuildPacketData(pAppTdoaData, u16TdoaDataLen, u8FrameType, &stTdoaBsmacPack, &u16TxLen);

	//�������ݰ�����
    TdoaWriteDataToStm32((void*)&stTdoaBsmacPack, u16TxLen);
	
    return u16TxLen;
}

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

	//�жϸ������Ƿ��Ƿ���Է���
	if (pstTdoaMsgArray->u16MsgSendFlag != TRUE)
	{
		return;
	}
	
	//���з������ݽṹ��ʼ��
	TdoaDataBuffClear();
	
	for (iLoop = 0; iLoop < u16TestCardNum; iLoop++)
	{ 
		memcpy(&gTdoaSendPack.stTdoaMsg[iLoop], &pstTdoaMsgArray->stTdoaMsg[iLoop], sizeof(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S));
		gTdoaSendPack.u16PackSendCount++;
	}
	
	PrintfUtil_vPrintf("u16TestCardNum = %d  u16PackSendCount  =  %d u16Cardseqnum = %d\n",
				       u16TestCardNum,
		               gTdoaSendPack.u16PackSendCount,
		               gTdoaSendPack.stTdoaMsg[0].u16Cardseqnum);

	//if(gTdoaSendPack.u16PackSendCount > 30)  //ԭ����Ҫ������ֵ����30���ܴ��������¼� ��ǰ��ֻҪ������������д������ڼ����д��ڷ�������
		event_timer_set(EVENT_UART_SEND_TDOA);
	
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
	
	for (i = 0; i < TDOA_PACK_SEND_MAX_NUM; i++)
	{
		memset(&gTdoaSendPack.stTdoaMsg[i], 0, sizeof(TDOA_UWB_TIMESTAMP_PACK_S));
	}
	gTdoaSendPack.u16PackSendCount=0;

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
	
	//���Э������ͷ
	stAppTdoaData.stAppTdoaHead.u8ProtocolType = APP_PROTOCOL_TYPE_UWB_CARD; //Э������ uwb������
	stAppTdoaData.stAppTdoaHead.u8MsgType      = APP_UWB_MSG_TDOA;           //�������� tdoa���������� 

	//��ȡ��ǰ������Ϣ���еĴ��⿨����
	u16TestTagNum = pstTdoaSendPack->u16PackSendCount;
	//PrintfUtil_vPrintf("100007 u16TestTagNum = %d\n", u16TestTagNum);

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
		stAppTdoaData.stAppTdoaHead.u16MsgLen = u16SendDataLen; //����������ĳ���
		
		//���д��ڷ�����������װ ÿ����װ8�Ŵ��⿨�����ݽ��з���
		for (i = u16GroupStartNum, j = 0; 
		    (i < u16GroupStartNum + u16MaxSendNum) && (i < u16TestTagNum); 
			 i++, j++)
		{
			memcpy(&stAppTdoaData.stAppTdoaMsg[j], &pstTdoaSendPack->stTdoaMsg[i], sizeof(APP_UWB_TDOA_TIMESTAMP_S));
		}
		//���д�����Ϣ����ǰ�ĸ�ʽ��װ
		TdoaBsmacBuildPacket((uint8 *)(&stAppTdoaData), u16SendDataLen, BSMAC_FRAME_TYPE_DATA);

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
	//stAppTdoaData.stAppTdoaHead.u16MsgLen =  u16SendDataLen;
	stAppTdoaData.stAppTdoaHead.u16MsgLen =  u16TestTagNum * sizeof(APP_UWB_TDOA_TIMESTAMP_S);

	TdoaBsmacBuildPacket((uint8 *)(&stAppTdoaData), u16SendDataLen, BSMAC_FRAME_TYPE_DATA);
	TdoaDataBuffClear();

	/*************************������Ϣ��ط���*****************************/
	PrintfUtil_vPrintf("**********Data send start***********\n");
	PrintfUtil_vPrintf("beTest_cardID     = %d\n",stAppTdoaData.stAppTdoaMsg[0].u16TestCardID);
	PrintfUtil_vPrintf("standard_cardID   = %d\n",stAppTdoaData.stAppTdoaMsg[0].u16StandardCardID);
	PrintfUtil_vPrintf("stationID         = %d\n",stAppTdoaData.stAppTdoaMsg[0].u16StationID);
	PrintfUtil_vPrintf("Card_seqnum       = %d\n",stAppTdoaData.stAppTdoaMsg[0].u16Cardseqnum);
	PrintfUtil_vPrintf("S_QANH_Tie_H      = %d\n",stAppTdoaData.stAppTdoaMsg[0].u32SQANHTieH);
	PrintfUtil_vPrintf("S_QANH_Tie_L      = %d\n",stAppTdoaData.stAppTdoaMsg[0].u32SQANHTieL);
	PrintfUtil_vPrintf("Q_QANH_Tie_H      = %d\n",stAppTdoaData.stAppTdoaMsg[0].u32QQANHTieH);
	PrintfUtil_vPrintf("Q_QANH_Tie_L      = %d\n",stAppTdoaData.stAppTdoaMsg[0].u32QQANHTieL);
	PrintfUtil_vPrintf("u8DevType         = %d\n",stAppTdoaData.stAppTdoaMsg[0].u8DevType);
	PrintfUtil_vPrintf("u8Status          = %d\n",stAppTdoaData.stAppTdoaMsg[0].u8Status);
	PrintfUtil_vPrintf("card count j      = %d\n",j);
	
	PrintfUtil_vPrintf("**********Data send end***********\n\n");
}
#endif

/* ==========================================================

Notes:

Previously code handled multiple instances in a single console application

Now have changed it to do a single instance only. With minimal code changes...(i.e. kept [instance] index but it is always 0.

Windows application should call instance_init() once and then in the "main loop" call instance_run().

*/
