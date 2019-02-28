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
函数名称:TdoaGetLocalInstStructurePtr
函数描述:获取实例全局结构数据
参数说明:void
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
TDOA_INSTANCE_DATA_S* TdoaGetLocalInstStructurePtr(void)
{
	return &gstTdoaInstData;
}

/*************************************************************
函数名称:TdoaGetLocalInstStructurePtr
函数描述:获取卡全局信息数据
参数说明:void
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
TDOA_INST_RXMSG_TO_CARDMSG_S* TdoaGetLocalCardStructurePtr(void)
{
	return &gstTdoaCardMsg;
}

/*************************************************************
函数名称:TdoaGetLocalInstStructurePtr
函数描述:获取Tdoa组包全局信息数据
参数说明:void
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* TdoaGetLocalMsgArrayStructurePtr(void)
{
//	return gstTdoaMsgArray;
}

/*************************************************************
函数名称:TdoaGetLocalInstStructurePtr
函数描述:获取Tdoa发送包的数据
参数说明:void
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
TDOA_UWB_MSG_PACK_SEND_S* TdoaGetLocalSendPackStructurePtr(void)
{
	return &gTdoaSendPack;
}


/*************************************************************
函数名称:TDOATxCallback
函数描述:发送回调函数 暂时不关注，只做预留
		 中断函数: dwt_isr	
参数说明:void
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
void TDOATxCallback(const dwt_callback_data_t *txd)
{

	//发送回调函数暂时不关注 不需要处理
	return;
}

/*************************************************************
函数名称:TDOARxTimeoutCallback
函数描述:接收超时回调函数 暂时不关注，只做预留
         中断函数: dwt_isr
参数说明:void
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
void TDOARxTimeoutCallback(const dwt_callback_data_t *rxd)
{
	return;
}

/*************************************************************
函数名称:TDOARxErrorCallback
函数描述:接收错误回调函数 暂时不关注，只做预留		 
		 中断函数: dwt_isr
参数说明:void
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
void TDOARxErrorCallback(const dwt_callback_data_t *rxd)
{
	return;
}

/*************************************************************
函数名称:TDOARxGoodCallback
函数描述:正确接收回调函数
		 中断函数: dwt_isr
参数说明:void
修改说明:
    作者:何宗峰
修改时间:2018-08-31

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
			pstInstMsg->u16RxMsgLength = pstRxCallBackData->datalength; //标准框架的数据长度为127byte
			pstInstMsg->stInstBaseData.eTdoaInstState = TDOA_RX_WAIT_DATA;
			pstInstMsg->stInstBaseData.eDwEventType = u8RxEvent;
			pstInstMsg->stInstBaseData.u8InstEvent[0] = DWT_SIG_RX_OKAY;

			//获取数据帧类型 在mac帧数据封装函数TdoaSetMacFrameData中u8MessageData[TDOA_INST_FRAME_TYPE_BIT]就为数据帧类型
			//计算方法为 发送总长度为 0-8byte为数据数组存储位置 9byte开始则为6byte长的数据数组，即第9字节为数据帧类型
			dwt_readrxdata((uint8 *)&pstInstMsg->stRxMsgFromDw, pstRxCallBackData->datalength, 0);  // Read Data Frame		

			//记录当前事件类型 DWT_SIG_RX_OKAY
		}
		
		//数据已经记录在实例数据结构中 
		//数据包括: 接收待测卡\快发卡的时间戳、接收器缓冲区获取的数据(数据帧类型、数据来源地址、序列号、数据来源卡速率类型)
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
函数名称:TdoaRxCardMsgProc
函数描述:基站接收到标签消息后进行接收消息的检查和组包处理
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaRxCardMsgProc(void)
{	
	uint8 u8StandCardLoop = 0; //遍历组包消息结构中所有的快发卡数据

	TDOA_INST_RXMSG_TO_CARDMSG_S* pstCardRxMsg = TdoaGetLocalCardStructurePtr();
	//PrintfUtil_vPrintf("100004\n");

	//根据设备的速率分别进行处理
	if (pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_QUICK)	//quick
	{
		//快发卡组包处理
		TdoaRxQuickCardMsgProc(pstCardRxMsg);
	}
	else if (pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_SLOW)
	{
		//慢发 待测卡组包处理
		TdoaRxSlowCardMsgProc(pstCardRxMsg);
	}
	else
	{		
		PrintfUtil_vPrintf("the TdoaRxCardMsgProc Speedtype was wrong u16Speedtype = %d\n", pstCardRxMsg->u16Speedtype);
	}
	//PrintfUtil_vPrintf("100005\n");
	/*********************	打包发送过程   ************************ 
	因为 TdoaCardMsdBuildUnity 里面的n 大小不一，每形成一个发送则每次发送频率太高 
	而且n太小则浪费，若以组合包则又很麻烦。而且n值可能变化较大 、	
	*********************  打包发送过程   ************************/
	
	//注意：若一开始是小包没达到25，后面都是大包自己发送出去，则会造成小包滞留 
	/********************  何时释放msg_arr[k] ***************************/	
	for (u8StandCardLoop = 0; u8StandCardLoop < TDOA_STANDCARD_MAX_NUM; u8StandCardLoop++) //遍历所有的快发卡形成的数据包
	{
		if (gstTdoaMsgArray[u8StandCardLoop].u16MsgSendFlag == TRUE) 		//可以发送才组包 
		{
			TdoaMsgPackInsetUartBuff(&gstTdoaMsgArray[u8StandCardLoop]);
			//PrintfUtil_vPrintf("100006\n");
		}
	}

	return;
}

