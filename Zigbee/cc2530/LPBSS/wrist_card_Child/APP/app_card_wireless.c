
#include "app_card_wireless.h"
#include <RadioProto.h>
#include "lpbssnwk.h"
#include "hal_mcu.h"
#include "app_flash.h"
#include "rf.h"
#include <types.h>
#include <timer_event.h>
#include <string.h>
#include "bsp_key.h"
#include "hal_adc.h"

LF_CARD_STATE_E cardState;    //card state, 4 kinds: free, read, wait, write.

uint32 retransBeginClock;    //重传计时
uint32 excitateClock;   //激励间隔计时

CARD_INFO_UNKNOWN_ITEM_POSITION_HANDLE unkownItemPosition; //存放baseinfo以外信息的位置,即信息中的UNKOWN类型
APP_WORK_STATE_T         s_stAppWork;
LF_TRANSLATION_INFO_T transInfo;

extern uint16 endDevID;   //卡自己的ID
extern uint16 midDevID;   //卡座ID
extern LPBSS_device_ID_e endDevType;
extern RADIO_DEV_CARD_LOC_T     s_stAppTxPkt;

uint8 sendframe[130] = {0};//128
uint8 sendframeLen = 0;

uint8 recframe[130] = {0};//128
uint8 recframeLen = 0;

#ifdef DEV_CARD_PROJ
    uint8 LF_modified;  //激励修改信息标志
    static uint8 s_u8BaseInfoLen = 0;
    extern DEV_CARD_BASIC_INFO_T    s_stAppBasicInfo;
#endif

bool app_set_workstate(UINT8 *pu8Buf, UINT8 u8Len)
{
    bool rs;
    if(!pu8Buf) return false;

    rs = BSP_FLASH_Erase(WORK_STATE_PG);
    if(!rs) return false;

    rs = BSP_FLASH_Write(WORK_STATE_PG, 0, pu8Buf, u8Len);
    if(!rs) return false;

    return true;
}

bool app_set_worktype(UINT8 *pu8Buf, UINT8 u8Len)
{
    bool rs;
    if(!pu8Buf) return false;

    rs = BSP_FLASH_Erase(WORK_TYPE_PG);
    if(!rs) return false;

    rs = BSP_FLASH_Write(WORK_TYPE_PG, 0, pu8Buf, u8Len);
    if(!rs) return false;

    return true;
}

bool app_set_baseinfo(UINT8 *pu8Buf, UINT16 u16Len)
{
    bool rs;
    if(!pu8Buf||u16Len>HAL_FLASH_PAGE_SIZE) return false;

    rs = BSP_FLASH_Erase(BASE_INFO_PG);
    if(!rs) return false;

    rs = BSP_FLASH_Write(BASE_INFO_PG, 0, pu8Buf, u16Len);
    if(!rs) return false;

    return true;
}

UINT8 app_get_workstate(UINT8 *pu8Buf, UINT8 u8Len)
{
    bool rs;

    if (!u8Len || !pu8Buf || u8Len > sizeof(s_stAppTxPkt.u8WorkType))
        return 0;
    rs = BSP_FLASH_Read(WORK_STATE_PG, 0, pu8Buf, sizeof(UINT8));

    if(!rs)
        return 0;
    else
        return u8Len;
}

UINT8 app_get_worktype(UINT8 *pu8Buf, UINT8 u8Len)
{
    bool rs;

    if (!u8Len || !pu8Buf || u8Len > sizeof(DEV_CARD_BASIC_INFO_T))
        return 0;
    rs = BSP_FLASH_Read(WORK_TYPE_PG, 0, pu8Buf, sizeof(DEV_CARD_BASIC_INFO_T));

    if(!rs)
        return 0;
    else
        return u8Len;
}

UINT16 app_get_baseinfo(UINT8 *pu8Buf, UINT16 u16Len)   //对应app_set_baseinfo()函数
{
    bool rs;

    if (!u16Len || u16Len>HAL_FLASH_PAGE_SIZE || !pu8Buf)
        return 0;
    rs = BSP_FLASH_Read(BASE_INFO_PG, 0, pu8Buf, u16Len);

    if(!rs)
        return 0;
    else
        return u16Len;
}


