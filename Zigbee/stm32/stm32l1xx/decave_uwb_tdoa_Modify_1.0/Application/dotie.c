#include"dotie.h"
#include "printf_util.h"
#include  "instance.h"


extern TDOA_UWB_TIMESTAMP_MSG_ARRAY_S gstTdoaMsgArray[TDOA_STANDCARD_MAX_NUM];


AVG_TIE avgTie[standard_num_max];

uint64 max_tie;
unsigned int standardID[standard_num_max]={0} ;
extern uint8 gu8NblinkSend;
//extern TDOA_INSTANCE_DATA_S gstTdoaInstData;
/********************************************************************************/
/***********************************************************************
����:avg_ANH_Tie ��ƽ����TIE
����:
	rx_msg :һ����յ��Ļ�׼��ǩ��Ϣ
	msg_array:�Ի�׼��ǩΪ��ĵ�Ԫ
����: 
	NULL
***********************************************************************/	

uint64 gettie(AVG_TIE avgtie,uint64 tie)
{
	uint64 temp;
	if(tie <= avgtie.Max_tie && tie >= avgtie.Min_tie)
		temp = tie;
	else if(tie > avgtie.Max_tie)
	{
		temp = avgtie.Max_tie;
		avgtie.Max_tie = tie;
	}
	else if(tie < avgtie.Min_tie)
	{
		temp = avgtie.Min_tie;
		avgtie.Min_tie = tie;
	}
	return temp;
}

void avg_ANH_Tie(Rx_msg rx_msg, TDOA_UWB_TIMESTAMP_MSG_ARRAY_S *pstTdoaMsgArray, int n)
{
	uint64 Tie,temp;
	uint64 time1=0,result=0;
	if(rx_msg.speedtype!=1)
		return ;

	if(rx_msg.seqnum - pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum == 1)
	{
		time1 = pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH;
		time1 = (time1<<32) + + pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL;
		result = (rx_msg.own_timestamp - time1 +Tick_max)%Tick_max;
		Tie = result;//(rx_msg.own_timestamp - pstTdoaMsgArray->stPreQuickCardMsg.own_timestamp + Tick_max) % Tick_max;
		if(avgTie[n].count ==0){
			avgTie[n].Max_tie = Tie;
			avgTie[n].avg_TIE =0;
		}
		else if(avgTie[n].count ==1)
		{
			if(Tie >avgTie[n].Max_tie)
			{
				avgTie[n].Min_tie = avgTie[n].Max_tie;
				avgTie[n].Max_tie = Tie;
			}
			else
				avgTie[n].Min_tie = Tie;
			avgTie[n].avg_TIE =0;
		}
		else if(avgTie[n].count >=2 && avgTie[n].count<102)
		{
			temp = gettie(avgTie[n], Tie);
			avgTie[n].avg_TIE = (uint64)(((avgTie[n].count-2) *avgTie[n].avg_TIE +temp)/(avgTie[n].count-1)) ;
		}
		else if(avgTie[n].count==102)
		{
			if(Tie<avgTie[n].Max_tie && Tie>avgTie[n].Min_tie)
			{
				avgTie[n].avg_TIE =(uint64)(((avgTie[n].count-2) *avgTie[n].avg_TIE +Tie)/(avgTie[n].count-1)) ;
				avgTie[n].count--;
			}
		}
		avgTie[n].count++;		//����10��ʱ�����ʹ��
	}	
}