/*************************************************************
函数名称:Dw1000Init
函数描述:dw1000初始化
参数说明:void
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
int Dw1000Init( void )
{
    int result;
    uint32 u32DevID ;

	reset_DW1000();
	//配置SPI速率 低速进行芯片的读写 高速进行数据的传输
//    port_set_dw1000_slowrate();  //max SPI before PLLs configured is ~4M

	//尝试获取DW1000设备ID，若获取失败则尝试唤醒操作
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

	//启动与DW1000收发器的通信，并读取其DEV_ID寄存器（地址0x00）以验证IC是否受支持
    result = dwt_initialise(DWT_LOADUCODE | DWT_LOADTXCONFIG | DWT_LOADANTDLY| DWT_LOADXTALTRIM) ;
	PrintfUtil_vPrintf("result = %d Dw1000Init 2222\n", result);

    //复位led引脚信号 
    dwt_setleds(2) ; //configure the GPIOs which control the leds on EVBs

    if (DWT_SUCCESS != result)
    {
        return (-1) ;   // device initialise has failed
    }

    //开启中断
	dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | ( DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO /*| DWT_INT_RXPTO*/), 1);

    //enable TX, RX states on GPIOs 6 and 5
//new123    dwt_setlnapamode(1,1);
	
	//初始化dw1000事件计数
    dwt_configeventcounters(TDOA_EABLE); //enable and clear - NOTE: the counters are not preserved when in DEEP SLEEP
	
//	port_set_dw1000_fastrate();

	return 0;
}

/*************************************************************
函数名称:TdoaTagInit
函数描述:

参数说明:void
修改说明:
	作者:何宗峰
修改时间:2018-09-05

**************************************************************/
void TdoaTagInit(void)
{
	int iTdoaInstMode    = 0; //实例角色类型
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

	//设置接收帧的过滤
	dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN); //allow data, ack frames;
	
	//设置设备的panid
	dwt_setpanid(pstInstMsg->stInstBaseData.u16PanId);

	//设置设备的地址
	//set source address into the message structure
	memcpy(&pstInstMsg->stTxMsgToPack.u8SourceAddr, &pstInstMsg->stInstBaseData.u16OwnAddress, ADDR_BYTE_SIZE);
	//dwt_setaddress16(pstInstMsg->stInstBaseData.u16OwnAddress); 

	//设置自动接收
	dwt_setautorxreenable(pstInstMsg->stInstBaseData.u8RxAutoreEnable); //not necessary to auto RX re-enable as the receiver is on for a short time (Tag knows when the response is coming)

	//关闭接收器双通道缓冲区
	#if (DOUBLE_RX_BUFFER == 1)
		dwt_setdblrxbuffmode(0); //disable double RX buffer
    #endif

	//开启自动应答
    #if (ENABLE_AUTO_ACK == 1)
		dwt_enableautoack(ACK_RESPONSE_TIME); //wait for 5 symbols before replying with the ACK
    #endif

	return;
}

/*************************************************************
函数名称:TdoaAnchorInit
函数描述:

参数说明:void
修改说明:
	作者:何宗峰
修改时间:2018-09-05

**************************************************************/
void TdoaAnchorInit(void)
{
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

	pstInstMsg->stInstBaseData.u16PanId = 0xeeee ;//;0xdeca  0xdddd 0xeeee
	//设置设备panid
	dwt_setpanid(pstInstMsg->stInstBaseData.u16PanId);
	
	//设置设备的地址  后面需要用结构体成员替换全局变量u16ShortAddr
	dwt_setaddress16(pstInstMsg->stInstBaseData.u16OwnAddress); 
	
	//set source address into the message structure 后面替换掉
	memcpy(&pstInstMsg->stTxMsgToPack.u8SourceAddr, &pstInstMsg->stInstBaseData.u16OwnAddress, ADDR_BYTE_SIZE);

	//设置发送后立即接收
	dwt_setrxaftertxdelay(WAIT_FOR_RESPONSE_DLY); //set the RX after TX delay time

	//设置自动接收功能
	#if (DECA_BADF_ACCUMULATOR == 0) //can use RX auto re-enable when not logging/plotting errored frames
		pstInstMsg->stInstBaseData.u8RxAutoreEnable = 1;
	#endif
	dwt_setautorxreenable(pstInstMsg->stInstBaseData.u8RxAutoreEnable);

	//禁用接收器双缓冲区
     #if (DOUBLE_RX_BUFFER == 1)
		dwt_setdblrxbuffmode(0); //enable double RX buffer
     #endif

	 //设置接收不超时
	 dwt_setrxtimeout(0);
	 
	 //立即开启接收器接收
	 dwt_rxenable(0) ; //立即开启接收器接收
	
	return;

}

