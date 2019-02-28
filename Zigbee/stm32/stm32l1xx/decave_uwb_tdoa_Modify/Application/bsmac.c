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
��������:UwbWriteDataToStm32
��������:��������stm���ش���оƬ���ڷ���
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-12-21

**************************************************************/
void UwbWriteDataToStm32(uint8* pPackBuf, uint16 u16PackLen)
{
	uint8 i = 0;
	
	//�������ݺ�����У��
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
��������:TdoaBsmacBuildPacketMacHead
��������:���д��ڷ������ݱ���Macͷ������װ
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-12-21

**************************************************************/
void UwbBsmacBuildPacketMacHead(UWB_BSMAC_PACKET_HEADER_S *pstBsmacHead, 
								 uint16 u16DataLen, 
								 uint8 u8FrameType)
{
	uint16 u16StationPanId; //�ݲ���ע����ֵ
	uint16 u16HdlRxFrameCnt;

	//����Ŀ���ַ��Դ��ַ���
	pstBsmacHead->u8SrcAddrH = (u16StationPanId >> 8) & 0xff;
	pstBsmacHead->u8SrcAddrL = (u16StationPanId) & 0xff;			  // source mac address
	pstBsmacHead->u8DstAddrH = 0;									  // dst address is usel
	pstBsmacHead->u8DstAddrL = 0;
	
	//���д��ڱ���mac������Ϣͷ���
	pstBsmacHead->u8PreambleH = BSMAC_PREAMBLE_H; 
	pstBsmacHead->u8PreambleL = BSMAC_PREAMBLE_L; 
	
	BSMAC_SET_DEVICETYPE(pstBsmacHead->u8frameControl, BSMAC_DEVICE_TYPE_LOC);     // I am location module
	BSMAC_SET_RDY(pstBsmacHead->u8frameControl, 1);								   // always ready
	BSMAC_SET_FRAMETYPE(pstBsmacHead->u8frameControl, u8FrameType);
	BSMAC_SET_PRIORITY(pstBsmacHead->u8frameControl, 1);
	
	//���ݲ�ͬ��֡���ͽ���macͷ���ݵķ�װ
	if (u8FrameType == BSMAC_FRAME_TYPE_ACK) // for ack, use recieved frame_cnt
	{
		//��Ӧ֡���ݳ���
		pstBsmacHead->u8FrameCountH = (u16HdlRxFrameCnt & 0xff00) >> 8;
		pstBsmacHead->u8FrameCountL = u16HdlRxFrameCnt  & 0xff;
	}
	else
	{
		//����֡���ݳ���
		pstBsmacHead->u8FrameCountH = ( u16HdlTxFrameCnt & 0xff00) >> 8; // framecnt_h
		pstBsmacHead->u8FrameCountL = u16HdlTxFrameCnt& 0xff; 			// framecnt_l
		u16HdlTxFrameCnt++;
	}
		
	return;	
}
/*************************************************************
��������:UwbBsmacBuildPacketNetHead
��������:���д��ڷ������ݱ�������ͷ������װ
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-12-21

**************************************************************/
void UwbBsmacBuildPacketNetHead(UWB_NET_PACKET_HEADER_S *pstNetHead, 
                                 uint16 u16OwnAddress,
                                 uint16 u16DataLen, 
                                 uint16 u16NetType)
{
	uint16 u16ArmId = 0xFFFF; //�㲥

	//���д��ڱ������粿����Ϣͷ���
	pstNetHead->u8Type = u16NetType; //������������
	pstNetHead->u8Ttl  = 1; 
	pstNetHead->u16Src  = u16OwnAddress; 
	pstNetHead->u16Dst  = u16ArmId; 
	pstNetHead->u16Len  = u16DataLen; 

	return;
}

/*************************************************************
��������:UwbBsmacBuildPacketData
��������:���а����������װ
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-12-21

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
	
	/* �������ݼ�⣬��Ϊack֡�����򲻽��д��� ack do not need payload, Live may have payload */
    if (u8FrameType != BSMAC_FRAME_TYPE_ACK)
    {
        memcpy((void*)pstBsmacPack->u8PackDataBuff, pAppData, u16DataLen);
    }

    //������������У׼ LIVE packet needs to be a long frame
    if (u8FrameType == BSMAC_FRAME_TYPE_LIVE)
    {
        u16DataLen = BSMAC_MAX_TX_PAYLOAD_LEN;
    }
    else if(u8FrameType == BSMAC_FRAME_TYPE_ACK)
    {
        u16DataLen = 0;
    }

	//��¼��ǰ�����͵����ݳ��� У�鳤�Ȳ�����bmacͷ���ȣ�
	//���շ���ǰ��������bamcͷ���ȱ�֤���ݷ�����ȷ
    u16TxLen = u16DataLen + BSMAC_FOOTER_LEN + sizeof(UWB_NET_PACKET_HEADER_S); // length = payload+footer
	pstBsmacPack->stBsmacPackHead.u8DataLenH = (u16TxLen >> 8) & 0xff; //
	pstBsmacPack->stBsmacPackHead.u8DataLenL = u16TxLen & 0xff; //
	
	pbuf = (uint8*)pstBsmacPack; //����crcУ��ǰ�Ƚ��е��ֽ�ǿת�ٽ���ƫ�ƣ�
									 //�������ڶ��������ַ��ƫ�ƴ��󣬶�Ӧ���ݻ����
	//����CRCУ��λ��¼ ���з������ݵ�У��
    u16Crc = CRC16((unsigned char *)(pbuf + 2), 
	  			    u16DataLen + BSMAC_HEADER_LEN + sizeof(UWB_NET_PACKET_HEADER_S) - 2, 
	  			    0xffff);   // caculate header and payload   
	  			    
	//�˴�Ϊ�˱�����ԭ�н���һֱ�������ṹ���Ա
	pbuf[u16DataLen + BSMAC_HEADER_LEN+sizeof(UWB_NET_PACKET_HEADER_S)] = (u16Crc >> 8) & 0xff;
	pbuf[u16DataLen + BSMAC_HEADER_LEN+sizeof(UWB_NET_PACKET_HEADER_S) + 1] = u16Crc & 0xff;
	
	//���з������ݳ��ȸ���
	*pu16DataTxLen = sizeof(bsmac_header_t) + u16TxLen;

	return;
}