/*************************************************************
��������:TdoaCardMsdBuildUnity
��������:���п���Ϣ���������  �����ܵĵ�ÿһ�����ݣ��ŵ����Ӧ���Ի�׼��ǩ����ĵ�Ԫ��
����˵��:	
		rx_msg :һ����յ��Ļ�׼��ǩ��Ϣ
		msg_array:�Ի�׼��ǩΪ��ĵ�Ԫ
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaClearMsgArray(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray)
{
	int i = 0;
	
	//����˿췢�������еĴ�����Ϣ��
	for(i = 0; i < TDOA_MSG_TESTCARD_MAX_NUM; i++)
	{
		memset(&pstTdoaMsgArray->stTdoaMsg[i], 0, sizeof(TDOA_UWB_TIMESTAMP_PACK_S));
	}
	memset(&pstTdoaMsgArray->stLastQuickCardMsg, 0, sizeof(TDOA_STANDARD_CARD_MSG_S));
	pstTdoaMsgArray->u16TdoaMsgNum  = 0;
	pstTdoaMsgArray->u16MsgSendFlag = 0;

	return;
}


/*************************************************************
��������:DecaTdoaBuildPackInit
��������:������Ļ��������г�ʼ��

����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-08-31

**************************************************************/
void TdoaBuildPackBuffInit(void)
{
	uint32 i = 0; 
	
	for (i = 0; i < TDOA_STANDCARD_MAX_NUM; i++)
	{
		memset(&gstTdoaMsgArray[i], 0, sizeof(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S));
	}
	
	return;
}