VOID app_StopReport(VOID)
{
    if (s_stAppWork.bReportOn) // 如果在上报信息
    {
        s_stAppWork.bReportOn = false;
        s_stAppWork.u8ReportInfoCnt = 0;

        if((cardState == CARD_STATE_FREE)||(cardState == CARD_STATE_WAIT))
        {
            RF_ReceiveOff();
        }
    }
}

VOID app_StartReport(VOID)
{
    s_stAppWork.bReportOn = true;
    s_stAppWork.u8ReportInfoCnt = 5;
    event_timer_set(EVENT_REPORT_MSG);
}

VOID app_SleepOff(UINT32 u32Event)
{
    s_stAppWork.u32EventHold |= u32Event;
}

VOID app_SleepOn(UINT32 u32Event)
{
    s_stAppWork.u32EventHold &= ~u32Event;
}

bool writeInfoFrame(uint8* buf, uint8 frameSeq, int len)
{
    uint8 pg;
    uint16 offset;
    uint8 backupZone;
    uint8 rs;

    if((len > MAX_EDEV_DATA_LEN)||(!buf))
    {
        return false;
    }

    if(frameSeq == 0)
    {
        //app_set_workstate(buf, 1);
        rs = app_set_worktype(buf, 1);
        if(!rs) return false;

        rs = app_set_baseinfo(buf+1, len-1);
        if(!rs) return false;

#ifdef DEV_CARD_PROJ
        s_u8BaseInfoLen = (len-3)>0?(len-3):0;
#endif
    }
    else
    {
        if(unkownItemPosition.usingZone == ZONE_1)
            backupZone = ZONE_2;
        else
            backupZone = ZONE_1;

        if(frameSeq == 1)
        {
            unkownItemPosition.position[backupZone].tailPage =
                unkownItemPosition.position[backupZone].headPage;
            unkownItemPosition.position[backupZone].tailOffset =
                unkownItemPosition.position[backupZone].headOffset;
            unkownItemPosition.position[backupZone].infoItemLen = 0;
            unkownItemPosition.position[backupZone].itemFrameSum = 0;
        }
        pg = unkownItemPosition.position[backupZone].tailPage;
        offset = unkownItemPosition.position[backupZone].tailOffset;
        if((pg >= HAL_NV_PAGE_END)&&(offset+len >= HAL_FLASH_PAGE_SIZE)) //防止写入越界
            return false;

        if(offset+len <= HAL_FLASH_PAGE_SIZE)
        {
            rs = app_FLASH_Write(pg, offset, buf, len);
            if(!rs)
                return false;

            unkownItemPosition.position[backupZone].tailPage = pg;
            unkownItemPosition.position[backupZone].tailOffset = offset + len;
        }
        else  //未测试
        {
            rs = app_FLASH_Write(pg, offset, buf, HAL_FLASH_PAGE_SIZE-offset);
            if(!rs)
                return false;

            rs = app_FLASH_Write(pg+1, 0, buf+(HAL_FLASH_PAGE_SIZE-offset)
                , len+offset-HAL_FLASH_PAGE_SIZE);
            if(!rs)
                return false;

            pg += 1;
            offset = len+offset-HAL_FLASH_PAGE_SIZE;
            unkownItemPosition.position[backupZone].tailPage = pg;
            unkownItemPosition.position[backupZone].tailOffset = offset;
        }

        unkownItemPosition.position[backupZone].infoItemLen += len;//*(buf+3);//cnt;
        unkownItemPosition.position[backupZone].itemFrameSum += 1;
    }
    return true;
}

void resetTransInfo(void) //LF_TRANSLATION_INFO_T transInfo
{
    transInfo.frameSeq = 0;
    transInfo.frameSum = 0;
    transInfo.isRetrans = false;
    transInfo.retransNum = 0;
}

