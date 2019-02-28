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
作用:avg_ANH_Tie 求平均的TIE
参数:
	rx_msg :一组接收到的基准标签信息
	msg_array:以基准标签为组的单元
返回: 
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
		avgTie[n].count++;		//大于10的时候才能使用
	}	
}

/*************************************************************
函数名称:TdoaCardMsdBuildUnity
函数描述:进行卡信息的组包处理  将接受的的每一组数据，放到相对应的以基准标签分类的单元中
参数说明:	
		rx_msg :一组接收到的基准标签信息
		msg_array:以基准标签为组的单元
修改说明:
	作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaClearMsgArray(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray)
{
	int i = 0;
	
	//清除此快发卡中所有的待测消息包
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
函数名称:DecaTdoaBuildPackInit
函数描述:将组包的缓冲区进行初始化

参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-08-31

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
函数名称:TdoaCardMsdBuildUnity
函数描述:进行卡信息的组包处理
参数说明:
修改说明:
	作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaCardMsdBuildUnityNew(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg, 
						   TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray, 
						   int iMsgArrayNum)
{
	int i;
	uint16 u16TdoaMsgNum    = 0;   //定义快发卡之间已被标记的待测卡数量
	uint64 u64TimeStampTemp = 0;   //时间戳值中间缓冲区
	uint64 Q_QANH_Tie;
	uint64 result=0;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	
	u16TdoaMsgNum = pstTdoaMsgArray->u16TdoaMsgNum;
    /* 数据到来 
       接收快发时间存储到 u32SQANHTieH
       接收待测时间存储到 u32QQANHTieH */


	//若新的消息为快发卡消息，并且组包缓冲区中还没有待测卡消息数据被记录
	if (u16TdoaMsgNum == 0 && pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_QUICK)  //快发
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
  			 u16TdoaMsgNum < TDOA_MSG_TESTCARD_MAX_NUM)		//慢速，待测标签来到 
	{
		//判断有效条件:在此之前存在快发卡记录 + 速率类型为慢发类型 + 此快发卡未记录超过二十张待测卡
		/*********************************************************************
			第一个与pre_slow 时间戳相差太大，则说明快发中间漏掉，不可取，
			一般相差为2，可用平均时间戳估计上一个的时间戳
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

        pstTdoaMsgArray->u16MsgSendFlag = TRUE;			   //可以发送出去 
	}
    #if 0
	else if(u16TdoaMsgNum >= 1 && pstCardRxMsg->u16Speedtype == 1) //存在待测卡信息后快发消息再次到来进行组包
	{
		pstTdoaMsgArray->stLastQuickCardMsg.u16Seqnum = pstCardRxMsg->u16Seqnum;
		pstTdoaMsgArray->stLastQuickCardMsg.u16CardId = pstCardRxMsg->u16CardId;
		pstTdoaMsgArray->stLastQuickCardMsg.u32RxCardTimestampH =(uint32) (pstCardRxMsg->u64RxCardTimestamp >>32) &0xFFFFFFFF;
		pstTdoaMsgArray->stLastQuickCardMsg.u32RxCardTimestampL =(uint32) (pstCardRxMsg->u64RxCardTimestamp) &0xFFFFFFFF;
		pstTdoaMsgArray->u16StandCardLost++;
		//前后两个快发消息的序列号差值必须为1才可以进行组包
		if(pstCardRxMsg->u16Seqnum - pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum == 1)  //快发卡标签的序列号相差1时进行组包
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
                //记录快发卡序列号相差为1组包
                pstTdoaMsgArray->stTdoaMsg[i].u8Status = 1;
			}
			pstTdoaMsgArray->u16MsgSendFlag = TRUE;			   //可以发送出去 
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg, &pstTdoaMsgArray->stLastQuickCardMsg, sizeof(TDOA_STANDARD_CARD_MSG_S));
		}
        else if (pstCardRxMsg->u16Seqnum - pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum == 2) //快发卡标签的序列号相差2时进行组包
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
                
                //记录快发卡序列号相差为2组包
                pstTdoaMsgArray->stTdoaMsg[i].u8Status = 2;
			}
			pstTdoaMsgArray->u16MsgSendFlag = TRUE;			   //可以发送出去 
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg, &pstTdoaMsgArray->stLastQuickCardMsg, sizeof(TDOA_STANDARD_CARD_MSG_S));

        }
		else
		{ 
			/************************** 监控错误数据 start *******************************/		
			PrintfUtil_vPrintf(" not %d,%d,%d,%d\n",
			pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum,
			pstTdoaMsgArray->stLastQuickCardMsg.u16Seqnum,
			pstTdoaMsgArray->u16TdoaMsgNum,
			pstTdoaMsgArray->stTdoaMsg[pstTdoaMsgArray->u16TdoaMsgNum - 1].u16Cardseqnum
			);
			/************************** 监控错误数据 end *******************************/
			//快发卡前后序列号差值不为一，清除当前快发卡以记录的待测卡信息
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg,&pstTdoaMsgArray->stLastQuickCardMsg,sizeof(TDOA_STANDARD_CARD_MSG_S));
			TdoaClearMsgArray(pstTdoaMsgArray);
		}
	}
    #endif
    //标记tdoa包已经进行了处理，可以不必要进行延时接收
    gu8NblinkSend = 1;
    
	return;
}