/*************************************************************
��������:TdoaCardMsdBuildUnity
��������:���п���Ϣ���������
����˵��:
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaCardMsdBuildUnityNew(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg, 
						   TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray, 
						   int iMsgArrayNum)
{
	int i;
	uint16 u16TdoaMsgNum    = 0;   //����췢��֮���ѱ���ǵĴ��⿨����
	uint64 u64TimeStampTemp = 0;   //ʱ���ֵ�м仺����
	uint64 Q_QANH_Tie;
	uint64 result=0;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	
	u16TdoaMsgNum = pstTdoaMsgArray->u16TdoaMsgNum;
    /* ���ݵ��� 
       ���տ췢ʱ��洢�� u32SQANHTieH
       ���մ���ʱ��洢�� u32QQANHTieH */


	//���µ���ϢΪ�췢����Ϣ����������������л�û�д��⿨��Ϣ���ݱ���¼
	if (u16TdoaMsgNum == 0 && pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_QUICK)  //�췢
	{
	
		u64TimeStampTemp = pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH;
		u64TimeStampTemp = (u64TimeStampTemp << 32) + pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL;
		if (max_tie==0&&pstCardRxMsg->u16Seqnum - pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum ==1)
		{
			max_tie = (pstCardRxMsg->u64RxCardTimestamp -u64TimeStampTemp +Tick_max)%Tick_max;
		}
		pstTdoaMsgArray->u16StandCardLost ++; 
		pstTdoaMsgArray->stPreQuickCardMsg.u16CardId = pstCardRxMsg->u16CardId;
		pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH =(uint32) (pstCardRxMsg->u64RxCardTimestamp>>32)& 0xFFFFFFFF;
		pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL =(uint32) pstCardRxMsg->u64RxCardTimestamp & 0xFFFFFFFF;
		pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum = pstCardRxMsg->u16Seqnum;	
        
	}
	else if ((pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH != 0 || 
  			 pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL != 0) && 
  			 pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_SLOW && 
  			 u16TdoaMsgNum < TDOA_MSG_TESTCARD_MAX_NUM)		//���٣������ǩ���� 
	{
		//�ж���Ч����:�ڴ�֮ǰ���ڿ췢����¼ + ��������Ϊ�������� + �˿췢��δ��¼������ʮ�Ŵ��⿨
		/*********************************************************************
			��һ����pre_slow ʱ������̫����˵���췢�м�©��������ȡ��
			һ�����Ϊ2������ƽ��ʱ���������һ����ʱ���
		*********************************************************************/
		u64TimeStampTemp = pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH;
		u64TimeStampTemp = (u64TimeStampTemp<<32) +pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL;
		//result = (pstCardRxMsg->u64RxCardTimestamp - u64TimeStampTemp +Tick_max)%Tick_max;
		pstTdoaMsgArray->u16TdoaMsgNum = pstTdoaMsgArray->u16TdoaMsgNum + 1;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u16TestCardID     = pstCardRxMsg->u16CardId;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u16StandardCardID = pstTdoaMsgArray->stPreQuickCardMsg.u16CardId;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u16StationID 		= pstInstMsg->stInstBaseData.u16OwnAddress;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u32SQANHTieH 		= (uint32)(pstCardRxMsg->u64RxCardTimestamp >>32) &0xFFFFFFFF;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u32SQANHTieL		=(uint32) pstCardRxMsg->u64RxCardTimestamp &0xFFFFFFFF ;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u16Cardseqnum 	= pstCardRxMsg->u16Seqnum;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u8DevType		 	= pstCardRxMsg->u8DevType;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u8Status			= pstCardRxMsg->u16Status;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].i8Rssi			= pstCardRxMsg->i8Rssi;
        
        pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u32QQANHTieH 		= pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u32QQANHTieL		= pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL;

        pstTdoaMsgArray->u16MsgSendFlag = TRUE;			   //���Է��ͳ�ȥ 
	}
    #if 0
	else if(u16TdoaMsgNum >= 1 && pstCardRxMsg->u16Speedtype == 1) //���ڴ��⿨��Ϣ��췢��Ϣ�ٴε����������
	{
		pstTdoaMsgArray->stLastQuickCardMsg.u16Seqnum = pstCardRxMsg->u16Seqnum;
		pstTdoaMsgArray->stLastQuickCardMsg.u16CardId = pstCardRxMsg->u16CardId;
		pstTdoaMsgArray->stLastQuickCardMsg.u32RxCardTimestampH =(uint32) (pstCardRxMsg->u64RxCardTimestamp >>32) &0xFFFFFFFF;
		pstTdoaMsgArray->stLastQuickCardMsg.u32RxCardTimestampL =(uint32) (pstCardRxMsg->u64RxCardTimestamp) &0xFFFFFFFF;
		pstTdoaMsgArray->u16StandCardLost++;
		//ǰ�������췢��Ϣ�����кŲ�ֵ����Ϊ1�ſ��Խ������
		if(pstCardRxMsg->u16Seqnum - pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum == 1)  //�췢����ǩ�����к����1ʱ�������
		{
			u64TimeStampTemp = pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH;
			u64TimeStampTemp = (u64TimeStampTemp<<32) + pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL;
			result = (pstCardRxMsg->u64RxCardTimestamp - u64TimeStampTemp + Tick_max)%Tick_max;
			Q_QANH_Tie = result;//(pstCardRxMsg->u64RxCardTimestamp - pstTdoaMsgArray->stPreQuickCardMsg.u64RxCardTimestamp+Tick_max)%Tick_max;
			for (i = 0; i < u16TdoaMsgNum; i++)
			{
				if(avgTie[iMsgArrayNum].count <20)
				{
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieH =(uint32)(Q_QANH_Tie >>32) &0xFFFFFFFF;
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieL =(uint32)Q_QANH_Tie &0xFFFFFFFF;
				}
				else
				{
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieH = (avgTie[iMsgArrayNum].avg_TIE >>32)&0xFFFFFFFF;
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieL = avgTie[iMsgArrayNum].avg_TIE &0xFFFFFFFF;
					u64TimeStampTemp = pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieH ;
					u64TimeStampTemp = (u64TimeStampTemp<<32) + pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieL;
					result = (uint64)((double)(avgTie[iMsgArrayNum].avg_TIE)/(double)(Q_QANH_Tie) * (double)(u64TimeStampTemp)) ;
					pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieH =(uint32) (result >>32) &0xFFFFFFFF;//(uint64)((double)(avgTie[k].avg_TIE)/(double)(Q_QANH_Tie) * (double)(pstTdoaMsgArray->stTdoaMsg[i].S_QANH_Tie)) ;
					pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieL = (uint32)(result &0xFFFFFFFF);
				}
                //��¼�췢�����к����Ϊ1���
                pstTdoaMsgArray->stTdoaMsg[i].u8Status = 1;
			}
			pstTdoaMsgArray->u16MsgSendFlag = TRUE;			   //���Է��ͳ�ȥ 
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg, &pstTdoaMsgArray->stLastQuickCardMsg, sizeof(TDOA_STANDARD_CARD_MSG_S));
		}
        else if (pstCardRxMsg->u16Seqnum - pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum == 2) //�췢����ǩ�����к����2ʱ�������
        {
			u64TimeStampTemp = pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH;
			u64TimeStampTemp = (u64TimeStampTemp<<32) + pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL;
			result = (pstCardRxMsg->u64RxCardTimestamp - u64TimeStampTemp + Tick_max)%Tick_max;
			Q_QANH_Tie = result;//(pstCardRxMsg->u64RxCardTimestamp - pstTdoaMsgArray->stPreQuickCardMsg.u64RxCardTimestamp+Tick_max)%Tick_max;
			for (i = 0; i < u16TdoaMsgNum; i++)
			{
				if(avgTie[iMsgArrayNum].count <20)
				{
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieH =(uint32)(Q_QANH_Tie >>32) &0xFFFFFFFF;
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieL =(uint32)Q_QANH_Tie &0xFFFFFFFF;
				}
				else
				{
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieH = (avgTie[iMsgArrayNum].avg_TIE >>32)&0xFFFFFFFF;
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieL = avgTie[iMsgArrayNum].avg_TIE &0xFFFFFFFF;
					u64TimeStampTemp = pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieH ;
					u64TimeStampTemp = (u64TimeStampTemp<<32) + pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieL;
					result = (uint64)((double)(avgTie[iMsgArrayNum].avg_TIE)/(double)(Q_QANH_Tie) * (double)(u64TimeStampTemp)) ;
					pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieH =(uint32) (result >>32) &0xFFFFFFFF;//(uint64)((double)(avgTie[k].avg_TIE)/(double)(Q_QANH_Tie) * (double)(pstTdoaMsgArray->stTdoaMsg[i].S_QANH_Tie)) ;
					pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieL = (uint32)(result &0xFFFFFFFF);
				}
                
                //��¼�췢�����к����Ϊ2���
                pstTdoaMsgArray->stTdoaMsg[i].u8Status = 2;
			}
			pstTdoaMsgArray->u16MsgSendFlag = TRUE;			   //���Է��ͳ�ȥ 
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg, &pstTdoaMsgArray->stLastQuickCardMsg, sizeof(TDOA_STANDARD_CARD_MSG_S));

        }
		else
		{ 
			/************************** ��ش������� start *******************************/		
			PrintfUtil_vPrintf(" not %d,%d,%d,%d\n",
			pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum,
			pstTdoaMsgArray->stLastQuickCardMsg.u16Seqnum,
			pstTdoaMsgArray->u16TdoaMsgNum,
			pstTdoaMsgArray->stTdoaMsg[pstTdoaMsgArray->u16TdoaMsgNum - 1].u16Cardseqnum
			);
			/************************** ��ش������� end *******************************/
			//�췢��ǰ�����кŲ�ֵ��Ϊһ�������ǰ�췢���Լ�¼�Ĵ��⿨��Ϣ
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg,&pstTdoaMsgArray->stLastQuickCardMsg,sizeof(TDOA_STANDARD_CARD_MSG_S));
			TdoaClearMsgArray(pstTdoaMsgArray);
		}
	}
    #endif
    //���tdoa���Ѿ������˴������Բ���Ҫ������ʱ����
    gu8NblinkSend = 1;
    
	return;
}