/*************************************************************
��������:UwbBsmacSendPacketProc
��������:��TOF����ͨ�����ڷ��͸����ش���stmоƬ
����˵��:
        u16UwbDataLen     Э��ͷ���� + ���ݳ���
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-12-21

**************************************************************/
uint8 UwbBsmacSendPacketProc( uint8 * pAppUwbData,
                            uint16  u16OwnAddress,
                            uint16  u16UwbDataLen,
                            uint8   u8FrameType)
{
	uint16 u16TxLen = 0;
	UWB_BSMAC_BUILD_PACK_S stUwbBsmacPack;
    
    // �������ݳ�����Ч��У��
    if ((u16UwbDataLen == 0) || (u16UwbDataLen > 512) || (pAppUwbData == NULL))
    {
        EDBG(PrintfUtil_vPrintf("Build Failed pbuf %X len%d\n", pAppUwbData, u16UwbDataLen);)
        return 0;
    }
	
	//���нṹ���ݳ�ʼ��
	memset(&stUwbBsmacPack, 0, sizeof(stUwbBsmacPack));
	
	//������Ϣ������ͷ������װ
	UwbBsmacBuildPacketNetHead(&stUwbBsmacPack.stNetPackHead, u16OwnAddress, u16UwbDataLen, NWK_DATA);
	
	//������Ϣ��Macͷ������װ
	UwbBsmacBuildPacketMacHead(&stUwbBsmacPack.stBsmacPackHead, u16UwbDataLen, u8FrameType);

	//���а�������װ
	UwbBsmacBuildPacketData(pAppUwbData, u16UwbDataLen, u8FrameType, &stUwbBsmacPack, &u16TxLen);

	//�������ݰ�����
    UwbWriteDataToStm32((void*)&stUwbBsmacPack, u16TxLen);
	
    return u16TxLen;
}

/*************************************************************
��������:UwbBsmacBlidPacketHeadProc
��������:��װ������Ϣ��ͷ���ṹ
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-12-21

**************************************************************/
void UwbBsmacBlidPacketHeadProc(uint8 u8ProtocolType, uint8 u8MsgType, uint16 u16SendDataLen, 
                                          APP_HEADER_S* pstAppHead)
{    
    //���Э������ͷ
	pstAppHead->u8ProtocolType = u8ProtocolType; //Э������ uwb������
	pstAppHead->u8MsgType      = u8MsgType;           //�������� tof����������  ��Ҫ�����µ��������ͽ��н���
    pstAppHead->u16MsgLen      =  u16SendDataLen;

    return;
}
#endif