void jumpIntoCardState(LF_CARD_STATE_E state)
{
    cardState = state;
    retransBeginClock = BSP_GetSysTick();
    if(state != CARD_STATE_WAIT)
    {
    	resetTransInfo();
    }
    if((state == CARD_STATE_FREE)||(state == CARD_STATE_WAIT))
    {
        RF_ReceiveOff();
        if(state == CARD_STATE_FREE)
        {
            if(s_stAppWork.u8IsInfochange == 3)
                app_StartReport();
        }

        //读或写成功或失败，都开启休眠
        app_SleepOn(EVENT_READ_MSG);
        app_SleepOn(EVENT_WRITE_MSG);
    }
    else
    {
        getCardInfoHandle();
        RF_ReceiveOn();

        if(state == CARD_STATE_READ)
        {
            if(!isNewCard())
            {
                transInfo.frameSum += 1;
            }
            transInfo.frameSum += unkownItemPosition.position[unkownItemPosition.usingZone].itemFrameSum;

            app_SleepOff(EVENT_READ_MSG);
        }
        else
        {
            app_SleepOff(EVENT_WRITE_MSG);
        }
    }
}


void resetRetransTime(void)
{
    transInfo.isRetrans = true;
    retransBeginClock = BSP_GetSysTick();
}


BOOL getCardInfoHandle()
{
    //读取卡信息HANDLE
    app_FLASH_Read(FLASH_CARD_INFO_HANDLE_PAGE
        , (FLASH_CARD_INFO_HANDLE_OSET), (uint8*)&unkownItemPosition, sizeof(CARD_INFO_UNKNOWN_ITEM_POSITION_HANDLE));

    //未写入卡信息或卡信息错误
    if((unkownItemPosition.usingZone != ZONE_1)
       &&(unkownItemPosition.usingZone != ZONE_2))
        return false;

    //未写入卡信息或卡信息错误
    if((unkownItemPosition.position[ZONE_1].headPage != FLASH_CARD_INFO_DETAIL_PAGE_1)
       ||(unkownItemPosition.position[ZONE_1].headOffset != FLASH_CARD_INFO_DETAIL_OSET_1))
    {
        return false;
    }
    if((unkownItemPosition.position[ZONE_2].headPage != FLASH_CARD_INFO_DETAIL_PAGE_2)
       ||(unkownItemPosition.position[ZONE_2].headOffset != FLASH_CARD_INFO_DETAIL_OSET_2))
    {
        return false;
    }

    return true;
}