/*************************************************************
��������:TdoaCardMsdBuildUnity
��������:���п���Ϣ���������
����˵��:
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaCardMsdBuildUnity(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg, 
						   TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray, 
						   int iMsgArrayNum)
{
	int i;
	uint16 u16TdoaMsgNum    = 0;   //����췢��֮���ѱ���ǵĴ��⿨����
	uint64 u64TimeStampTemp = 0;   //ʱ���ֵ�м仺����
	uint64 Q_QANH_Tie;
	uint64 result=0;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	
	u16TdoaMsgNum = pstTdoaMsgArray->u16TdoaMsgNum;
	//PrintfUtil_vPrintf("%d %d %d \n", pstCardRxMsg->u16CardId, pstCardRxMsg->u16Seqnum, pstCardRxMsg->u16Speedtype);

	//���µ���ϢΪ�췢����Ϣ����������������л�û�д��⿨��Ϣ���ݱ���¼
	if (u16TdoaMsgNum == 0 && pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_QUICK)  //�췢
	{
		u64TimeStampTemp = pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH;
		u64TimeStampTemp = (u64TimeStampTemp << 32) + pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL;
		if (max_tie==0&&pstCardRxMsg->u16Seqnum - pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum ==1)
		{
			max_tie = (pstCardRxMsg->u64RxCardTimestamp -u64TimeStampTemp +Tick_max)%Tick_max;
		}
		pstTdoaMsgArray->u16StandCardLost ++; 
		pstTdoaMsgArray->stPreQuickCardMsg.u16CardId = pstCardRxMsg->u16CardId;
		pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH =(uint32) (pstCardRxMsg->u64RxCardTimestamp>>32)& 0xFFFFFFFF;
		pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL =(uint32) pstCardRxMsg->u64RxCardTimestamp & 0xFFFFFFFF;
		pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum = pstCardRxMsg->u16Seqnum;		
	}
	else if ((pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH != 0 || 
  			 pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL != 0) && 
  			 pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_SLOW && 
  			 u16TdoaMsgNum < TDOA_MSG_TESTCARD_MAX_NUM)		//���٣������ǩ���� 
	{
		//�ж���Ч����:�ڴ�֮ǰ���ڿ췢����¼ + ��������Ϊ�������� + �˿췢��δ��¼������ʮ�Ŵ��⿨
		/*********************************************************************
			��һ����pre_slow ʱ������̫����˵���췢�м�©��������ȡ��
			һ�����Ϊ2������ƽ��ʱ���������һ����ʱ���
		*********************************************************************/
		u64TimeStampTemp = pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH;
		u64TimeStampTemp = (u64TimeStampTemp<<32) +pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL;
		result = (pstCardRxMsg->u64RxCardTimestamp - u64TimeStampTemp +Tick_max)%Tick_max;
		pstTdoaMsgArray->u16TdoaMsgNum = pstTdoaMsgArray->u16TdoaMsgNum + 1;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u16TestCardID     = pstCardRxMsg->u16CardId;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u16StandardCardID = pstTdoaMsgArray->stPreQuickCardMsg.u16CardId;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u16StationID 		= pstInstMsg->stInstBaseData.u16OwnAddress;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u32SQANHTieH 		= (uint32)(result >>32) &0xFFFFFFFF;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u32SQANHTieL		=(uint32) result &0xFFFFFFFF ;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u16Cardseqnum 	= pstCardRxMsg->u16Seqnum;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u8DevType		 	= pstCardRxMsg->u8DevType;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].u8Status			= pstCardRxMsg->u16Status;
		pstTdoaMsgArray->stTdoaMsg[u16TdoaMsgNum].i8Rssi			= pstCardRxMsg->i8Rssi;
	}
	else if(u16TdoaMsgNum >= 1 && pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_QUICK) //���ڴ��⿨��Ϣ��췢��Ϣ�ٴε����������
	{
		pstTdoaMsgArray->stLastQuickCardMsg.u16Seqnum = pstCardRxMsg->u16Seqnum;
		pstTdoaMsgArray->stLastQuickCardMsg.u16CardId = pstCardRxMsg->u16CardId;
		pstTdoaMsgArray->stLastQuickCardMsg.u32RxCardTimestampH =(uint32) (pstCardRxMsg->u64RxCardTimestamp >>32) &0xFFFFFFFF;
		pstTdoaMsgArray->stLastQuickCardMsg.u32RxCardTimestampL =(uint32) (pstCardRxMsg->u64RxCardTimestamp) &0xFFFFFFFF;
		pstTdoaMsgArray->u16StandCardLost++;
		//ǰ�������췢��Ϣ�����кŲ�ֵ����Ϊ1�ſ��Խ������
		if(pstCardRxMsg->u16Seqnum - pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum == 1)  //�췢����ǩ�����к����1ʱ�������
		{
			u64TimeStampTemp = pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH;
			u64TimeStampTemp = (u64TimeStampTemp<<32) + pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL;
			result = (pstCardRxMsg->u64RxCardTimestamp - u64TimeStampTemp + Tick_max)%Tick_max;
			Q_QANH_Tie = result;//(pstCardRxMsg->u64RxCardTimestamp - pstTdoaMsgArray->stPreQuickCardMsg.u64RxCardTimestamp+Tick_max)%Tick_max;
			for (i = 0; i < u16TdoaMsgNum; i++)
			{
				if(avgTie[iMsgArrayNum].count <20)
				{
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieH =(uint32)(Q_QANH_Tie >>32) &0xFFFFFFFF;
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieL =(uint32)Q_QANH_Tie &0xFFFFFFFF;
				}
				else
				{
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieH = (avgTie[iMsgArrayNum].avg_TIE >>32)&0xFFFFFFFF;
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieL = avgTie[iMsgArrayNum].avg_TIE &0xFFFFFFFF;
					u64TimeStampTemp = pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieH ;
					u64TimeStampTemp = (u64TimeStampTemp<<32) + pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieL;
					result = (uint64)((double)(avgTie[iMsgArrayNum].avg_TIE)/(double)(Q_QANH_Tie) * (double)(u64TimeStampTemp)) ;
					pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieH =(uint32) (result >>32) &0xFFFFFFFF;//(uint64)((double)(avgTie[k].avg_TIE)/(double)(Q_QANH_Tie) * (double)(pstTdoaMsgArray->stTdoaMsg[i].S_QANH_Tie)) ;
					pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieL = (uint32)(result &0xFFFFFFFF);
				}
                //��¼�췢�����к����Ϊ1���
                pstTdoaMsgArray->stTdoaMsg[i].u8Status = 1;
			}
			pstTdoaMsgArray->u16MsgSendFlag = TRUE;			   //���Է��ͳ�ȥ 
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg, &pstTdoaMsgArray->stLastQuickCardMsg, sizeof(TDOA_STANDARD_CARD_MSG_S));
		}
        else if (pstCardRxMsg->u16Seqnum - pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum == 2) //�췢����ǩ�����к����2ʱ�������
        {
			u64TimeStampTemp = pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampH;
			u64TimeStampTemp = (u64TimeStampTemp<<32) + pstTdoaMsgArray->stPreQuickCardMsg.u32RxCardTimestampL;
			result = (pstCardRxMsg->u64RxCardTimestamp - u64TimeStampTemp + Tick_max)%Tick_max;
			Q_QANH_Tie = result;//(pstCardRxMsg->u64RxCardTimestamp - pstTdoaMsgArray->stPreQuickCardMsg.u64RxCardTimestamp+Tick_max)%Tick_max;
			for (i = 0; i < u16TdoaMsgNum; i++)
			{
				if(avgTie[iMsgArrayNum].count <20)
				{
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieH =(uint32)(Q_QANH_Tie >>32) &0xFFFFFFFF;
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieL =(uint32)Q_QANH_Tie &0xFFFFFFFF;
				}
				else
				{
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieH = (avgTie[iMsgArrayNum].avg_TIE >>32)&0xFFFFFFFF;
					pstTdoaMsgArray->stTdoaMsg[i].u32QQANHTieL = avgTie[iMsgArrayNum].avg_TIE &0xFFFFFFFF;
					u64TimeStampTemp = pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieH ;
					u64TimeStampTemp = (u64TimeStampTemp<<32) + pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieL;
					result = (uint64)((double)(avgTie[iMsgArrayNum].avg_TIE)/(double)(Q_QANH_Tie) * (double)(u64TimeStampTemp)) ;
					pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieH =(uint32) (result >>32) &0xFFFFFFFF;//(uint64)((double)(avgTie[k].avg_TIE)/(double)(Q_QANH_Tie) * (double)(pstTdoaMsgArray->stTdoaMsg[i].S_QANH_Tie)) ;
					pstTdoaMsgArray->stTdoaMsg[i].u32SQANHTieL = (uint32)(result &0xFFFFFFFF);
				}
                
                //��¼�췢�����к����Ϊ2���
                pstTdoaMsgArray->stTdoaMsg[i].u8Status = 2;
			}
			pstTdoaMsgArray->u16MsgSendFlag = TRUE;			   //���Է��ͳ�ȥ 
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg, &pstTdoaMsgArray->stLastQuickCardMsg, sizeof(TDOA_STANDARD_CARD_MSG_S));

        }
		else
		{ 
			/************************** ��ش������� start *******************************/		
			PrintfUtil_vPrintf(" not %d,%d,%d,%d\n",
			pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum,
			pstTdoaMsgArray->stLastQuickCardMsg.u16Seqnum,
			pstTdoaMsgArray->u16TdoaMsgNum,
			pstTdoaMsgArray->stTdoaMsg[pstTdoaMsgArray->u16TdoaMsgNum - 1].u16Cardseqnum
			);
			/************************** ��ش������� end *******************************/
			//�췢��ǰ�����кŲ�ֵ��Ϊһ�������ǰ�췢���Լ�¼�Ĵ��⿨��Ϣ
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg,&pstTdoaMsgArray->stLastQuickCardMsg,sizeof(TDOA_STANDARD_CARD_MSG_S));
			TdoaClearMsgArray(pstTdoaMsgArray);
		}
	}
    //���tdoa���Ѿ������˴������Բ���Ҫ������ʱ����
    gu8NblinkSend = 1;
    
	return;
}