/*************************************************************
函数名称:TdoaInstChannelConfig
函数描述:进行通道模式配置 模式配置只有单次不需要进行通道模式结构体传入
参数说明:uint8 usModeNum 模式类型
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaInstChannelConfig(uint8 usModeNum)
{
	uint32 u32Txpower = 0;
	dwt_config_t stChannelCfg;
    int iUseNvmData = DWT_LOADANTDLY | DWT_LOADXTALTRIM;
	
	//进行通道配置
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

	//通过实例配置进行设备通道配置
	dwt_configure(&stChannelCfg, iUseNvmData);

	//通过实例信息进行发送天线和功率设置
    gstTdoaInstData.stDwInstTxCfg.u8PGdly = gstSpectrumTxConfig[gstChConfig[usModeNum].u8ChannelNumber].u8PGdelay ;
	//获取功率校准值 若获取校准值失败则使用默认值
	u32Txpower = dwt_getotptxpower(gstTdoaInstData.stDwChannelCfg.u8Prf, gstTdoaInstData.stDwChannelCfg.u8ChannelNum);
	if((u32Txpower == 0x0) || (u32Txpower == 0xFFFFFFFF)) //if there are no calibrated values... need to use defaults
    {
        u32Txpower = gstSpectrumTxConfig[gstChConfig[usModeNum].u8ChannelNumber].u32TxPwr[gstTdoaInstData.stDwChannelCfg.u8Prf - DWT_PRF_16M];
    }
	//配置发送功率
	//如果使用智能电源，则直接使用从NVM读取的值
	//如果使用智能电源，用户需要确保每1ms仅传输一帧，否则将违反TX频谱功率
	gstTdoaInstData.stDwInstTxCfg.u32Power = 0x751F1F75;
	dwt_setsmarttxpower(gstTdoaInstData.stDwChannelCfg.u8SmartPowerEn);
	//configure the tx spectrum parameters (power and PG delay)
    dwt_configuretxrf((dwt_txconfig_t *)&gstTdoaInstData.stDwInstTxCfg);

	//检查是否使用从NVM读取的天线延迟校准值
    if((iUseNvmData & DWT_LOADANTDLY) == 0)
    {
        gstTdoaInstData.u16AntennaTxDelay = gau16RfDelays[gstTdoaInstData.stDwChannelCfg.u8Prf - DWT_PRF_16M];
    }
    else
    {
        //获取OTP校准区域读取的天线延迟
        gstTdoaInstData.u16AntennaTxDelay = dwt_readantennadelay(gstTdoaInstData.stDwChannelCfg.u8Prf) >> 1;

        // if nothing was actually programmed then set a reasonable value anyway
		if (gstTdoaInstData.u16AntennaTxDelay == 0)
		{
			gstTdoaInstData.u16AntennaTxDelay = gau16RfDelays[gstTdoaInstData.stDwChannelCfg.u8Prf - DWT_PRF_16M];
		}
    }
	
	// -------------------------------------------------------------------------------------------------------------------
	//设置天线延迟，设置接收和发送的天线延迟相同
	dwt_setrxantennadelay(gstTdoaInstData.u16AntennaTxDelay);
	dwt_settxantennadelay(gstTdoaInstData.u16AntennaTxDelay);

	//此处只需要计算时间差，不关注飞行时间 天线发送接收校准暂不关注
	
	return;
}

/*************************************************************
函数名称:TdoaInstanceBaseMsginit
函数描述:设备实例基本信息初始化
参数说明:void
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
void TdoaInstanceBaseMsginit(int iTdoaInstMode)
{
	int iTxSpeedType = INST_MODE_TX_SPEED_BUTT;

	//进行实例数据全局初始化
	memset(&gstTdoaInstData, 0, sizeof(TDOA_INSTANCE_DATA_S));	
	
	//初始化实例基本信息 
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
		
	//设置实例设备发送速率类型
	if(iTdoaInstMode == TDOA_INST_TAG_TEST)
	{
		iTxSpeedType = INST_MODE_TX_SPEED_SLOW;
	}
	else if (iTdoaInstMode == TDOA_INST_TAG_STANDARD)
	{
		iTxSpeedType = INST_MODE_TX_SPEED_QUICK;
	}
	
	//根据设备类型配置panid 若不是基站则panid设置为0xdeca 若是基站节点则panid为0xeeee
	gstTdoaInstData.stInstBaseData.u16PanId 	  = TDOA_INST_PANID_TAG;
	if (iTdoaInstMode == TDOA_INST_ANCHOR)
	{	
		gstTdoaInstData.stInstBaseData.u16PanId 	  = TDOA_INST_PANID_TAG; //设置和卡同个网段
	}
	
	gstTdoaInstData.stInstBaseData.eTxSpeedType   = iTxSpeedType;

	//进行中断回调函数注册
    dwt_setcallbacks(TDOATxCallback, TDOARxGoodCallback); //zanshizhushi
    
	//进行基站初始化
	if (iTdoaInstMode == TDOA_INST_ANCHOR)
	{
		TdoaAnchorInit();
	}
	else if ((iTdoaInstMode == TDOA_INST_TAG_TEST) || (iTdoaInstMode == TDOA_INST_TAG_STANDARD))
	{
		//进行待测卡初始化
		TdoaTagInit();
	}
	//根据设备类型对应外部LED设备进行不同的闪灯提示 需要根据实际硬件修改
	//dwt_setleds(iTdoaInstMode);

    return;
}

/*************************************************************
函数名称:AppInstanceInit
函数描述:设备实例初始化   
1.暂时不关注天线时延影响 
2.不开启接收器的双缓冲区
3.各种响应延时均不设置

参数说明:void
修改说明:
    作者:何宗峰
修改时间:2018-08-31

**************************************************************/
void AppInstanceInit(int iTdoaInstMode)
{
	(void)Dw1000Init();  //Dw1000模块初始化 暂时放这不影响其他工程编译，后期需要放到board.c中

	//进行实例基础项配置
	TdoaInstanceBaseMsginit(iTdoaInstMode);
		
	//进行实例通道配置 当前默认使用0通道
	TdoaInstChannelConfig(TDOA_CHANNEL_CONFIG_MODE_ONE);

	//去使能设备的两个接收缓冲区 关闭了接收，此逻辑需要修改，接收应该一直存在 有可能导致设备休眠后被唤醒
	//Appsleepinit(); 
	//初始化组包缓冲区
  //   TdoaBuildPackBuffInit();
	
	//设置卡的发送周期,开启消息轮询发送事件
  //  DecaTdoaEquipEventSet();

	//参数校验
	//Tdoa_check_data();

	return;
}