BOOL groupFrame(LPBSS_LF_WR_MSGType frameType, int8 frameSum, int8 frameSeq)
{
    uint8 rs;
    uint16 len = 0;
    uint8 dataType = 0;
    uint8 pg = 0;
    uint16 offset = 0;

    uint8 usingZone;

    app_eDev_hdr_t s_devHeader;
    app_eDev_Data_t s_devData;

    DEV_CARD_DESC_INFO_T* s_cardDescInfo;

    if(frameSeq == 0)
    {
        dataType = BASEINFO;
    }
    else
    {
        dataType = UNKNOWN;
    }

    //卡只会发送这三种帧。第一种才有数据，后两种只有帧头。
    if((frameType != READ_DATA)
        &&(frameType != READY)
        &&(frameType != WRITE_DATA_ACK))
    {
        return false;
    }

    memset(sendframe,0,130);
    //app_eDev_hdr_t
    s_devHeader.MSGType = (uint8)frameType;
    s_devHeader.eDevType = endDevType;
    s_devHeader.eDevID = endDevID;
    s_devHeader.mDevID = midDevID;
    s_devHeader.dataLen = sizeof(app_eDev_Data_t);
    //app_eDev_Data_t
    s_devData.frameSum = frameSum;
    s_devData.frameSeq = frameSeq;
    s_devData.datatype = dataType;
    s_devData.len = 0;

    memcpy(sendframe, &s_devHeader, sizeof(app_eDev_hdr_t));
    memcpy(sendframe+sizeof(app_eDev_hdr_t), &s_devData, sizeof(app_eDev_Data_t));

    sendframeLen = sizeof(app_eDev_hdr_t)+sizeof(app_eDev_Data_t);

    if(frameType == READ_DATA)
    {
        if(frameSum == 0)   //未写入真实卡信息数据
            return true;

        if(frameSeq == 0)
        {
            //radio协议中的baseinfo+descinfo=lpbss协议中的baseinfo
            rs = app_get_worktype(sendframe+sendframeLen, sizeof(DEV_CARD_BASIC_INFO_T));
            if(!rs) return false;

            rs = app_get_baseinfo(sendframe+sendframeLen+sizeof(DEV_CARD_BASIC_INFO_T),102);
            if(rs != 102) return false;
            s_cardDescInfo = (DEV_CARD_DESC_INFO_T*)(sendframe+sendframeLen+sizeof(DEV_CARD_BASIC_INFO_T));

            len = sizeof(app_eDev_BaseInfo_t)+s_cardDescInfo->u16Len;   //infoLen

            sendframeLen += len;
        }
        else
        {
            usingZone = unkownItemPosition.usingZone;
            if(frameSeq < unkownItemPosition.position[usingZone].itemFrameSum)
                len = MAX_EDEV_DATA_LEN;
            else
                len = unkownItemPosition.position[usingZone].infoItemLen%MAX_EDEV_DATA_LEN;

            if(len == 0)
                len = MAX_EDEV_DATA_LEN;//防止刚好整除
            pg = unkownItemPosition.position[usingZone].headPage+
                (MAX_EDEV_DATA_LEN*(frameSeq-1))/HAL_FLASH_PAGE_SIZE;
            offset = (MAX_EDEV_DATA_LEN*(frameSeq-1))%HAL_FLASH_PAGE_SIZE;

            if(offset+len < HAL_FLASH_PAGE_SIZE)
            {
                rs = app_FLASH_Read(pg, offset, sendframe+sendframeLen, len);
                if(!rs) return false;
            }
            else
            {
                rs = app_FLASH_Read(pg, offset, sendframe+sendframeLen, HAL_FLASH_PAGE_SIZE-offset);
                if(!rs) return false;

                rs = app_FLASH_Read(pg+1, 0, sendframe+sendframeLen+(HAL_FLASH_PAGE_SIZE-offset)
                    , len+offset-HAL_FLASH_PAGE_SIZE);
                if(!rs) return false;
            }

            sendframeLen += len;
        }
        s_devData.len = len;
        s_devHeader.dataLen = len+sizeof(app_eDev_Data_t);

        memcpy(sendframe, &s_devHeader, sizeof(app_eDev_hdr_t));
        memcpy(sendframe+sizeof(app_eDev_hdr_t), &s_devData, sizeof(app_eDev_Data_t));
    }


    return true;
}


/*
* 卡信息上报。发送第一帧和重传时调用这里。
*/
void app_ReadProc(void)
{
    bool rs;

    if(cardState != CARD_STATE_READ)
    {
        return;
    }

    //重传与传第一帧的区别是组帧和发送不同
    if((transInfo.frameSeq >= transInfo.frameSum)&&(transInfo.frameSum != 0))   //seq出错
    {
        jumpIntoCardState(CARD_STATE_FREE);
        return;
    }

    //===========下面传输信息帧==============
    rs = groupFrame(READ_DATA, transInfo.frameSum, transInfo.frameSeq);
    if(!rs)
    {
        jumpIntoCardState(CARD_STATE_WAIT);
        resetTransInfo();
        return;
    }

    RF_SendPacket(midDevID, POS_STATION_PANID, sendframe, sendframeLen);//BROADCAST_ADDR dstaddr

    //重新开始重传计时。//change in bsp_SysTick_Update()
    resetRetransTime();

#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
}

void app_ReadRetransProc(void)
{
    bool rs;

    if(cardState != CARD_STATE_READ)//||(cardState != CARD_STATE_WRITE))
        return;

    if(transInfo.isRetrans == true)    //发送第一帧
        transInfo.retransNum++;
    else
        return;

    rs = groupFrame(READ_DATA, transInfo.frameSum, transInfo.frameSeq);
    if(!rs)
    {
        jumpIntoCardState(CARD_STATE_WAIT);
        resetTransInfo();
        return;
    }

    RF_SendPacket(midDevID, POS_STATION_PANID, sendframe, sendframeLen);//BROADCAST_ADDR dstaddr

    retransBeginClock = BSP_GetSysTick();

#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
}

