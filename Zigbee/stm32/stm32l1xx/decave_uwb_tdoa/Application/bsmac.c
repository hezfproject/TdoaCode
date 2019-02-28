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

#if 1
uint16 u16HdlTxFrameCnt = 0;
/*************************************************************
函数名称:UwbWriteDataToStm32
函数描述:将数据往stm主控处理芯片串口发送
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-12-21

**************************************************************/
void UwbWriteDataToStm32(uint8* pPackBuf, uint16 u16PackLen)
{
	uint8 i = 0;
	
	//进行数据合理性校验
	if(pPackBuf == NULL || u16PackLen == 0)
	{
		return;
	}
	
	//PrintfUtil_vPrintf(" write len = %d\n\n", u16PackLen);
	for (i = 0; i < u16PackLen; i++)
	{
		USART_SendData(USART1,(unsigned char)( *(pPackBuf + i))); /* Loop until the end of transmission */

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
修改时间:2018-12-21

**************************************************************/
void UwbBsmacBuildPacketMacHead(UWB_BSMAC_PACKET_HEADER_S *pstBsmacHead, 
								 uint16 u16DataLen, 
								 uint8 u8FrameType)
{
	uint16 u16StationPanId; //暂不关注具体值
	uint16 u16HdlRxFrameCnt;

	//进行目标地址和源地址填充
	pstBsmacHead->u8SrcAddrH = (u16StationPanId >> 8) & 0xff;
	pstBsmacHead->u8SrcAddrL = (u16StationPanId) & 0xff;			  // source mac address
	pstBsmacHead->u8DstAddrH = 0;									  // dst address is usel
	pstBsmacHead->u8DstAddrL = 0;
	
	//进行串口报文mac部分信息头填充
	pstBsmacHead->u8PreambleH = BSMAC_PREAMBLE_H; 
	pstBsmacHead->u8PreambleL = BSMAC_PREAMBLE_L; 
	
	BSMAC_SET_DEVICETYPE(pstBsmacHead->u8frameControl, BSMAC_DEVICE_TYPE_LOC);     // I am location module
	BSMAC_SET_RDY(pstBsmacHead->u8frameControl, 1);								   // always ready
	BSMAC_SET_FRAMETYPE(pstBsmacHead->u8frameControl, u8FrameType);
	BSMAC_SET_PRIORITY(pstBsmacHead->u8frameControl, 1);
	
	//根据不同的帧类型进行mac头数据的封装
	if (u8FrameType == BSMAC_FRAME_TYPE_ACK) // for ack, use recieved frame_cnt
	{
		//响应帧数据长度
		pstBsmacHead->u8FrameCountH = (u16HdlRxFrameCnt & 0xff00) >> 8;
		pstBsmacHead->u8FrameCountL = u16HdlRxFrameCnt  & 0xff;
	}
	else
	{
		//发送帧数据长度
		pstBsmacHead->u8FrameCountH = ( u16HdlTxFrameCnt & 0xff00) >> 8; // framecnt_h
		pstBsmacHead->u8FrameCountL = u16HdlTxFrameCnt& 0xff; 			// framecnt_l
		u16HdlTxFrameCnt++;
	}
		
	return;	
}
/*************************************************************
函数名称:UwbBsmacBuildPacketNetHead
函数描述:进行串口发送数据报文网络头部分组装
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-12-21

**************************************************************/
void UwbBsmacBuildPacketNetHead(UWB_NET_PACKET_HEADER_S *pstNetHead, 
                                 uint16 u16OwnAddress,
                                 uint16 u16DataLen, 
                                 uint16 u16NetType)
{
	uint16 u16ArmId = 0xFFFF; //广播

	//进行串口报文网络部分信息头填充
	pstNetHead->u8Type = u16NetType; //网络数据类型
	pstNetHead->u8Ttl  = 1; 
	pstNetHead->u16Src  = u16OwnAddress; 
	pstNetHead->u16Dst  = u16ArmId; 
	pstNetHead->u16Len  = u16DataLen; 

	return;
}

/*************************************************************
函数名称:UwbBsmacBuildPacketData
函数描述:进行包数据填充组装
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-12-21

**************************************************************/
void UwbBsmacBuildPacketData(uint8* pAppData, 
							  uint16 u16DataLen, 
							  uint8  u8FrameType,
							  UWB_BSMAC_BUILD_PACK_S *pstBsmacPack,
							  uint16 *pu16DataTxLen)
{
	uint16 u16TxLen = 0;
	uint16 u16Crc   = 0;
	uint8*  pbuf;
	
	/* 进行数据检测，若为ack帧类型则不进行处理 ack do not need payload, Live may have payload */
    if (u8FrameType != BSMAC_FRAME_TYPE_ACK)
    {
        memcpy((void*)pstBsmacPack->u8PackDataBuff, pAppData, u16DataLen);
    }

    //进行数据重新校准 LIVE packet needs to be a long frame
    if (u8FrameType == BSMAC_FRAME_TYPE_LIVE)
    {
        u16DataLen = BSMAC_MAX_TX_PAYLOAD_LEN;
    }
    else if(u8FrameType == BSMAC_FRAME_TYPE_ACK)
    {
        u16DataLen = 0;
    }

	//记录当前包发送的数据长度 校验长度不包括bmac头长度，
	//最终发送前会进行添加bamc头长度保证数据发送正确
    u16TxLen = u16DataLen + BSMAC_FOOTER_LEN + sizeof(UWB_NET_PACKET_HEADER_S); // length = payload+footer
	pstBsmacPack->stBsmacPackHead.u8DataLenH = (u16TxLen >> 8) & 0xff; //
	pstBsmacPack->stBsmacPackHead.u8DataLenL = u16TxLen & 0xff; //
	
	pbuf = (uint8*)pstBsmacPack; //进行crc校验前先进行单字节强转再进行偏移，
									 //否则由于对齐问题地址会偏移错误，对应数据会出错
	//进行CRC校验位记录 进行发送数据的校验
    u16Crc = CRC16((unsigned char *)(pbuf + 2), 
	  			    u16DataLen + BSMAC_HEADER_LEN + sizeof(UWB_NET_PACKET_HEADER_S) - 2, 
	  			    0xffff);   // caculate header and payload   
	  			    
	//此处为了保持与原有解析一直不新增结构体成员
	pbuf[u16DataLen + BSMAC_HEADER_LEN+sizeof(UWB_NET_PACKET_HEADER_S)] = (u16Crc >> 8) & 0xff;
	pbuf[u16DataLen + BSMAC_HEADER_LEN+sizeof(UWB_NET_PACKET_HEADER_S) + 1] = u16Crc & 0xff;
	
	//进行发送数据长度更正
	*pu16DataTxLen = sizeof(bsmac_header_t) + u16TxLen;

	return;
}

/*************************************************************
函数名称:UwbBsmacSendPacketProc
函数描述:将TOF数据通过串口发送给主控处理stm芯片
参数说明:
        u16UwbDataLen     协议头长度 + 数据长度
修改说明:
    作者:何宗峰
修改时间:2018-12-21

**************************************************************/
uint8 UwbBsmacSendPacketProc( uint8 * pAppUwbData,
                            uint16  u16OwnAddress,
                            uint16  u16UwbDataLen,
                            uint8   u8FrameType)
{
	uint16 u16TxLen = 0;
	UWB_BSMAC_BUILD_PACK_S stUwbBsmacPack;
    
    // 进行数据长度有效性校验
    if ((u16UwbDataLen == 0) || (u16UwbDataLen > 512) || (pAppUwbData == NULL))
    {
        EDBG(PrintfUtil_vPrintf("Build Failed pbuf %X len%d\n", pAppUwbData, u16UwbDataLen);)
        return 0;
    }
	
	//进行结构数据初始化
	memset(&stUwbBsmacPack, 0, sizeof(stUwbBsmacPack));
	
	//进行消息包网络头部分组装
	UwbBsmacBuildPacketNetHead(&stUwbBsmacPack.stNetPackHead, u16OwnAddress, u16UwbDataLen, NWK_DATA);
	
	//进行消息包Mac头部分组装
	UwbBsmacBuildPacketMacHead(&stUwbBsmacPack.stBsmacPackHead, u16UwbDataLen, u8FrameType);

	//进行包数据组装
	UwbBsmacBuildPacketData(pAppUwbData, u16UwbDataLen, u8FrameType, &stUwbBsmacPack, &u16TxLen);

	//进行数据包发送
    UwbWriteDataToStm32((void*)&stUwbBsmacPack, u16TxLen);
	
    return u16TxLen;
}

/*************************************************************
函数名称:UwbBsmacBlidPacketHeadProc
函数描述:组装串口消息包头部结构
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-12-21

**************************************************************/
void UwbBsmacBlidPacketHeadProc(uint8 u8ProtocolType, uint8 u8MsgType, uint16 u16SendDataLen, 
                                          APP_HEADER_S* pstAppHead)
{    
    //填充协议数据头
	pstAppHead->u8ProtocolType = u8ProtocolType; //协议类型 uwb卡类型
	pstAppHead->u8MsgType      = u8MsgType;           //数据类型 tof的数据类型  需要定义新的数据类型进行解析
    pstAppHead->u16MsgLen      =  u16SendDataLen;

    return;
}
#endif