/*************************************************************
函数名称:TdoaCardMsdBuildUnity
函数描述:进行卡信息的组包处理
参数说明:
修改说明:
	作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaCardMsdBuildUnity(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg, 
						   TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray, 
						   int iMsgArrayNum)
{
	int i;
	uint16 u16TdoaMsgNum    = 0;   //定义快发卡之间已被标记的待测卡数量
	uint64 u64TimeStampTemp = 0;   //时间戳值中间缓冲区
	uint64 Q_QANH_Tie;
	uint64 result=0;
	TDOA_INSTANCE_DATA_S* pstInstMsg = TdoaGetLocalInstStructurePtr();
	
	u16TdoaMsgNum = pstTdoaMsgArray->u16TdoaMsgNum;
	//PrintfUtil_vPrintf("%d %d %d \n", pstCardRxMsg->u16CardId, pstCardRxMsg->u16Seqnum, pstCardRxMsg->u16Speedtype);

	//若新的消息为快发卡消息，并且组包缓冲区中还没有待测卡消息数据被记录
	if (u16TdoaMsgNum == 0 && pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_QUICK)  //快发
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
  			 u16TdoaMsgNum < TDOA_MSG_TESTCARD_MAX_NUM)		//慢速，待测标签来到 
	{
		//判断有效条件:在此之前存在快发卡记录 + 速率类型为慢发类型 + 此快发卡未记录超过二十张待测卡
		/*********************************************************************
			第一个与pre_slow 时间戳相差太大，则说明快发中间漏掉，不可取，
			一般相差为2，可用平均时间戳估计上一个的时间戳
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
	else if(u16TdoaMsgNum >= 1 && pstCardRxMsg->u16Speedtype == INST_MODE_TX_SPEED_QUICK) //存在待测卡信息后快发消息再次到来进行组包
	{
		pstTdoaMsgArray->stLastQuickCardMsg.u16Seqnum = pstCardRxMsg->u16Seqnum;
		pstTdoaMsgArray->stLastQuickCardMsg.u16CardId = pstCardRxMsg->u16CardId;
		pstTdoaMsgArray->stLastQuickCardMsg.u32RxCardTimestampH =(uint32) (pstCardRxMsg->u64RxCardTimestamp >>32) &0xFFFFFFFF;
		pstTdoaMsgArray->stLastQuickCardMsg.u32RxCardTimestampL =(uint32) (pstCardRxMsg->u64RxCardTimestamp) &0xFFFFFFFF;
		pstTdoaMsgArray->u16StandCardLost++;
		//前后两个快发消息的序列号差值必须为1才可以进行组包
		if(pstCardRxMsg->u16Seqnum - pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum == 1)  //快发卡标签的序列号相差1时进行组包
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
                //记录快发卡序列号相差为1组包
                pstTdoaMsgArray->stTdoaMsg[i].u8Status = 1;
			}
			pstTdoaMsgArray->u16MsgSendFlag = TRUE;			   //可以发送出去 
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg, &pstTdoaMsgArray->stLastQuickCardMsg, sizeof(TDOA_STANDARD_CARD_MSG_S));
		}
        else if (pstCardRxMsg->u16Seqnum - pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum == 2) //快发卡标签的序列号相差2时进行组包
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
                
                //记录快发卡序列号相差为2组包
                pstTdoaMsgArray->stTdoaMsg[i].u8Status = 2;
			}
			pstTdoaMsgArray->u16MsgSendFlag = TRUE;			   //可以发送出去 
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg, &pstTdoaMsgArray->stLastQuickCardMsg, sizeof(TDOA_STANDARD_CARD_MSG_S));

        }
		else
		{ 
			/************************** 监控错误数据 start *******************************/		
			PrintfUtil_vPrintf(" not %d,%d,%d,%d\n",
			pstTdoaMsgArray->stPreQuickCardMsg.u16Seqnum,
			pstTdoaMsgArray->stLastQuickCardMsg.u16Seqnum,
			pstTdoaMsgArray->u16TdoaMsgNum,
			pstTdoaMsgArray->stTdoaMsg[pstTdoaMsgArray->u16TdoaMsgNum - 1].u16Cardseqnum
			);
			/************************** 监控错误数据 end *******************************/
			//快发卡前后序列号差值不为一，清除当前快发卡以记录的待测卡信息
			memcpy(&pstTdoaMsgArray->stPreQuickCardMsg,&pstTdoaMsgArray->stLastQuickCardMsg,sizeof(TDOA_STANDARD_CARD_MSG_S));
			TdoaClearMsgArray(pstTdoaMsgArray);
		}
	}
    //标记tdoa包已经进行了处理，可以不必要进行延时接收
    gu8NblinkSend = 1;
    
	return;
}