void app_ReadACKProc(uint8* buf)
{
    bool rs;
    uint8 frameSum;
    uint8 frameSeq;

    if(!buf)
        return;

    app_eDev_Data_t* lpbss_eDev_data;
    lpbss_eDev_data = (app_eDev_Data_t*)(buf+sizeof(app_eDev_hdr_t));

    frameSum = lpbss_eDev_data->frameSum;
    frameSeq = lpbss_eDev_data->frameSeq;

    if(cardState != CARD_STATE_READ)
    {
        return;
    }
    //对比帧的序号
    if(frameSum != transInfo.frameSum)
        return;

    //传输完成
    if((frameSeq >= transInfo.frameSum-1)
    ||(frameSum == 0))
    {
        jumpIntoCardState(CARD_STATE_WAIT);
        resetTransInfo();
        return;
    }

    if(frameSeq == transInfo.frameSeq)  //确认了本帧    //重传计时、计数清零
    {
        //更新本地传输状态
        transInfo.frameSeq++;
        transInfo.retransNum = 0;
    }
    else if(frameSeq == transInfo.frameSeq-1)   //确认的是前一帧，再传一遍
    {
        transInfo.retransNum = 0;
    }
    else
        return;

    //传输下一帧
    rs = groupFrame(READ_DATA, transInfo.frameSum, transInfo.frameSeq);
    if(!rs)
    {
        jumpIntoCardState(CARD_STATE_WAIT);
        resetTransInfo();
        return;
    }

    RF_SendPacket(midDevID, POS_STATION_PANID, sendframe, sendframeLen);//dstaddr

    resetRetransTime();   //ticktack中断函数修改

#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
}

bool saveCardInfoHandle(void)
{
    bool rs;
    rs = app_FLASH_Write(FLASH_CARD_INFO_HANDLE_PAGE,
        FLASH_CARD_INFO_HANDLE_OSET, (uint8*)&unkownItemPosition, sizeof(CARD_INFO_UNKNOWN_ITEM_POSITION_HANDLE));

    return rs;
}

//初始化卡信息
void cardInfoInit()
{
    unkownItemPosition.usingZone = ZONE_1;

    unkownItemPosition.position[ZONE_1].headPage = FLASH_CARD_INFO_DETAIL_PAGE_1;
    unkownItemPosition.position[ZONE_1].tailPage = FLASH_CARD_INFO_DETAIL_PAGE_1;
    unkownItemPosition.position[ZONE_1].headOffset = FLASH_CARD_INFO_DETAIL_OSET_1;
    unkownItemPosition.position[ZONE_1].tailOffset = FLASH_CARD_INFO_DETAIL_OSET_1;
    unkownItemPosition.position[ZONE_1].pading = 0;
    unkownItemPosition.position[ZONE_1].itemFrameSum = 0;
    unkownItemPosition.position[ZONE_1].infoItemLen = 0;

    unkownItemPosition.position[ZONE_2].headPage = FLASH_CARD_INFO_DETAIL_PAGE_2;
    unkownItemPosition.position[ZONE_2].tailPage = FLASH_CARD_INFO_DETAIL_PAGE_2;
    unkownItemPosition.position[ZONE_2].headOffset = FLASH_CARD_INFO_DETAIL_OSET_2;
    unkownItemPosition.position[ZONE_2].tailOffset = FLASH_CARD_INFO_DETAIL_OSET_2;
    unkownItemPosition.position[ZONE_2].pading = 0;
    unkownItemPosition.position[ZONE_2].itemFrameSum = 0;
    unkownItemPosition.position[ZONE_2].infoItemLen = 0;

    saveCardInfoHandle();
}


BOOL isNewCard(void)
{
    uint16 descinfoLen;
    uint8 tmp[2];
    bool rs;

    //读取baseinfo信息，判断是否为空
    rs = app_get_worktype(tmp, 1);
    if(!rs) return false;

    if(tmp[0] == 0xFF)    //worktype
        return true;

    rs = app_get_baseinfo(tmp, 2);
    if(!rs) return false;

    descinfoLen = tmp[0]|(tmp[1]<<8);
    if((descinfoLen == 0)||(descinfoLen > 102))   //len
    {
        return true;
    }
    return false;
}

//初始化卡
void app_CardStateInit(void)
{
    //判断卡内是否有信息
    if(!getCardInfoHandle())//从FLASH读取卡信息的句柄
    {
        cardInfoInit();
    }

#ifdef DEV_CARD_PROJ
    LF_modified = 0;
#endif
    jumpIntoCardState(CARD_STATE_FREE);
}