/*************************************************************
��������:TdoaCheckRxMsgStandardID
��������:���ĳһ���췢(��׼��ǩ)�Ƿ��м�¼
����˵��:����1��˵��Ϊ�¿���û�м�¼
�޸�˵��:
	����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
int TdoaCheckRxMsgStandardID(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg)    
{
	int iStandCardNum = 0;
	uint8 u8IfNewCardFlag = TRUE;

	for (iStandCardNum = 0; iStandCardNum < TDOA_STANDCARD_MAX_NUM; iStandCardNum++)
	{
		if(gstTdoaMsgArray[iStandCardNum].stPreQuickCardMsg.u16CardId == pstCardRxMsg->u16CardId)
		{
			u8IfNewCardFlag = FALSE;
			break;
		}
	}

	return u8IfNewCardFlag;
}
/*************************************************************
��������:TdoaRxSlowCardMsgProc
��������:��վ���յ����⿨��Ϣ���н�����Ϣ�ļ����������
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaRxSlowCardMsgProc(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg)
{
	uint8 u8TdoaStandCardLoop = 0; //���������Ϣ�ṹ�����еĿ췢������

	if(pstCardRxMsg->u64RxCardTimestamp != 0)  //���մ��⿨����Ϣ��ʱ�����Ч
	{
		for(u8TdoaStandCardLoop = 0; u8TdoaStandCardLoop < TDOA_STANDCARD_MAX_NUM; u8TdoaStandCardLoop++) //׼�����������еĿ췢�������������
		{
			if(gstTdoaMsgArray[u8TdoaStandCardLoop].stPreQuickCardMsg.u16CardId != 0) //�����ڿ췢����������ݽṹ��
			{
				//�������ݵ����
				TdoaCardMsdBuildUnity(pstCardRxMsg, &gstTdoaMsgArray[u8TdoaStandCardLoop], u8TdoaStandCardLoop); //���뵽�췢�ռ� 
				//TdoaCardMsdBuildUnityNew(pstCardRxMsg, &gstTdoaMsgArray[u8TdoaStandCardLoop], u8TdoaStandCardLoop); //���뵽�췢�ռ� 
            }
		}
	}

	return;
}

/*************************************************************
��������:TdoaRxQuickCardMsgProc
��������:��վ���յ��췢��ǩ��Ϣ���н�����Ϣ�ļ����������
����˵��:
�޸�˵��:
    ����:���ڷ�
�޸�ʱ��:2018-09-03

**************************************************************/
void TdoaRxQuickCardMsgProc(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg)
{
	int iStandNum = 0;
	int iIdleArrayNum = -1;
	uint8 u8TdoaStandCardLoop = 0; //���������Ϣ�ṹ�����еĿ췢������
	uint8 u8IfNewCardFlag = FALSE;
	//TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray[] = TdoaGetLocalMsgArrayStructurePtr();

	u8IfNewCardFlag = TdoaCheckRxMsgStandardID(pstCardRxMsg); 
	if(u8IfNewCardFlag == TRUE)//�����췢 δ����ǵĿ췢��
	{	  
		//��ȡ���е�����������ṹ
		for(iStandNum = 0; iStandNum < TDOA_STANDCARD_MAX_NUM; iStandNum++)
		{
			if(gstTdoaMsgArray[iStandNum].stPreQuickCardMsg.u16CardId == 0)
			{
				//�����ݽṹ�п췢��idδ���������Ϊ�п��е�����������ṹ
				iIdleArrayNum = iStandNum;
				break;
			}
		}

		//��û�п��е�������������򲻽����µĿ췢����¼
		if (iIdleArrayNum == -1)
		{
			return;
		}
		//����ָ��������л��������г�ʼ��
		memset(&gstTdoaMsgArray[iIdleArrayNum], 0, sizeof(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S));
		//�������ݵ����
		TdoaCardMsdBuildUnity(pstCardRxMsg, &gstTdoaMsgArray[iIdleArrayNum], iIdleArrayNum); 
		//TdoaCardMsdBuildUnityNew(pstCardRxMsg, &gstTdoaMsgArray[iIdleArrayNum], iIdleArrayNum); 

		gstTdoaMsgArray[iIdleArrayNum].u16StandCardStartSeq = pstCardRxMsg->u16Seqnum;
	}
	else if(u8IfNewCardFlag == FALSE)   //���м�¼
	{
		for(u8TdoaStandCardLoop = 0; u8TdoaStandCardLoop < TDOA_STANDCARD_MAX_NUM; u8TdoaStandCardLoop++)
		{
			if(gstTdoaMsgArray[u8TdoaStandCardLoop].stPreQuickCardMsg.u16CardId == pstCardRxMsg->u16CardId)
			{
				iIdleArrayNum = u8TdoaStandCardLoop;
				break;
			}
		}

		//���������е����������δ�ҵ���Ӧ�Ŀ췢��id������з���
		if (iIdleArrayNum == -1)
		{
			return;
		}
		
		//�������ݵ����
		TdoaCardMsdBuildUnity(pstCardRxMsg, &gstTdoaMsgArray[iIdleArrayNum], iIdleArrayNum); 
		//TdoaCardMsdBuildUnityNew(pstCardRxMsg, &gstTdoaMsgArray[iIdleArrayNum], iIdleArrayNum); 

	}
	else
	{
		PrintfUtil_vPrintf("the TdoaCheckRxMsgStandardID array was full ,get a wrong menber \n");
	}

	return;
}