/*************************************************************
函数名称:TdoaCheckRxMsgStandardID
函数描述:检测某一个快发(基准标签)是否有记录
参数说明:返回1则说明为新卡，没有记录
修改说明:
	作者:何宗峰
修改时间:2018-09-03

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
函数名称:TdoaRxSlowCardMsgProc
函数描述:基站接收到待测卡消息进行接收消息的检查和组包处理
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaRxSlowCardMsgProc(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg)
{
	uint8 u8TdoaStandCardLoop = 0; //遍历组包消息结构中所有的快发卡数据

	if(pstCardRxMsg->u64RxCardTimestamp != 0)  //接收待测卡的信息的时间戳有效
	{
		for(u8TdoaStandCardLoop = 0; u8TdoaStandCardLoop < TDOA_STANDCARD_MAX_NUM; u8TdoaStandCardLoop++) //准备遍历与所有的快发卡进行数据组包
		{
			if(gstTdoaMsgArray[u8TdoaStandCardLoop].stPreQuickCardMsg.u16CardId != 0) //检测存在快发卡的组包数据结构体
			{
				//进行数据的组包
				TdoaCardMsdBuildUnity(pstCardRxMsg, &gstTdoaMsgArray[u8TdoaStandCardLoop], u8TdoaStandCardLoop); //存入到快发空间 
				//TdoaCardMsdBuildUnityNew(pstCardRxMsg, &gstTdoaMsgArray[u8TdoaStandCardLoop], u8TdoaStandCardLoop); //存入到快发空间 
            }
		}
	}

	return;
}

/*************************************************************
函数名称:TdoaRxQuickCardMsgProc
函数描述:基站接收到快发标签消息进行接收消息的检查和组包处理
参数说明:
修改说明:
    作者:何宗峰
修改时间:2018-09-03

**************************************************************/
void TdoaRxQuickCardMsgProc(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg)
{
	int iStandNum = 0;
	int iIdleArrayNum = -1;
	uint8 u8TdoaStandCardLoop = 0; //遍历组包消息结构中所有的快发卡数据
	uint8 u8IfNewCardFlag = FALSE;
	//TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray[] = TdoaGetLocalMsgArrayStructurePtr();

	u8IfNewCardFlag = TdoaCheckRxMsgStandardID(pstCardRxMsg); 
	if(u8IfNewCardFlag == TRUE)//新生快发 未被标记的快发卡
	{	  
		//获取空闲的组包缓冲区结构
		for(iStandNum = 0; iStandNum < TDOA_STANDCARD_MAX_NUM; iStandNum++)
		{
			if(gstTdoaMsgArray[iStandNum].stPreQuickCardMsg.u16CardId == 0)
			{
				//若数据结构中快发卡id未被标记则认为有空闲的组包缓冲区结构
				iIdleArrayNum = iStandNum;
				break;
			}
		}

		//若没有空闲的组包缓冲区，则不进行新的快发卡记录
		if (iIdleArrayNum == -1)
		{
			return;
		}
		//进行指定组包空闲缓冲区进行初始化
		memset(&gstTdoaMsgArray[iIdleArrayNum], 0, sizeof(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S));
		//进行数据的组包
		TdoaCardMsdBuildUnity(pstCardRxMsg, &gstTdoaMsgArray[iIdleArrayNum], iIdleArrayNum); 
		//TdoaCardMsdBuildUnityNew(pstCardRxMsg, &gstTdoaMsgArray[iIdleArrayNum], iIdleArrayNum); 

		gstTdoaMsgArray[iIdleArrayNum].u16StandCardStartSeq = pstCardRxMsg->u16Seqnum;
	}
	else if(u8IfNewCardFlag == FALSE)   //已有记录
	{
		for(u8TdoaStandCardLoop = 0; u8TdoaStandCardLoop < TDOA_STANDCARD_MAX_NUM; u8TdoaStandCardLoop++)
		{
			if(gstTdoaMsgArray[u8TdoaStandCardLoop].stPreQuickCardMsg.u16CardId == pstCardRxMsg->u16CardId)
			{
				iIdleArrayNum = u8TdoaStandCardLoop;
				break;
			}
		}

		//若遍历所有的组包缓冲区未找到对应的快发卡id，则进行返回
		if (iIdleArrayNum == -1)
		{
			return;
		}
		
		//进行数据的组包
		TdoaCardMsdBuildUnity(pstCardRxMsg, &gstTdoaMsgArray[iIdleArrayNum], iIdleArrayNum); 
		//TdoaCardMsdBuildUnityNew(pstCardRxMsg, &gstTdoaMsgArray[iIdleArrayNum], iIdleArrayNum); 

	}
	else
	{
		PrintfUtil_vPrintf("the TdoaCheckRxMsgStandardID array was full ,get a wrong menber \n");
	}

	return;
}