/*
* 卡信息写入
*/
void app_WriteProc(uint8* buf)
{
    uint8 frameSum;
    uint8 frameSeq;
    uint8 dataLen;
    bool rs;

    if(!buf)
        return;

    app_eDev_Data_t* lpbss_eDev_data;

    lpbss_eDev_data = (app_eDev_Data_t*)(buf+sizeof(app_eDev_hdr_t));

    frameSum = lpbss_eDev_data->frameSum;
    frameSeq = lpbss_eDev_data->frameSeq;
    dataLen = lpbss_eDev_data->len;

    if(cardState == CARD_STATE_WAIT)
    {
        jumpIntoCardState(CARD_STATE_WRITE);
    }
    if(cardState != CARD_STATE_WRITE)   //状态错误
    {
        return;
    }

    if(frameSeq == 0)   //the first frame to write
    {
        transInfo.frameSum = frameSum;
        transInfo.frameSeq = 0;
    }
    else
    {
        //对比帧的序号
        if(frameSum != transInfo.frameSum)
            return;

        //更新本地传输状态
        if(frameSeq == transInfo.frameSeq+1)
        {
            transInfo.frameSeq++;
        }
        else// if(frameSeq != transInfo.frameSeq) //收到的不是本帧或下一帧
        {
            return;
        }
    }
    transInfo.retransNum = 0;

    rs = writeInfoFrame(buf+sizeof(app_eDev_hdr_t)+sizeof(app_eDev_Data_t),frameSeq, dataLen);
    if(!rs)
    {
        jumpIntoCardState( CARD_STATE_WAIT );
        resetTransInfo();
        return;
    }

    rs = groupFrame(WRITE_DATA_ACK, transInfo.frameSum, transInfo.frameSeq);
    if(!rs)
    {
        jumpIntoCardState( CARD_STATE_WAIT );
        resetTransInfo();
        return;
    }

    RF_SendPacket(midDevID, POS_STATION_PANID, sendframe, sendframeLen);//dstaddr

    resetRetransTime();   //ticktack中断函数修改

    //传输完成
    if((frameSeq+1) >= frameSum)
    {
        //store cardInfoHandle
        if(unkownItemPosition.usingZone == ZONE_1)
        {
            unkownItemPosition.usingZone = ZONE_2;
        }
        else
        {
            unkownItemPosition.usingZone = ZONE_1;
        }
        rs = saveCardInfoHandle();
        if(!rs)
        {
            jumpIntoCardState( CARD_STATE_WAIT );
            resetTransInfo();
            return;
        }

#ifdef DEV_CARD_PROJ
        s_stAppWork.u8IsInfochange = 3;
        s_stAppWork.u16DescLen = s_u8BaseInfoLen;
        app_get_worktype((UINT8*)&s_stAppBasicInfo, sizeof(DEV_CARD_BASIC_INFO_T));
#endif

        jumpIntoCardState( CARD_STATE_WAIT );
        transInfo.isRetrans = true; //ACK重传

        event_timer_add(EVENT_WRITE_ACK_RETRANS_MSG, RETRANS_TIMEOUT);

#ifdef DEV_CARD_PROJ
        LF_modified = 1;
        event_timer_add(EVENT_INFO_MASK_MSG, INFO_MASK_TIME);
#endif
    }

#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
}

void app_WriteACKRetransProc(void)
{

    if((cardState == CARD_STATE_FREE)||(cardState == CARD_STATE_READ))
    {
        return;
    }

    transInfo.retransNum++;

    groupFrame(WRITE_DATA_ACK, transInfo.frameSum, transInfo.frameSeq);

    RF_SendPacket(midDevID, POS_STATION_PANID, sendframe, sendframeLen);//dstaddr

    transInfo.isRetrans = true;
    retransBeginClock = BSP_GetSysTick();

    if(transInfo.retransNum < 2)
    {
        event_timer_add(EVENT_WRITE_ACK_RETRANS_MSG, RETRANS_TIMEOUT);
    }
    else
    {
        event_timer_del(EVENT_WRITE_ACK_RETRANS_MSG);
    }

#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif    //设置开始重传计时。如果需要，把帧序减一。
}