/*************************************************************
函数名称:TdoaSetMacFrameData
函数描述:进行事件消息帧头进行配置
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaInstConfigFrameHeader(TDOA_INSTANCE_DATA_S *pstInstMsg, int iIfAckRequst)
{	 	
	//配置发送帧的panid 三种设备都需要在同一panid网络中工作
	pstInstMsg->stTxMsgToPack.u8PanID[0] = (pstInstMsg->stInstBaseData.u16PanId) & 0xff;  //main station
	pstInstMsg->stTxMsgToPack.u8PanID[1] = (pstInstMsg->stInstBaseData.u16PanId >> 8) & 0xff;	

	//设置帧类型 frame type (0-2), SEC (3), Pending (4), ACK (5), PanIDcomp(6)
	pstInstMsg->stTxMsgToPack.u8FrameCtrl[0] = 0x1 /*frame type 0x1 == data*/ | 0x40 /*PID comp*/;
	pstInstMsg->stTxMsgToPack.u8FrameCtrl[0] |= (iIfAckRequst ? 0x20 : 0x00);

	//进行地址类型标识 此处为短地址
    pstInstMsg->stTxMsgToPack.u8FrameCtrl[1] = 0x8 /*dest short address (16bits)*/ | 0x80 /*src short address (16bits)*/;

	//设置设备的目的地址和源地址
	memcpy(&pstInstMsg->stTxMsgToPack.u8DestAddr, &pstInstMsg->stInstBaseData.u16DestAddress, ADDR_BYTE_SIZE_S);
	memcpy(&pstInstMsg->stTxMsgToPack.u8SourceAddr, &pstInstMsg->stInstBaseData.u16OwnAddress, ADDR_BYTE_SIZE_S);

	return;
}