/*
 * LF数据处理
 */
void app_ResponseLFProc(void)
{
    volatile uint16 sCardAddr = 0;
    uint8  sDevCmdType = 0;

    //低电2.1V时不允许进行读写
    if (!HalAdcCheckVdd(VDD_MIN_NV))
        return;

    if(LFData[LF_DATA_LENORMATCH] != RECEIVE_MATCH_BYTE)
    {
        return;
    }
    sCardAddr = (LFData[LF_DATA_CARDID_H]<<8)|LFData[LF_DATA_CARDID_L];
    sDevCmdType = LFData[LF_DATA_CMDANDTYPE]&0x0f;              //低4位表示读写。
    midDevID = LFData[LF_DATA_DEVID_H]<<8|LFData[LF_DATA_DEVID_L];

    memset(LFData,0,LF_TO_CARD_LEN+1);

    if((sCardAddr == endDevID)
        ||(sCardAddr == 0xFFFF)
        ||(sCardAddr == 0)
        ||(cardState == CARD_STATE_FREE))
    {
        //更新激励超时时间
        excitateClock = BSP_GetSysTick();
    #ifdef _DEBUG_TICK
        Report_tick(8,excitateClock);
    #endif //_DEBUG_TICK
    }

    if(sDevCmdType == LF_CMD_READ)
    {
        if(cardState == CARD_STATE_FREE)
        {
            jumpIntoCardState(CARD_STATE_READ);
            event_timer_set(EVENT_READ_MSG);
        }
    }
    else if(sDevCmdType == LF_CMD_WRITE)
    {
        if(cardState == CARD_STATE_WAIT)
        {
            if((sCardAddr != endDevID)&&(sCardAddr != 0xFFFF))
            {
                jumpIntoCardState(CARD_STATE_READ);
                event_timer_set(EVENT_READ_MSG);
                return;
            }
            jumpIntoCardState(CARD_STATE_WRITE);

            groupFrame(READY, 0, 0);
            retransBeginClock = BSP_GetSysTick();
            RF_SendPacket(midDevID,POS_STATION_PANID,sendframe, sendframeLen);
        }
        else if(cardState == CARD_STATE_WRITE)    //write状态收到任意写激励都上报READY
        {
            groupFrame(READY, 0, 0);
            retransBeginClock = BSP_GetSysTick();
            RF_SendPacket(midDevID,POS_STATION_PANID,sendframe, sendframeLen);
        }
        else if(cardState == CARD_STATE_FREE)    //free状态收到任意写激励都上报卡信息
        {
            jumpIntoCardState(CARD_STATE_READ);
            event_timer_set(EVENT_READ_MSG);
        }
    }
    return;
}


/*
 *  LF信息接收处理
 */
void app_recv_card_proc(uint8 *buf)
{
    app_eDev_hdr_t* lpbss_eDev_hdr;

    if(!buf)
        return;

    lpbss_eDev_hdr = (app_eDev_hdr_t*)buf;

    uint8 msgType = lpbss_eDev_hdr->MSGType;
    LPBSS_device_ID_e eDevType = (LPBSS_device_ID_e)lpbss_eDev_hdr->eDevType;
    uint16 eDevAddr = lpbss_eDev_hdr->eDevID;
    uint16 mDevID = lpbss_eDev_hdr->mDevID;
    uint16 dataLen = lpbss_eDev_hdr->dataLen;

    if((eDevType != endDevType)||(eDevAddr != endDevID)||(midDevID != mDevID))
    {
        return;
    }

    if((dataLen < sizeof(app_eDev_Data_t))||(dataLen > MAC_MAX_FRAME_SIZE -sizeof(app_eDev_hdr_t)))
        return;

    memset(recframe,0,130);
    switch(msgType)//msgType,lpbss_header->msgtype
    {
    case READ_DATA_ACK:
    {
        if(cardState == CARD_STATE_READ)
        {
            memcpy(recframe, buf, sizeof(app_eDev_hdr_t)+dataLen);
            event_timer_set(EVENT_READ_ACK_MSG);
        }
        break;
    }
    case WRITE_DATA:
    {
        if(cardState == CARD_STATE_WRITE)
        {
            memcpy(recframe, buf, sizeof(app_eDev_hdr_t)+dataLen);
            event_timer_set(EVENT_WRITE_MSG);
        }
        break;
    }
    default:
        break;
    }
    return;
}

/*
 *  卡的重传和超时信息更新
 */
void cardTransInfoUpdata(void)
{
    if(cardState == CARD_STATE_FREE)
        return;

    if((BSP_GetSysTick() - excitateClock) >= EXCITATION_TIMEOUT)
    {
        jumpIntoCardState(CARD_STATE_FREE);
    #ifdef _DEBUG_TICK
        uint32 u32Tick = BSP_GetSysTick();
        uint8 xBuff[11]={0};
        xBuff[0]=0x06;
        xBuff[1]=cardState;
        xBuff[2]=(uint8)(u32Tick>>24);
        xBuff[3]=(uint8)((u32Tick>>16)&0xff);
        xBuff[4]=(uint8)((u32Tick>>8)&0xff);
        xBuff[5]=(uint8)(u32Tick&0xff);
        xBuff[6]=(uint8)(excitateClock>>24);
        xBuff[7]=(uint8)((excitateClock>>16)&0xff);
        xBuff[8]=(uint8)((excitateClock>>8)&0xff);
        xBuff[9]=(uint8)(excitateClock&0xff);

        RF_SendPacket(midDevID, POS_STATION_PANID, xBuff, 10);
    #endif
    }

    if(transInfo.isRetrans == true)
    {
        if(BSP_GetSysTick() - retransBeginClock >= RETRANS_TIMEOUT)
        {
            retransBeginClock = BSP_GetSysTick();
            if(transInfo.retransNum < 2)    //启动重传
            {
                if(cardState == CARD_STATE_READ)
                {
                    event_timer_set(EVENT_READ_RETRANS_MSG);
                }
                else    // (cardState == CARD_STATE_WRITE)||(cardState == CARD_STATE_WAIT)
                {
                    event_timer_set(EVENT_WRITE_ACK_RETRANS_MSG);
                }
            }
            else    //跳回WAIT状态
            {
                resetTransInfo();
                if(cardState != CARD_STATE_WAIT)
                {
                    jumpIntoCardState(CARD_STATE_WAIT);
                }
            }
        }
    }
    else    //收到写激励后未收到写信息的超时
    {
        if(cardState == CARD_STATE_WRITE)
        {
            if(BSP_GetSysTick() - retransBeginClock >= RETRANS_TIMEOUT*7)
            {
        #ifdef _DEBUG_TICK
                uint32 u32Tick = BSP_GetSysTick();
                uint8 xBuff[11]={0};
                xBuff[0]=0x03;
                xBuff[1]=cardState;
                xBuff[2]=(uint8)(u32Tick>>24);
                xBuff[3]=(uint8)((u32Tick>>16)&0xff);
                xBuff[4]=(uint8)((u32Tick>>8)&0xff);
                xBuff[5]=(uint8)(u32Tick&0xff);
                xBuff[6]=(uint8)(retransBeginClock>>24);
                xBuff[7]=(uint8)((retransBeginClock>>16)&0xff);
                xBuff[8]=(uint8)((retransBeginClock>>8)&0xff);
                xBuff[9]=(uint8)(retransBeginClock&0xff);
                RF_SendPacket(midDevID, POS_STATION_PANID, xBuff, 10);
        #endif
                //resetTransInfo();
                jumpIntoCardState(CARD_STATE_WAIT);
            }
        }
    }
}

#ifdef DEV_CARD_PROJ
void app_RemovInfoMask(void)
{
    LF_modified = 0;
}
#endif

#ifdef _DEBUG_TICK
void Report_tick(uint8 cmd,uint32 tick)
{
        uint8 xBuff[7]={0};
        xBuff[0]=cmd;
        xBuff[1]=cardState;
        xBuff[2]=(uint8)(tick>>24);
        xBuff[3]=(uint8)((tick>>16)&0xff);
        xBuff[4]=(uint8)((tick>>8)&0xff);
        xBuff[5]=(uint8)(tick&0xff);
        RF_SendPacket(midDevID, POS_STATION_PANID, xBuff, 6);
}
#endif