/*************************************************************
函数名称:TdoaSetMacFrameData
函数描述:进行数据发送前进行mac帧数据和类型配置
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaSetMacFrameData(TDOA_INSTANCE_DATA_S *pstInstMsg, int iFrameDataLen, int iFrameType, int iIfAck)
{

	//记录发送数据帧类型 与ctrl[0]对应
	pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_TYPE_BIT] = iFrameType;
	//记录发送缓冲区长度 可变长数组数据长度 + 帧控制以及目的地址和源地址长度 + 帧校验长度
	pstInstMsg->u16TxMsgLength = iFrameDataLen + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC;

	//设置事件发送消息帧头
    TdoaInstConfigFrameHeader(pstInstMsg, iIfAck); //set up frame header (with/without ack request)

	//设置消息确认等待时间
    if(iIfAck == TDOA_INST_ACK_REQUESTED)
	{
        pstInstMsg->u8WaitAck = DWT_RESPONSE_EXPECTED;
	}
	//记录当前事件消息是否需要等待响应
  //  pstInstMsg->u8AckExpected = iIfAck ; //used to ignore unexpected ACK frames

	return;
}

/*************************************************************
函数名称:TdoaSendTagPoll
函数描述:待测卡和快发卡将轮询消息进行帧封装后通过dw1000发送器发送
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
int TdoaInstSendTagPoolPacket(TDOA_INSTANCE_DATA_S *pstInstMsg, int iTxDelayed)
{
    int result = 0;

	//将待发送数据写入到发送器缓冲区并且在发送器寄存器记录发送长度作为校验处理
    dwt_writetxdata(pstInstMsg->u16TxMsgLength, (uint8 *)&pstInstMsg->stTxMsgToPack, 0) ;   // write the frame data
    dwt_writetxfctrl(pstInstMsg->u16TxMsgLength, 0);//不进行测距
	
	//判断是否需要进行延时发送 发送过程不进行延时发送
    if(iTxDelayed)
    {
        dwt_setdelayedtrxtime(iTxDelayed) ;
    }
	
	//判断消息发送后是否需要进行等待消息确认 poll消息不进行消息确认
    if(pstInstMsg->u8WaitAck)
    {
        //if the ACK is requested there is a 5ms timeout to stop RX if no ACK coming
        dwt_setrxtimeout(5000);  //units are us - wait for 5ms after RX on
    }

    //进行数据发送 poll消息进行立即发送，不需要等待延时
    if (dwt_starttx(iTxDelayed | pstInstMsg->u8WaitAck))  // delayed start was too late
    {    
		PrintfUtil_vPrintf("TdoaInstSendTagPoolPacket dwt_starttx error\n");
        result = 1; //late/error
    }

    return result;                                              // state changes
    // after sending we should return to TX ON STATE ?
}

/*************************************************************
函数名称:TdoaSendTagPoll
函数描述:待测卡和快发卡发送轮询消息给Dw1000发送器
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaSendTagPoll(void)
{
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

	//根据设备速率类型配置发送缓冲区
	if (pstInstMsg->stInstBaseData.eTxSpeedType == INST_MODE_TX_SPEED_SLOW)
	{
		pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SPEED_TYPE_BIT] = TDOA_INST_MODE_SPEED_SLOW;
	}
	else if (pstInstMsg->stInstBaseData.eTxSpeedType == INST_MODE_TX_SPEED_QUICK)
	{
		pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SPEED_TYPE_BIT] = TDOA_INST_MODE_SPEED_QUCIK;
	}

	//配置消息的序列号
	pstInstMsg->stInstBaseData.u16SeqNum ++;
	pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_H] = (pstInstMsg->stInstBaseData.u16SeqNum >> 8) & 0xff;
	pstInstMsg->stTxMsgToPack.u8MessageData[TDOA_INST_FRAME_SEQNUM_BIT_L] = (pstInstMsg->stInstBaseData.u16SeqNum) & 0xff;
	pstInstMsg->stTxMsgToPack.u8SeqNum = (uint8)pstInstMsg->stInstBaseData.u16SeqNum;
		
	//配置实例发送的目的地址 此函数只有待测卡和快发卡会触发 目的地址可以设置为广播全f 
	pstInstMsg->stInstBaseData.u16DestAddress = TDOA_INST_SEND_POLL_DEST_ADDR;
	//pstInstMsg->stTxMsgToPack.u8DestAddr = TDOA_INST_SEND_POLL_DEST_ADDR;

	//向设备mac层设置发送帧内容及类型 与控制寄存器的ctrl[1]对应
	TdoaSetMacFrameData(pstInstMsg, 6, RTLS_DEMO_MSG_TAG_POLL, !TDOA_INST_ACK_REQUESTED);

	//进行消息发送
	if(TdoaInstSendTagPoolPacket(pstInstMsg, DWT_START_TX_IMMEDIATE))
	{
		//若发送失败，则触发轮询发送事件，外部进行周期轮询此处不需要再次设置

		//当前序列号进行减一操作
		pstInstMsg->stInstBaseData.u16SeqNum --;

	}
	
	//发送成功进行闪灯提示
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

	//若发送成功，不需要进行额外操作，轮询流程为周期性流程

	return;
}
/*************************************************************
函数名称:TdoaInstRxMsgToCardMsgProc
函数描述:

参数说明:void
修改说明:
	作者:何宗峰
修改时间:2018-09-05

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
函数名称:TdoaInstRxMsgToCardMsgProc
函数描述:将实例中由接收器缓冲区获取的数据赋值给卡的全局缓冲区，为数据组包做准备

参数说明:void
修改说明:
	作者:何宗峰
修改时间:2018-08-31

**************************************************************/
void TdoaInstRxMsgToCardMsgProc(void)
{
	uint16	u16SrcAddr;
	uint16	u16Seqnum;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

	//获取设备的能量值
	TdoaInstRxRssiLevel();
	// 获取接收信息的源地址
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
函数名称:TdoaInstRxMsgToCardMsgProcNew
函数描述:将实例中由接收器缓冲区获取的数据赋值给卡的全局缓冲区，为数据组包做准备

参数说明:void
修改说明:
	作者:何宗峰
修改时间:2018-08-31

**************************************************************/
void TdoaInstRxMsgToCardMsgProcNew(void)
{
	uint16	u16SrcAddr;
	uint16	u16Seqnum;
	//TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	instance_data_t* pstInstMsg = TdoaGetOldLocalInstStructurePtr();

	//获取设备的能量值
	rx_power_lever(pstInstMsg);
	
	// 获取接收信息的源地址
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


	//进行十秒的TDOA的测距,超过十次则不在发送TDOA poll包进行测距 u8TdoaRxMsgCount

/*
	PrintfUtil_vPrintf("u16CardId = %d u16OwnAddress = %d u16Seqnum = %d\n",
		gstTdoaCardMsg.u16CardId, 
		pstInstMsg->stInstBaseData.u16OwnAddress,
		u16Seqnum);
*/
	return;
}

/*************************************************************
函数名称:AppInstanceInit
函数描述:设备实例初始化   
1.暂时不关注天线时延影响 
2.不开启接收器的双缓冲区
3.各种响应延时均不设置

参数说明:void
修改说明:
	作者:何宗峰
修改时间:2018-08-31

**************************************************************/
void TdoaInstRunState(void)
{
	int iTdoaInstMode    = 0; //实例角色类型
	int iTdoaInstState   = 0; //实例当前运行状态
	int iDwEventType 	 = 0; //实例当前事件类型
	int iRxMsgFrameType  = 0; //事例接收帧类型
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();

	iTdoaInstMode   = pstInstMsg->stInstBaseData.eTdoaInstMode;
	iTdoaInstState  = pstInstMsg->stInstBaseData.eTdoaInstState;
	iDwEventType    = pstInstMsg->stInstBaseData.eDwEventType;
	iRxMsgFrameType = pstInstMsg->stRxMsgFromDw.u8MessageData[0];

	//PrintfUtil_vPrintf("iTdoaInstMode = %d eTdoaInstState = %d iDwEventType = %d\n",
	//	iTdoaInstMode, iTdoaInstState, iDwEventType);
	//PrintfUtil_vPrintf("100002 iTdoaInstState = %d iDwEventType = %d iRxMsgFrameType = %d\n", iTdoaInstState, iDwEventType, iRxMsgFrameType);

	//判断实例当前运行状态
    switch (iTdoaInstState)
    {
    	case TDOA_INIT: //初始化状态进行根据设备类型分别进行配置
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

	                default: //其他情况不进行处理
	                break;
			}
			break; 
			
		case TDOA_RX_WAIT_DATA:
			switch(iDwEventType) //实例事件类型
			{
				case DWT_SIG_RX_OKAY:
					switch(iRxMsgFrameType) //实例接收帧类型
					{
						case RTLS_DEMO_MSG_TAG_POLL:
							if (iTdoaInstMode == TDOA_INST_ANCHOR)
							{
								//PrintfUtil_vPrintf("100003\n");
								//将实例中由接收器缓冲区获取的数据赋值给卡的全局缓冲区
								TdoaInstRxMsgToCardMsgProc();
								//修改实例状态
								pstInstMsg->stInstBaseData.eTdoaInstState = TDOA_RX_WAIT_DATA;
								//清除数据缓冲区避免没有接收时使用上一次数据进行处理
								memset(&pstInstMsg->stRxMsgFromDw, 0, sizeof(TDOA_DW_TRX_MSG_S));
							//	pstInstMsg->stInstBaseData.eDwEventType = DWT_SIG_RX_NOERR; //改变接收状态，等待下一次接收
								//设置设备触发组包事件
								event_timer_set(EVENT_CHECKTDOA_REVMSG_EVENT); 
							}
							break;

		                default: //其他情况不进行处理
							//开启设备接收 开启不延时就开始接收 开启自动接收
							// PrintfUtil_vPrintf("other RxMsgFrameType\n");
							dwt_rxenable(0); 
			                break;
							
					}
					break;
					 
                default: //其他情况不进行处理
					//开启设备接收 开启不延时就开始接收 开启自动接收
					PrintfUtil_vPrintf("other DwEventType\n");
					dwt_rxenable(0); 
	                break;
			}
			break;

			default: //其他情况不进行处理
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
函数名称:TdoaWriteDataToStm32
函数描述:将数据往stm主控处理芯片串口发送
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaWriteDataToStm32(uint8* pTdoaPackBuf, uint16 u16TdoaPackLen)
{
	uint8 i = 0;
	
	//进行数据合理性校验
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
函数名称:TdoaBsmacBuildPacketMacHead
函数描述:进行串口发送数据报文Mac头部分组装
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaBsmacBuildPacketMacHead(TDOA_BSMAC_PACKET_HEADER_S *pstTdoaBsmacHead, 
								 uint16 u16TdoaDataLen, 
								 uint8 u8FrameType)
{
	uint16 u16StationPanId; //暂不关注具体值
	uint16 u16HdlRxFrameCnt;

	//进行目标地址和源地址填充
	pstTdoaBsmacHead->u8SrcAddrH = (u16StationPanId >> 8) & 0xff;
	pstTdoaBsmacHead->u8SrcAddrL = (u16StationPanId) & 0xff;			  // source mac address
	pstTdoaBsmacHead->u8DstAddrH = 0;									  // dst address is usel
	pstTdoaBsmacHead->u8DstAddrL = 0;
	
	//进行串口报文mac部分信息头填充
	pstTdoaBsmacHead->u8PreambleH = BSMAC_PREAMBLE_H; 
	pstTdoaBsmacHead->u8PreambleL = BSMAC_PREAMBLE_L; 
	
	BSMAC_SET_DEVICETYPE(pstTdoaBsmacHead->u8frameControl, BSMAC_DEVICE_TYPE_LOC);     // I am location module
	BSMAC_SET_RDY(pstTdoaBsmacHead->u8frameControl, 1);								   // always ready
	BSMAC_SET_FRAMETYPE(pstTdoaBsmacHead->u8frameControl, u8FrameType);
	BSMAC_SET_PRIORITY(pstTdoaBsmacHead->u8frameControl, 1);
	
	//根据不同的帧类型进行mac头数据的封装
	if (u8FrameType == BSMAC_FRAME_TYPE_ACK) // for ack, use recieved frame_cnt
	{
		//响应帧数据长度
		pstTdoaBsmacHead->u8FrameCountH = (u16HdlRxFrameCnt & 0xff00) >> 8;
		pstTdoaBsmacHead->u8FrameCountL = u16HdlRxFrameCnt  & 0xff;
	}
	else
	{
		//发送帧数据长度
		pstTdoaBsmacHead->u8FrameCountH = ( u16HdlTxFrameCnt & 0xff00) >> 8; // framecnt_h
		pstTdoaBsmacHead->u8FrameCountL = u16HdlTxFrameCnt& 0xff; 			// framecnt_l
		u16HdlTxFrameCnt++;
	}
		
	return;	
}
/*************************************************************
函数名称:TdoaBsmacBuildPacketNetHead
函数描述:进行串口发送数据报文网络头部分组装
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaBsmacBuildPacketNetHead(TDOA_NET_PACKET_HEADER_S *pstTdoaNetHead, 
                                 uint16 u16TdoaDataLen, 
                                 uint16 u16NetType)
{
	uint16 u16ArmId = 0xFFFF; //广播

	//进行串口报文网络部分信息头填充
	pstTdoaNetHead->u8Type = u16NetType; //网络数据类型
	pstTdoaNetHead->u8Ttl  = 1; 
	pstTdoaNetHead->u16Src  = gstTdoaInstData.stInstBaseData.u16OwnAddress; 
	pstTdoaNetHead->u16Dst  = u16ArmId; 
	pstTdoaNetHead->u16Len  = u16TdoaDataLen; 

	return;
}

/*************************************************************
函数名称:TdoaBsmacBuildPacketData
函数描述:进行包数据填充组装
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

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
	
	/* 进行数据检测，若为ack帧类型则不进行处理 ack do not need payload, Live may have payload */
    if (u8FrameType != BSMAC_FRAME_TYPE_ACK)
    {
        memcpy((void*)pstTdoaBsmacPack->u8PackDataBuff, pAppTdoaData, u16TdoaDataLen);
    }

    //进行数据重新校准 LIVE packet needs to be a long frame
    if (u8FrameType == BSMAC_FRAME_TYPE_LIVE)
    {
        u16TdoaDataLen = BSMAC_MAX_TX_PAYLOAD_LEN;
    }
    else if(u8FrameType == BSMAC_FRAME_TYPE_ACK)
    {
        u16TdoaDataLen = 0;
    }

	//记录当前包发送的数据长度 校验长度不包括bmac头长度，
	//最终发送前会进行添加bamc头长度保证数据发送正确
    u16TxLen = u16TdoaDataLen + BSMAC_FOOTER_LEN + sizeof(TDOA_NET_PACKET_HEADER_S); // length = payload+footer
	pstTdoaBsmacPack->stBsmacPackHead.u8DataLenH = (u16TxLen >> 8) & 0xff; //
	pstTdoaBsmacPack->stBsmacPackHead.u8DataLenL = u16TxLen & 0xff; //
	
	pbuf = (uint8*)pstTdoaBsmacPack; //进行crc校验前先进行单字节强转再进行偏移，
									 //否则由于对齐问题地址会偏移错误，对应数据会出错
	//进行CRC校验位记录 进行发送数据的校验
    u16Crc = CRC16((unsigned char *)(pbuf + 2), 
	  			    u16TdoaDataLen + BSMAC_HEADER_LEN + sizeof(TDOA_NET_PACKET_HEADER_S) - 2, 
	  			    0xffff);   // caculate header and payload   
	  			    
	//此处为了保持与原有解析一直不新增结构体成员
	pbuf[u16TdoaDataLen + BSMAC_HEADER_LEN+sizeof(TDOA_NET_PACKET_HEADER_S)] = (u16Crc >> 8) & 0xff;
	pbuf[u16TdoaDataLen + BSMAC_HEADER_LEN+sizeof(TDOA_NET_PACKET_HEADER_S) + 1] = u16Crc & 0xff;
	
	//进行发送数据长度更正
	*pu16TdoaDataTxLen = sizeof(bsmac_header_t) + u16TxLen;

	return;
}

/*************************************************************
函数名称:TdoaBsmacBuildPacket
函数描述:将TDOA数据通过串口发送给主控处理stm芯片
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
uint8 TdoaBsmacBuildPacket( uint8 * pAppTdoaData,
                            uint16  u16TdoaDataLen,
                            uint8   u8FrameType)
{
	uint16 u16TxLen = 0;
	TDOA_BSMAC_BUILD_PACK_S stTdoaBsmacPack;

    // 进行数据长度有效性校验
    if ((u16TdoaDataLen == 0) || (u16TdoaDataLen > 512) || (pAppTdoaData == NULL))
    {
        EDBG(PrintfUtil_vPrintf("Build Failed pbuf %X len%d\n", pAppTdoaData, u16TdoaDataLen);)
        return 0;
    }
	
	//进行结构数据初始化
	memset(&stTdoaBsmacPack, 0, sizeof(stTdoaBsmacPack));
	
	//进行消息包网络头部分组装
	TdoaBsmacBuildPacketNetHead(&stTdoaBsmacPack.stNetPackHead, u16TdoaDataLen, NWK_DATA);
	
	//进行消息包Mac头部分组装
	TdoaBsmacBuildPacketMacHead(&stTdoaBsmacPack.stBsmacPackHead, u16TdoaDataLen, u8FrameType);

	//进行包数据组装
	TdoaBsmacBuildPacketData(pAppTdoaData, u16TdoaDataLen, u8FrameType, &stTdoaBsmacPack, &u16TxLen);

	//进行数据包发送
    TdoaWriteDataToStm32((void*)&stTdoaBsmacPack, u16TxLen);
	
    return u16TxLen;
}

/***********************************************************************
作用:TdoaMsgPackInsetUartBuff 将数据发送出去，若传入的数据够大，直接发送出去，若比较小
	则放入到pack中待包一定大时发送出去
参数:
	pack :待发送的包空间
	msg_array :待放入到pack的数据
	time :放入pack包的次数
返回: 空
***********************************************************************/
void TdoaMsgPackInsetUartBuff(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray)
{
	int iLoop = 0;
	uint16 u16TestCardNum = 0;
	u16TestCardNum = pstTdoaMsgArray->u16TdoaMsgNum; //获取该快发卡组包数据中包含的待测卡数量

	//判断该数据是否是否可以发送
	if (pstTdoaMsgArray->u16MsgSendFlag != TRUE)
	{
		return;
	}
	
	//进行发送数据结构初始化
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

	//if(gTdoaSendPack.u16PackSendCount > 30)  //原来需要缓冲区值大于30才能触发发送事件 当前是只要存在数据则进行触发串口即进行串口发送数据
		event_timer_set(EVENT_UART_SEND_TDOA);
	
	TdoaClearMsgArray(pstTdoaMsgArray);            //清空改基准标签单元的原有已发送的数据

	return;
}

/*************************************************************
函数名称:TdoaDataBuffClear
函数描述:进行发送到串口的数据包初始化
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

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
函数名称:TdoaSendCardReportToUart
函数描述:将卡的组包数据由小stm32往大stm32发送
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaSendCardReportToUart(void)
{
	uint16 i = 0;
	uint16 j = 0;
	uint16 u16SendGroupNum  = 0; //发送组号
	uint16 u16MaxSendNum    = 0; //串口一次性最大发送的数据对应待测卡数量
	uint16 u16TestTagNum    = 0; //消息包中的待测卡数量
	uint16 u16GroupStartNum = 0; //当前串口发送数据组的起始地址
	uint16 u16SendDataLen   = 0; //串口发送的数据长度 卡的数据长度 + 协议头长度
	APP_UWB_TDOA_DITANCE_S stAppTdoaData;

	TDOA_UWB_MSG_PACK_SEND_S* pstTdoaSendPack =  TdoaGetLocalSendPackStructurePtr();
	
	//填充协议数据头
	stAppTdoaData.stAppTdoaHead.u8ProtocolType = APP_PROTOCOL_TYPE_UWB_CARD; //协议类型 uwb卡类型
	stAppTdoaData.stAppTdoaHead.u8MsgType      = APP_UWB_MSG_TDOA;           //数据类型 tdoa的数据类型 

	//获取当前发送消息包中的待测卡数量
	u16TestTagNum = pstTdoaSendPack->u16PackSendCount;
	//PrintfUtil_vPrintf("100007 u16TestTagNum = %d\n", u16TestTagNum);

	//若没有待测卡数据则不往下进行串口发送
	if(u16TestTagNum <= 0)
	{
		return;
	}
	/* 由于串口为单字节发送，若一次性发送数据过多会导致串口发送奔溃，所以此处进行数据长度检查，
	   若消息包中存在的待测卡信息超过8，则需要进行分批此发送消息才能保证串口发送的安全，
	   每次最多发送8张待测卡的信息
	*/
	u16MaxSendNum = TDOA_UART_SEND_MAX_TAG_NUM;
	u16SendDataLen = u16MaxSendNum * sizeof(APP_UWB_TDOA_TIMESTAMP_S) + sizeof(APP_HEADER_S);
	while(u16TestTagNum > u16MaxSendNum)
	{
		u16GroupStartNum = u16SendGroupNum * u16MaxSendNum;
		u16SendGroupNum ++; //数据组号增加
		stAppTdoaData.stAppTdoaHead.u16MsgLen = u16SendDataLen; //单个数据组的长度
		
		//进行串口发送数据组组装 每次组装8张待测卡的数据进行发送
		for (i = u16GroupStartNum, j = 0; 
		    (i < u16GroupStartNum + u16MaxSendNum) && (i < u16TestTagNum); 
			 i++, j++)
		{
			memcpy(&stAppTdoaData.stAppTdoaMsg[j], &pstTdoaSendPack->stTdoaMsg[i], sizeof(APP_UWB_TDOA_TIMESTAMP_S));
		}
		//进行串口消息发送前的格式组装
		TdoaBsmacBuildPacket((uint8 *)(&stAppTdoaData), u16SendDataLen, BSMAC_FRAME_TYPE_DATA);

		//更新数据未发送的待测卡数量 
		u16TestTagNum -= u16MaxSendNum;
	}

	//检查是否仍有未发送的待测卡数据 若已经发送完则直接进行返回
	if(u16TestTagNum == 0)
	{
		return;
	}
	
	//若进行分组发送后仍有待测卡数据未发送 则需要进行正确偏移获取未发送的待测卡数据
	u16GroupStartNum = u16SendGroupNum * u16MaxSendNum;  //若没有进行分组发送此公式的偏移也正确
	//将剩余的待测卡数据进行组装
	for (i = u16GroupStartNum, j = 0; 
	     i < u16TestTagNum; 
		 i++, j++)
	{
		memcpy(&stAppTdoaData.stAppTdoaMsg[j],&pstTdoaSendPack->stTdoaMsg[u16GroupStartNum+i],sizeof(APP_UWB_TDOA_TIMESTAMP_S));
	}
		 
	//串口发送的数据长度	 
	u16SendDataLen =  u16TestTagNum * sizeof(APP_UWB_TDOA_TIMESTAMP_S) + sizeof(APP_HEADER_S);
	//stAppTdoaData.stAppTdoaHead.u16MsgLen =  u16SendDataLen;
	stAppTdoaData.stAppTdoaHead.u16MsgLen =  u16TestTagNum * sizeof(APP_UWB_TDOA_TIMESTAMP_S);

	TdoaBsmacBuildPacket((uint8 *)(&stAppTdoaData), u16SendDataLen, BSMAC_FRAME_TYPE_DATA);
	TdoaDataBuffClear();

	/*************************发送信息监控分析*****************************/
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
