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

// -------------------------------------------------------------------------------------------------------------------

#define INST_DONE_WAIT_FOR_NEXT_EVENT   1   //this signifies that the current event has been processed and instance is ready for next one
#define INST_DONE_WAIT_FOR_NEXT_EVENT_TO    2   //this signifies that the current event has been processed and that instance is waiting for next one with a timeout
                                        //which will trigger if no event coming in specified time
#define INST_NOT_DONE_YET               0   //this signifies that the instance is still processing the current event

typedef struct {
                uint8 PGdelay;

                //TX POWER
                //31:24     BOOST_0.125ms_PWR
                //23:16     BOOST_0.25ms_PWR-TX_SHR_PWR
                //15:8      BOOST_0.5ms_PWR-TX_PHR_PWR
                //7:0       DEFAULT_PWR-TX_DATA_PWR
                uint32 txPwr[2]; //
}tx_struct;

//The table below specifies the default TX spectrum configuration parameters... this has been tuned for DW EVK hardware units
const tx_struct txSpectrumConfig[8] =
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
const uint16 rfDelays[2] = {
        (uint16) ((DWT_PRF_16M_RFDLY/ 2.0) * 1e-9 / DWT_TIME_UNITS),//PRF 16
        (uint16) ((DWT_PRF_64M_RFDLY/ 2.0) * 1e-9 / DWT_TIME_UNITS)
};

typedef struct
{
	uint16 stationID;
	int8 i8rssi;
	uint8 count;
	uint8 status;
}inblinkmsg;

// -------------------------------------------------------------------------------------------------------------------
//      Data Definitions
// -------------------------------------------------------------------------------------------------------------------
#define MAX_NUMBER_OF_REPORT_RETRYS (3)         // max number of times to send/re-send the report message

#define FIXED_REPORT_DELAY                  2 //15 //ms             //tx delay when sending the (ToF) report
#define RX_ON_TIME                          2 //ms                  //the time RX is turned on before the reply from the other end
#define RX_FWTO_TIME                        (RX_ON_TIME + 5) //ms //total "RX on" time
#define BLINK_SLEEP_DELAY					1000 //ms

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// NOTE: the maximum RX timeout is ~ 65ms
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// -------------------------------------------------------------------------------------------------------------------

double inst_idist = 0;
double inst_adist = 0;
double inst_ldist = 0;
double inst_m_idist =0;
//uint32 u32inst_m_idist =0;
instance_data_t instance_data[NUM_INST] ;

uint32 LastPollTick = 0;
uint32 CurPollTick = 0;
uint32 RngInitTick = 0;
uint32 LastrevfinalTick=0;
uint32 LastRsepTick=0;


//uint32 TAGRngInitTick = 0;
//uint32 TAGReportTick = 0;

//uint32 AnchorPollTick = 0;

uint64 delayedReplyTime = 0;

//uint8  ANCHORRangeWith = 0;
uint16 tx_antennaDelay =0;

uint32 toftimesum=0;
//uint32 toftimesum1=0;
//uint8  blinktag=0;
uint8 isnewblinkon=1; //nerver poll
uint8 slotfirstinpoll=0; //this is the frist time in poll between this slot


int pre_event=-1;
int sleeptime =0;
uint8 is_in_blink=0;              //blink pro
uint16 test_tof_an_addr=0;        //tof poll start process dest addr
//uint8 card_rev_poll_continue=0;   //if station power on again ,card didn't revpoll anytime then  "is_in_blink=0"  ;slot_msg_t->u8LostNum
int rx_time=0;         //how many time this slot rev
uint8 helpreportoff=0;   //if the service counter report the help ask ,1:yes
uint8 new_2blink_tick=0;
uint32 pre_distance=0;
uint16 pre_seqnum=0;
uint8 card_last_dist_msgtype=0;   //wether the card's last distance msg have send (report to m_station, final to s_station),1,yes ,0,no, if no ,should send pre distance to m_station
uint8 pre_dist_right=0;
static uint8 new_tof_in=0;  //the card get in a new tof 
uint8 card_inblink_slot=255;  //the slot when the card  in blink
uint8 inblink_status=0;
uint16 inblink_uptime=0;
uint32 Station_Power=0x758D8D75;

uint8 u8seqnum = 0;


alloc_Slot_t my_slotlist[TOF_SLOT_LOC_PERIOD] ;    //TOF_SLOT_LOC_PERIOD  = 1000/EVERY_SLOT_TIME
alarm_addrlist_t my_alarmlist;
slot_msg_t my_cardslotmsg ,my_staslotmsg;
Sub_alarm_msg_t sub_alarmmsg;
#ifdef DEC_UWB_SUB
ts_Car_cardlist Car_revcardlist;
#endif

extern uint16 u16ShortAddr;
//uint16 test_final_dea=0;
extern uint8 bool_check_poll;
extern tsCardTypeSet CardType_5s;
extern tsCardTypeSet CardType_1s;
inblinkmsg new_inblinkmsg;
extern uint8 ever_rev_dis;
uint8 rev_retreat_ack=0;
uint8 u8TdoaCardSendCount = 0;
uint16 u16TdoaQuickCardSendCount = 0;
TOF_INST_CARD_DATA_S gstTofInstCardDate;

extern uwb_tof_distance_ts distlist[TOF_SLOT_LOC_PERIOD];


// -------------------------------------------------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------------------------------------------------


int sta_samecount =0;
void printf_event(instance_data_t *inst,int message)
{
	int cur_event = inst->testAppState;
	if(cur_event != pre_event)
	{
		PrintfUtil_vPrintf("--%d ",cur_event);
		sta_samecount =0;
	}

	pre_event = cur_event;

}
void led_station_flash(void)
{
	uint8 temp=0;
	temp = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12);//PA_10;
	if(temp)
		led_station_on();
	else
		led_station_off();
}
void instance_set_sta_status(uint8 status)  //set main station's status
{
	instance_data[0].station_status |= status;
}

void instance_reset_sta_status(uint8 status) //reset main station's status
{
	instance_data[0].station_status &= ~status;

}

uint8 instance_get_sta_status(uint8 status)   //sub station check the help status,if return 1 send uart ask help
{
	uint8 x = instance_data[0].station_status & status;
	instance_data[0].station_status &= ~status;
	return x;
}

uint8 instance_check_devchg(uint8 status)   //sub station check the help status,if return 1 send uart ask help
{
	uint8 devchg=0;
	if((status & STATION_TYPE_DEVCH_1S) && (status & STATION_TYPE_DEVCH_5S))
		return 0;
	if(status & STATION_TYPE_DEVCH_1S)  
	{
		if(my_cardslotmsg.u8DeviceType == CARD_5S)
		{
			instance_data[0].change_devtype =1 ;
			devchg =1;
		}
	}
	else if(status & STATION_TYPE_DEVCH_5S)  
	{
		if(my_cardslotmsg.u8DeviceType == CARD_1S)
		{
			instance_data[0].change_devtype =1 ;
			devchg =1;
		}
	}
	return devchg;
}

uint8 instance_setpower_rssi(uint8 type)
{
	
	//typedef enum RSSI_power{POWER_10_5DB=0x75858575,POWER_10_5DB=0x75878775,POWER_10_5DB=0x75898975,
	//	POWER_11_5DB=0x758B8B75,POWER_12_5DB=0x758D8D75,POWER_13_5DB=0x758F8F75,POWER_18_5DB=0x75999975 } RSSI_POWER;
	if(type ==1)
		Station_Power=0x75848475;  //8.0db
	else if(type ==2)
		Station_Power=0x75858575;  //8.5db
	else if(type==3)
		Station_Power=0x75878775;  //9.5db
	else if(type==4)
		Station_Power=0x75898975;  //10.5db
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
	return 1;
}

instance_data_t* TdoaGetOldLocalInstStructurePtr(void)
{
	return &instance_data[0];
}

uint16 instance_get_cardid(void)
{
	uint16 cardid=0;

	cardid = *(uint16*)(DEV_ADDRESS);
	//return cardid;
	return DEV_ADDRESS_CARDID;
}

void instance_set_insleep(void)
{
	dwt_forcetrxoff() ;
	event_timer_unset(EVENT_SLEEP_EVENT);			
	event_timer_add(EVENT_SLEEP_EVENT,4); 

}

void instance_set_AnchorPanid(uint8 type)
{
	uint16 panid;
	if(type ==ANCHOR_TOF_PRO)
	{
		instance_change_channel(ANCHOR_TOF_CHANNEL);
		panid= 0xcccc ;     //tof and help ask process
	}
	else if(type ==ANCHOR_BLINK_PRO)
	{
		instance_change_channel(ANCHOR_BLINK_CHANNEL);
		panid = 0xdddd;     //blink process 
	}
	dwt_setpanid(panid);
}

void instance_set_Alarmstatus(uint16 addr,uint8 type)  //set the slot's alarm status
{
	uint8 i,j;
	uint16 listaddr=0;
	for(i=0;i<TOF_SLOT_LOC_PERIOD;i++)
	{
		if(my_slotlist[i].slotmode == CARD_1S
			&& my_slotlist[i].allocslot[0].b1Used == USED_TOF)
		{
			memcpy(&listaddr,&my_slotlist[i].allocslot[0].dest_addr[0],ADDR_BYTE_SIZE);
			if(listaddr == addr)
			{
				if(type ==1) //set alarm
				{
					for(j=0;j<MAX_CARD_CYCLE_SEC ;j++)
						my_slotlist[i].allocslot[j].status |=UWB_CARD_STATUS_RETREAT ;
				}
				else        //reset alarm
				{
					for(j=0;j<MAX_CARD_CYCLE_SEC ;j++)
						my_slotlist[i].allocslot[j].status &= ~UWB_CARD_STATUS_RETREAT ;
				}
				return;
			}
		}
		else if(my_slotlist[i].slotmode == CARD_5S)
		{
			for(j=0;j<MAX_CARD_CYCLE_SEC ;j++)
			{
				if(my_slotlist[i].allocslot[j].b1Used == USED_TOF)
				{
					memcpy(&listaddr,&my_slotlist[i].allocslot[0].dest_addr[0],ADDR_BYTE_SIZE);
					if(listaddr == addr)
					{
						if(type ==1)
							my_slotlist[i].allocslot[j].status |=UWB_CARD_STATUS_RETREAT ;
						else
							my_slotlist[i].allocslot[j].status &= ~UWB_CARD_STATUS_RETREAT ;
						return;
					}
				}
			}
		}
	}
}

void instance_set_alarmlist(uint16 addr,uint8 type,uint8 status)
{
	int i=0,j=0;
	if(status ==1)  //set alarm
	{
		if(type==0xff)  //all
		{
			memset(&my_alarmlist,0,sizeof(alarm_addrlist_t));
			my_alarmlist.count =255;
			instance_set_sta_status(STATION_TYPE_ALARM);   //撤离
			instance_reset_sta_status(STATION_TYPE_ALARM_RESET | STATION_TYPE_ALARM_ANY);
		}
		else         //part 
		{
			if(my_alarmlist.count ==255)
				my_alarmlist.count =0;
			if(my_alarmlist.count <MAX_ALARM_NUM)
			{
				my_alarmlist.addr[my_alarmlist.count]= addr;
				my_alarmlist.count ++;
			}
			instance_set_Alarmstatus(addr,status);            //set retreat statua to every slot's card 
			instance_set_sta_status(STATION_TYPE_ALARM_ANY); 
			instance_reset_sta_status(STATION_TYPE_ALARM_RESET | STATION_TYPE_ALARM);
		}
		return;
	}
	else if(status ==0)          //reset alarm ，stop retreat
	{
		if(type==0xff)   //all
		{
			for(i=0;i<MAX_ALARM_NUM;i++)
				my_alarmlist.addr[i] == 0;
			my_alarmlist.count =0;
			instance_set_sta_status(STATION_TYPE_ALARM_RESET);
			instance_reset_sta_status(STATION_TYPE_ALARM |STATION_TYPE_ALARM_ANY);
			for(i=0;i<TOF_SLOT_LOC_PERIOD;i++)
				for(j=0;j<MAX_CARD_CYCLE_SEC ;j++)
						my_slotlist[i].allocslot[j].status  = ~UWB_CARD_STATUS_RETREAT ;
		}
	}
}


uint8 instance_inset_alarmmsg(instance_data_t *inst,uint8 start)
{
/*	int count=0,i=0;
	count = my_alarmlist.count ;
	inst->macdata_msdu[start] = count;   //0xff: all    ;other:some card
	if(count != 0xff)
	{
		for(i=0;i<count;i++)
		{
			memcpy(&inst->macdata_msdu[start+1],&my_alarmlist.addr[i],ADDR_BYTE_SIZE);
			start = start+2;
		}
	}
*/	
	inst->macdata_msdu[start]  = inst->station_status;
	return start;
}


//the anchor check it's status
uint8 instance_get_Alarmstatus(uint8 u8cur_Slot,uint8 u8mode)
{
	uint8 row=0,column=0,alarmstatus=0,i=0;
	if(instance_data[0].station_status & STATION_TYPE_ALARM)   //all retreat
		return 0xff;
	else if(instance_data[0].station_status & STATION_TYPE_ALARM_RESET)
		return 0;
	else
	{
		if(u8mode == TAG)
		{
			row=u8cur_Slot/TOF_SLOT_LOC_PERIOD;	 //0-4
			column = u8cur_Slot % TOF_SLOT_LOC_PERIOD; //0-39
			alarmstatus = my_slotlist[column].allocslot[row].status;

		}
		else
			alarmstatus = my_cardslotmsg.status;
		if(instance_data[0].station_status & STATION_TYPE_ALARM_ANY)
		{
			for(i=0;i<my_alarmlist.count;i++)
			{
				if(my_alarmlist.addr[i] == test_tof_an_addr)
				{
					alarmstatus |=UWB_CARD_STATUS_RETREAT;
					return 0xff;
				}
			}
		}
		return 0;
	}
}

void instance_check_chgdevtype(uint16 cardaddr,uint8 u8cur_Slot,uint8 *status)
{
	uint8 row=0,column=0,i,type;
	row=u8cur_Slot/TOF_SLOT_LOC_PERIOD;	 //0-4
	column = u8cur_Slot % TOF_SLOT_LOC_PERIOD; //0-39
	type = my_slotlist[column].allocslot[row].u8DeviceType;
	*status=  instance_data[0].station_status;
	if(type == CARD_1S&&CardType_5s.u8CardCnt >0)
	{
		for(i=0;i<CardType_5s.u8CardCnt;i++)
		{
			if(CardType_5s.u16CardNum[i] == cardaddr)
				*status |= STATION_TYPE_DEVCH_5S;
		}
	}
	else if(type == CARD_5S &&CardType_1s.u8CardCnt >0)
	{
		for(i=0;i<CardType_1s.u8CardCnt;i++)
		{
			if(CardType_1s.u16CardNum[i] == cardaddr)
				*status |= STATION_TYPE_DEVCH_1S;
		}
	}
}

//when uart rev alarmack data from stm32 then use this
void instance_set_helpstatus(uint16 addr)
{
	
	uint8 i,j;
	uint16 listaddr=0;
	if(instance_data[0].mode == TAG)
	{
		for(i=0;i<TOF_SLOT_LOC_PERIOD;i++)
		{
			if(my_slotlist[i].slotmode == CARD_1S
				&& my_slotlist[i].allocslot[0].b1Used == USED_TOF
				&& listaddr == addr)
			{
				for(j=0;j<MAX_CARD_CYCLE_SEC ;j++)
					my_slotlist[i].allocslot[j].status &= ~UWB_CARD_STATUS_HELP ;

			}
			else if(my_slotlist[i].slotmode == CARD_5S)
			{
				for(j=0;j<MAX_CARD_CYCLE_SEC ;j++)
				{
					if(my_slotlist[i].allocslot[j].b1Used == USED_TOF &&(listaddr == addr || addr== 0xFFFF))
					{
						my_slotlist[i].allocslot[j].status &= ~UWB_CARD_STATUS_HELP ;
					}
				}
			}
		}
	}
	else if(instance_data[0].mode == SUB_STA)
	{
		if(sub_alarmmsg.alarmaddr == addr ||addr ==0xFFFF)
		{
			sub_alarmmsg.alarmstatus &= ~UWB_CARD_STATUS_HELP;
		//	my_staslotmsg.status &= ~UWB_CARD_STATUS_HELP;
		}
		
	}
}

uint8 u16GetIdleSlot_TOF1s(uint8* pu8Index,uint8 u8cur_Slot)
{
    uint8 i,startnextslot,count=0,count1=0;
	uint8 u8column=0,row=0,temp;
	uint8 slot1=0,slot2=0 ,pu8Index_2;
	startnextslot =4;
    for(i=startnextslot; i<=TOF_SLOT_LOC_PERIOD+startnextslot; i++)   //start from the startnextslot ,circulation a cycle slot
    {
    	temp = (i+1)%(TOF_SLOT_LOC_PERIOD/2);
		u8column = (u8cur_Slot + i)%TOF_SLOT_LOC_PERIOD;
		row = (u8cur_Slot + i) / TOF_SLOT_LOC_PERIOD;
		if(my_slotlist[u8column].slotmode == L_IDLE)
		{
			count++;
			if(count ==1)
				*pu8Index = (u8column + row*TOF_SLOT_LOC_PERIOD)%SUM_SLOT_COUNT;
			if(count ==2)
			{
				pu8Index_2 = (u8column + row*TOF_SLOT_LOC_PERIOD)%SUM_SLOT_COUNT;
				return pu8Index_2;
			}
			//return 1;
		}
		else if(my_slotlist[u8column].slotmode == OTHER && temp !=0)
		{
			if(my_slotlist[u8column].allocslot[0].b1Used == USED_BLINK)
			{
				count1++;
				if(count1 ==1)
					slot1 =(u8cur_Slot +i)% SUM_SLOT_COUNT;   //the blink slot
				else if(count1 ==2)
					slot2 =(u8cur_Slot +i)% SUM_SLOT_COUNT;   //the blink slot
			}
		}
    }
	
 	if(count == 0)
 	{
 		*pu8Index  = slot1;
		pu8Index_2 = slot2;
 	}
	else if(count == 1)
	{
		pu8Index_2 = slot1;
 	}
	return pu8Index_2;

}

uint8 u16GetIdleSlot_TOF5s(uint8* pu8Index,uint8 u8cur_Slot)  //find the min range IDLE 5s_card slot
{
    uint8 i,j,startnextslot,count=0,count1=0;;
    uint8 u8column=0, slot1=0,slot2=0,pu8Index_2;
	int firstidleslot=-1,row=0;
	*pu8Index = 255;
	startnextslot = 4;
	for(i=0;i<MAX_CARD_CYCLE_SEC;i++)
	{
		row = (u8cur_Slot / TOF_SLOT_LOC_PERIOD+i)%MAX_CARD_CYCLE_SEC;
	    for(j=startnextslot; j<TOF_SLOT_LOC_PERIOD+startnextslot; j++)
	    {
	    	
		/*    if(i==(MAX_CARD_CYCLE_SEC-1) &&j ==TOF_SLOT_LOC_PERIOD+startnextslot) //equal to the u8CardSlot
			{
				*pu8Index = 255;
				break;
			}*/
			u8column = (u8cur_Slot + j)%TOF_SLOT_LOC_PERIOD;
			row = ((u8cur_Slot+j)/ TOF_SLOT_LOC_PERIOD +i)%MAX_CARD_CYCLE_SEC;
			if(my_slotlist[u8column].slotmode == CARD_5S || my_slotlist[u8column].slotmode==L_IDLE )
	    	{
		    	if(my_slotlist[u8column].slotmode == CARD_5S
				//	&& my_slotlist[u8column].num_of5s <MAX_CARD_CYCLE_SEC 
					&&my_slotlist[u8column].allocslot[row].b1Used == IDLE)
		    	{
					count++;
					if(count ==1)
						*pu8Index = (u8column + row*TOF_SLOT_LOC_PERIOD)%SUM_SLOT_COUNT;
					if(count ==2)
					{
						pu8Index_2 = (u8column + row*TOF_SLOT_LOC_PERIOD)%SUM_SLOT_COUNT;
						return pu8Index_2;
					}
			
		    	}
				else if(my_slotlist[u8column].slotmode==L_IDLE && firstidleslot==-1)  // the first idle list slot
				{
					firstidleslot = (u8column + row*TOF_SLOT_LOC_PERIOD)%SUM_SLOT_COUNT;//u8column;
				//	firstidleslot = (firstidleslot + TOF_SLOT_LOC_PERIOD)%SUM_SLOT_COUNT;  //next 1s
				}
	    	}
			else if(my_slotlist[u8column].slotmode== OTHER && count<2)
			{
				if(my_slotlist[u8column].allocslot[row].b1Used == USED_BLINK )
				{
					count1++;
					if(count1 ==1)
						slot1 =(u8cur_Slot +i)% SUM_SLOT_COUNT;   //the blink slot
					else if(count1 ==2)
						slot2 =(u8cur_Slot +i)% SUM_SLOT_COUNT;   //the blink slot
				}
			}
	    }	
		
	}
	if(count==0)
	{
		if(firstidleslot<0)
		{
			*pu8Index  = slot1;
			pu8Index_2 = slot2;
		}
		else
		{
			*pu8Index = firstidleslot;
			pu8Index_2 = (firstidleslot +TOF_SLOT_LOC_PERIOD)%SUM_SLOT_COUNT;
		}
	}
	else if(count==1)
	{
		if(firstidleslot<0)
			pu8Index_2 = slot2;
		else
			pu8Index_2 = firstidleslot;
	}
    return pu8Index_2;

}


uint8 check_slot_list(uint16 revaddr,uint8 cardtype,uint8 cur_slot)  //check if the card have been in the slotlist
{
	uint8 i=0,j=0,result=0xFF,lost=0;
	uint8 addr[2];
	uint16 temp1=0,temp2=0;
	addr[0] = (uint8)revaddr&0xff;
	addr[1] = (uint8)(revaddr>>8)&0xff;
	if(cardtype == CARD_1S)
	{
		for(j=0; j<TOF_SLOT_LOC_PERIOD; j++)
		{
			
			if(my_slotlist[j].slotmode == CARD_1S &&
				my_slotlist[j].allocslot[0].dest_addr[0]==addr[0]&&
				my_slotlist[j].allocslot[0].dest_addr[1]==addr[1])
			{
				temp1 = j;
				while(temp1 < cur_slot)
					temp1 = temp1+TOF_SLOT_LOC_PERIOD;
				result =(uint8)temp1% SUM_SLOT_COUNT;
				lost = 0;//my_slotlist[j].allocslot[0].u8LostNum;
			//	if( lost>0)
				{
					for(i=0;i<MAX_CARD_CYCLE_SEC;i++)       //old slot ,then reduce one slotnum to wait the slot
					{
						my_slotlist[j].allocslot[i].u8LostNum = lost;
					}
				}
				return result;
			}
		}
	}
	else if(cardtype == CARD_5S)
	{
		for(i=0;i<TOF_SLOT_LOC_PERIOD ;i++)
		{
			if(my_slotlist[i].slotmode == CARD_5S|| my_slotlist[i].slotmode==L_IDLE )
			{
				for(j=0; j<MAX_CARD_CYCLE_SEC; j++)
				{
					if(my_slotlist[i].allocslot[j].dest_addr[0]==addr[0]&&
						my_slotlist[i].allocslot[j].dest_addr[1]==addr[1])
					{
						result = j*TOF_SLOT_LOC_PERIOD +i;
						temp1 = ((uint16)result + SUM_SLOT_COUNT - (uint16)cur_slot) %SUM_SLOT_COUNT;
						temp2 = ((uint16)instance_data[0].next_idle_5sslot + SUM_SLOT_COUNT - (uint16)cur_slot) %SUM_SLOT_COUNT;
						my_slotlist[j].allocslot[i].u8LostNum =0;
						if((temp1<= temp2)|| temp1 <= SUM_SLOT_COUNT/2)
						{
							return result;
						}
						else
						{
							my_slotlist[j].allocslot[i].u8LostNum = LOST_TOF_MAX_NUM;
							return 255;
						}
					}
				}
			}
		}
	}
	return result;
}

void instance_setslot_alarm(uint8 cur_slot)
{
	uint8 row=0,column=0,i=0 ,cardtype;

	row=cur_slot/TOF_SLOT_LOC_PERIOD;	 //0-4
	column = cur_slot % TOF_SLOT_LOC_PERIOD; //0-39
	cardtype = my_slotlist[column].slotmode;
	if(cardtype == CARD_1S)
	{
		for(i=0;i<MAX_CARD_CYCLE_SEC;i++)  
			my_slotlist[column].allocslot[i].status |=UWB_CARD_STATUS_RETREAT;
	}
	else if(cardtype == CARD_5S)
	{
		my_slotlist[column].allocslot[row].status |=UWB_CARD_STATUS_RETREAT;
	}
}

void instance_set_sleeptick(uint8 gettick,uint8 cur_slot,uint8 next_slot)
{
	uint16 slot;
	
	if(my_cardslotmsg.b1Used == IDLE)
	{
		slot = (uint16)((uint16)next_slot+SUM_SLOT_COUNT - (uint16)cur_slot)%SUM_SLOT_COUNT;   //nextslot - curslot
		if(slot>1)
			my_cardslotmsg.sleeptick = slot*EVERY_SLOT_TIME-gettick;

		if(slot<=1 || next_slot ==255)
		{
			if(my_cardslotmsg.u8DeviceType == CARD_1S){
				slot= TOF_SLOT_LOC_PERIOD+cur_slot;
			}
			else if(my_cardslotmsg.u8DeviceType == CARD_5S){
				slot= TOF_SLOT_LOC_PERIOD *CARD_5S+cur_slot;
			}
			my_cardslotmsg.sleeptick = (slot)*EVERY_SLOT_TIME-gettick;
		}
		

	}
	else if(my_cardslotmsg.b1Used == USED_TOF)
	{
		my_cardslotmsg.u8cur_slot =cur_slot; //(cur_slot +i*TOF_SLOT_LOC_PERIOD)%(MAX_CARD_CYCLE_SEC*TOF_SLOT_LOC_PERIOD);				
	//	my_cardslotmsg.u16SeqNum = 0;
	//	my_cardslotmsg.u8LostNum =0;
		if(my_cardslotmsg.u8DeviceType == CARD_1S)
		{	

			if( next_slot!=255)
			{
				if(cur_slot ==next_slot)
					my_cardslotmsg.sleeptick = CARD_1S_SEC_TIME-gettick;
				else
				{
					slot = (uint16)((uint16)next_slot+SUM_SLOT_COUNT - (uint16)cur_slot)%SUM_SLOT_COUNT;   //nextslot - curslot ,shuld not -1
					my_cardslotmsg.sleeptick = slot*EVERY_SLOT_TIME-gettick;
				}
			}
			else if(next_slot ==255)
			{
				uint16 temp=0;
				temp= (uint16)cur_slot+SUM_SLOT_COUNT - card_inblink_slot;
				temp = temp%TOF_SLOT_LOC_PERIOD ;
				if(temp ==0)
					my_cardslotmsg.sleeptick = CARD_1S_SEC_TIME-gettick;
				else
					my_cardslotmsg.sleeptick = CARD_1S_SEC_TIME-temp*EVERY_SLOT_TIME-gettick;
			}
			
		}
		else if(my_cardslotmsg.u8DeviceType == CARD_5S)
		{
			my_cardslotmsg.sleeptick = CARD_5S_SEC_TIME-gettick;
			if(next_slot ==255)
			{
				uint16 temp=0;
				temp = (uint16)cur_slot +SUM_SLOT_COUNT - (uint16)card_inblink_slot;
				temp = temp%SUM_SLOT_COUNT ;
				my_cardslotmsg.sleeptick = CARD_5S_SEC_TIME-temp*EVERY_SLOT_TIME-gettick;
			}
		}
		//memcpy(&my_cardslotmsg.dest_addr[0],&addr,ADDR_BYTE_SIZE);         //station addr
	}

}


//when blink rev card's response ,the inset this card to the slot list
void instance_inset_slotlist(instance_data_t *inst,uint16 rev_addr)  //cur_slot:0-199  ,inst->rev_card_type,srcAddr);
{
	uint8 row=0,column=0;
	uint8 used_slot=0;
//	used_slot = inst->cur_slot;
	
//	row=used_slot/TOF_SLOT_LOC_PERIOD; 	 //0-4
//	column = used_slot % TOF_SLOT_LOC_PERIOD; //0-39
//	if(my_slotlist[column].allocslot[row].b1Used == USED_BLINK)
	{
		if(inst->rev_card_type == CARD_1S)
			used_slot = inst->next_idle_1sslot ;
		else if(inst->rev_card_type == CARD_5S)
			used_slot = inst->next_idle_5sslot ;
		else
			return;
		
	}
	row=used_slot/TOF_SLOT_LOC_PERIOD; 	 //0-4
	column = used_slot % TOF_SLOT_LOC_PERIOD; //0-39
	
	if(inst->rev_card_type == CARD_1S)
	{
		my_slotlist[column].slotmode = CARD_1S;
		for(int i=0; i<MAX_CARD_CYCLE_SEC;i++)
		{
			my_slotlist[column].allocslot[i].b1Used = USED_TOF;
			my_slotlist[column].allocslot[i].u8DeviceType = CARD_1S;
			my_slotlist[column].allocslot[i].u8LostNum = 0;
			my_slotlist[column].allocslot[i].u8cur_slot =(column +i*TOF_SLOT_LOC_PERIOD) % SUM_SLOT_COUNT;	
			my_slotlist[column].allocslot[i].sleeptick = (uint16)(TOF_SLOT_LOC_PERIOD-1)*EVERY_SLOT_TIME -3;	//3ms before
			memcpy(&my_slotlist[column].allocslot[i].dest_addr[0],&rev_addr,ADDR_BYTE_SIZE);  //card addr
		}
	}
	else if(inst->rev_card_type == CARD_5S)
	{
		my_slotlist[column].slotmode = CARD_5S;
		my_slotlist[column].num_of5s++;
		my_slotlist[column].allocslot[row].b1Used = USED_TOF;
		my_slotlist[column].allocslot[row].u8DeviceType = CARD_5S;
		my_slotlist[column].allocslot[row].u8cur_slot= used_slot;
		my_slotlist[column].allocslot[row].u8LostNum = 0;
		my_slotlist[column].allocslot[row].sleeptick= (uint16)(SUM_SLOT_COUNT-1)*EVERY_SLOT_TIME -3;	 //3ms before
		memcpy(&my_slotlist[column].allocslot[row].dest_addr[0],&rev_addr,ADDR_BYTE_SIZE);  //card addr
	}
}


//before send poll or blink , judge if send :blink  /poll /uart  or others;
//used in application 
void instance_set_cardidle(uint8 row,uint8 column)
{
	uint8 devtype ,i=0;
	devtype = my_slotlist[column].allocslot[row].u8DeviceType;
	if(devtype == CARD_5S)
	{
		if(my_slotlist[column].num_of5s>0)
			my_slotlist[column].num_of5s--;
		if(my_slotlist[column].num_of5s == 0)
			my_slotlist[column].slotmode = L_IDLE ;
		my_slotlist[column].allocslot[row].status = 0;
		my_slotlist[column].allocslot[row].b1Used = IDLE;
		my_slotlist[column].allocslot[row].u8LostNum = 0;
		my_slotlist[column].allocslot[row].u16SeqNum =0;
		my_slotlist[column].allocslot[row].dest_addr[0] = my_slotlist[column].allocslot[row].dest_addr[1] =0; 
		my_slotlist[column].allocslot[row].sleeptick =0;
		my_slotlist[column].allocslot[row].u8cur_slot =0;
	}
	else if(devtype == CARD_1S)
	{
		DBG(PrintfUtil_vPrintf("||00>> ");)
		my_slotlist[column].slotmode = L_IDLE;
		for(i=0;i<MAX_CARD_CYCLE_SEC;i++)
		{
			my_slotlist[column].allocslot[i].b1Used = IDLE;
			my_slotlist[column].allocslot[i].status = 0;
			my_slotlist[column].allocslot[i].u8LostNum = 0;
			my_slotlist[column].allocslot[i].u16SeqNum = 0;
			my_slotlist[column].allocslot[i].dest_addr[0] = my_slotlist[column].allocslot[i].dest_addr[1] =0; 
			my_slotlist[column].allocslot[i].sleeptick =0;
			my_slotlist[column].allocslot[i].u8cur_slot =0;
		}
	}

}

uint8 instance_get_listslotmsg(uint8 cur_slot)
{
	uint8 i=0,row=0,column=0,type,lost=0,devtype;
	instance_data[0].cur_slot = cur_slot;
	row=cur_slot/TOF_SLOT_LOC_PERIOD;      //0-4
	column = cur_slot % TOF_SLOT_LOC_PERIOD; //0-39 
	instance_data[0].curslot_column = column;
	instance_data[0].curslot_row = row;
	devtype = my_slotlist[column].allocslot[row].u8DeviceType;
	if(my_slotlist[column].allocslot[row].b1Used == USED_TOF)
	{
		if(devtype == CARD_5S)
			lost = my_slotlist[column].allocslot[row].u8LostNum;
		else if(devtype == CARD_1S)
		{
			for(i=0;i<MAX_CARD_CYCLE_SEC;i++)
			{
				if(my_slotlist[column].allocslot[i].u8LostNum > lost)
					lost = my_slotlist[column].allocslot[i].u8LostNum;
			}
			for(i=0;i<MAX_CARD_CYCLE_SEC;i++)
			{
				my_slotlist[column].allocslot[i].u8LostNum =lost ;
			}
		}
		if(lost >= LOST_TOF_MAX_NUM)
		{
			instance_set_cardidle(row,column);
		}
	}
	type = my_slotlist[column].allocslot[row].b1Used ;
	switch(type)
	{
		case IDLE:         //send blink an sllot itself
			return 1;
		case USED_TOF:     //just do tof to every slot card,send poll
			return 2;
		case USED_UART:    //send ranging massege to the big stm32
			return 3;
		case USED_BLINK:   //only blink but not allot itself
			return 4;      
		default:
			return 0;      //error
	}	
}

//get the seqnum of the list
uint16 instance_get_curseq(uint8 curslot) 
{
	uint8 row=0,column=0;
	uint16 seq;
	uint8 cardtype;
	
	row=curslot/TOF_SLOT_LOC_PERIOD;      //0-4
	column = curslot % TOF_SLOT_LOC_PERIOD; //0-39
	cardtype = my_slotlist[column].allocslot[row].u8DeviceType;
	if(cardtype == CARD_5S)
	{
		pre_dist_right =0;
		seq = my_slotlist[column].allocslot[row].u16SeqNum+1;
		if(my_slotlist[column].allocslot[row].m_distance!=0)
			pre_dist_right =1;
	}
	else if(cardtype == CARD_1S)
	{
		if(row==0)
			seq = my_slotlist[column].allocslot[MAX_CARD_CYCLE_SEC-1].u16SeqNum +1;
		else
			seq = my_slotlist[column].allocslot[row-1].u16SeqNum +1;
		
	}
	else
		return 0;
//	my_slotlist[column].allocslot[row].u16SeqNum = seq;
	my_slotlist[column].allocslot[row].b1Used = USED_TOF;
	my_slotlist[column].allocslot[row].m_distance = 0;
	my_slotlist[column].allocslot[row].u8LostNum++;    //when rev resp, the num set 0
	return seq;
}

uint16 get_curslot_destaddr(uint8 curslot) 
{
	uint8 row=0,column=0;
	uint16 addr;
	row=curslot/TOF_SLOT_LOC_PERIOD;      //0-4
	column = curslot % TOF_SLOT_LOC_PERIOD; //0-39 
	memcpy(&addr ,&my_slotlist[column].allocslot[row].dest_addr[0],ADDR_BYTE_SIZE);
	return addr;
}

uint8 get_curslot_cardmsg(uint16 *cardid) 
{
	uint8 row=0,column=0,type=0;
	row= instance_data[0].curslot_row;      //0-4
	column = instance_data[0].curslot_column; //0-39 
	type = my_slotlist[column].allocslot[row].u8DeviceType;
	memcpy(&cardid,&my_slotlist[column].allocslot[0].dest_addr[0],2);
	return type;
}

uint16 set_curslot_distance(double distance,uint8 curslot,uint8 type) 
{
	uint8 row=0,column=0;
//	uint16 addr;
	uint32 u32distance=0;
	u32distance = (uint32)(distance*100);
	row=curslot/TOF_SLOT_LOC_PERIOD;      //0-4
	column = curslot % TOF_SLOT_LOC_PERIOD; //0-39 
	if(type == 0)                //main station distance
		my_slotlist[column].allocslot[row].m_distance = u32distance;
	else if(type ==1)           //sub station distance
		my_slotlist[column].allocslot[row].s_distance = u32distance;
//	return addr;
	return 1;
}


//when the application start ,init the  station's slot list
void instance_init_slotlist(void)
{
	int i,j,temp,s,temp1;
	for(i=0;i<TOF_SLOT_LOC_PERIOD;i++)
	{
		my_slotlist[i].slotmode =L_IDLE;        //idle
		my_slotlist[i].num_of5s = 0;        
		for(j=0;j<MAX_CARD_CYCLE_SEC;j++)
		{
			temp = j*TOF_SLOT_LOC_PERIOD +i;          //0-199
			my_slotlist[i].allocslot[j].b1Used = IDLE;
			my_slotlist[i].allocslot[j].u8cur_slot = temp;
			my_slotlist[i].allocslot[j].u16SeqNum = 0;
			my_slotlist[i].allocslot[j].u8LostNum = 0;
			my_slotlist[i].allocslot[j].dest_addr[0] = my_slotlist[i].allocslot[j].dest_addr[1] =0;
			my_slotlist[i].allocslot[j].m_distance =0;
			my_slotlist[i].allocslot[j].s_distance =0;
			my_slotlist[i].allocslot[j].status =0;
		}
	}
	if(1)
	{	
		s=TOF_SLOT_LOC_PERIOD/2;
		for(i=0;i<TOF_SLOT_LOC_PERIOD;i++)
		{
			if((i+1)%s ==0)   //19,39,59,79,.....199
			{
				my_slotlist[i].slotmode =OTHER ;      //USED_UART ,every 500ms
				my_slotlist[i].num_of5s =MAX_CARD_CYCLE_SEC;
				for(j=0;j<MAX_CARD_CYCLE_SEC;j++)
				{
  
					temp = j*TOF_SLOT_LOC_PERIOD +i;          //0-199
					my_slotlist[i].allocslot[j].b1Used = USED_UART;
					my_slotlist[i].allocslot[j].u8cur_slot = temp;
				}
			}
			else
			{
				if((i-5)%10 ==0)//5,15.25,35,45......295
				{
					my_slotlist[i].slotmode =OTHER ;   //USED_BLINK     about every 6 slot have one
					my_slotlist[i].num_of5s =MAX_CARD_CYCLE_SEC;
					for(j=0;j<MAX_CARD_CYCLE_SEC;j++)
					{
  
						temp = j*TOF_SLOT_LOC_PERIOD +i;          //0-199
						my_slotlist[i].allocslot[j].b1Used = USED_BLINK;
						my_slotlist[i].allocslot[j].u8cur_slot = temp;
					}
				}
			}
		}
	}
	
}

uint8 instance_get_distancelist(uwb_tof_distance_ts *distance,uwb_tof_distance_ts *predistance, uint8 type)
{
	uint16 destaddr;
	memcpy(&destaddr,&my_staslotmsg.dest_addr[0],ADDR_BYTE_SIZE);	
	distance->i8Rssi = instance_data[0].i8rssi;
	if(instance_data[0].mode == TAG)
	{
		distance->u16ShortAddr= destaddr;
		distance->u16SeqNum = my_staslotmsg.u16SeqNum;
		distance->u8Status = my_staslotmsg.status;
		distance->u8DevType = my_staslotmsg.u8DeviceType;
		if(type ==1)  //the pre seqnum distance
		{
			distance->u16SeqNum = my_staslotmsg.u16SeqNum -1;
			distance->u32StationDistance =  pre_distance;
		}
		else
			distance->u32StationDistance =my_staslotmsg.m_distance ;
		distance->u32LocDistance = 0; 
		return 1;
	}
	else if(instance_data[0].mode == SUB_STA)
	{       // my_slotlist[i].allocslot[row].m_distance;
		if(destaddr == predistance->u16ShortAddr
			&& my_staslotmsg.u16SeqNum == predistance->u16SeqNum )
		{
			predistance->u16ShortAddr=destaddr ;	
			predistance->u16SeqNum = my_staslotmsg.u16SeqNum;
			predistance->u8Status = my_staslotmsg.status;
			predistance->u8DevType = my_staslotmsg.u8DeviceType;
			predistance->u32StationDistance =my_staslotmsg.m_distance ;
			predistance->u32LocDistance = my_staslotmsg.s_distance;        //my_slotlist[i].allocslot[row].s_distance;
			return 0;
		}
		else if(destaddr != predistance->u16ShortAddr || my_staslotmsg.u16SeqNum != predistance->u16SeqNum)
		{
			distance->u16ShortAddr =destaddr;
			distance->u16SeqNum = my_staslotmsg.u16SeqNum;
			distance->u8Status = my_staslotmsg.status;
			distance->u8DevType = my_staslotmsg.u8DeviceType;
			distance->u32StationDistance =my_staslotmsg.m_distance+500000 ;
			distance->u32LocDistance = my_staslotmsg.s_distance;
			return 1;
		}

	}
	return 1;
}

#ifdef DEC_UWB_SUB
uint8 instance_get_car_cardlist(uwb_tof_distance_ts *distance,Car_cardsmsg_t *car_card)
{
	uint16 destaddr;
	memcpy(&destaddr,&car_card->u8cardaddr[0],ADDR_BYTE_SIZE);	
	distance->u16ShortAddr =destaddr;
	distance->u16SeqNum = 0;
	distance->u8Status = car_card->status;
	distance->u8DevType = car_card->devtype;
	distance->u32StationDistance =Car_revcardlist.m_distance;
	distance->u32LocDistance = Car_revcardlist.s_distance;
	return 1;
}

//ts_Car_cardlist Car_revcardlist;
#endif

void instance_init_cardslot(uint8 cardtype)
{
	my_cardslotmsg.b1Used = IDLE;
	if(cardtype == 1)
	{
		my_cardslotmsg.u8DeviceType = CARD_1S;
		my_cardslotmsg.sleeptick = CARD_1S_SEC_TIME-50;
	}
	else if(cardtype ==5)
	{
		my_cardslotmsg.u8DeviceType = CARD_5S;
		my_cardslotmsg.sleeptick = CARD_5S_SEC_TIME-50;
	}
	my_cardslotmsg.u8cur_slot = 0;
	my_cardslotmsg.dest_addr[0] = my_cardslotmsg.dest_addr[1] =0;
	my_cardslotmsg.u16SeqNum = 0;
	my_cardslotmsg.u8LostNum =0;
	my_cardslotmsg.status =0;
	
	PrintfUtil_vPrintf("u8LostNum = %d b1Used = %d u8DeviceType = %d\n", my_cardslotmsg.u8LostNum, my_cardslotmsg.b1Used, my_cardslotmsg.u8DeviceType);
}

void instance_clear_substa(void)
{
	my_staslotmsg.u8cur_slot = 0;
	my_staslotmsg.dest_addr[0] = my_staslotmsg.dest_addr[1] =0;
	my_staslotmsg.u16SeqNum = 0;
	my_staslotmsg.u8LostNum =0;
	my_staslotmsg.status =0;
	my_staslotmsg.m_distance =0;
	my_staslotmsg.s_distance =0;
	instance_data[0].i8rssi = 0;
}

//check if the rev pre distance is valid, if it is valid then save and report it 
void instance_check_predistance(instance_data_t *inst,uint32 old_distance )
{
	uint8 devtype;
	uint32 my_old_dist=0;
	uint16 old_seq =0 ,seq=0 ,row =0;
	seq = inst->seqnum;
	devtype = my_slotlist[inst->curslot_column].allocslot[inst->curslot_row].u8DeviceType;
	if(devtype == CARD_1S)
	{
		row = ((uint16)inst->curslot_row + MAX_CARD_CYCLE_SEC -1) % MAX_CARD_CYCLE_SEC ; 
		my_old_dist = my_slotlist[inst->curslot_column].allocslot[row].m_distance;
		old_seq = my_slotlist[inst->curslot_column].allocslot[row].u16SeqNum;
		if((my_old_dist == 0&& seq!=0 && seq -old_seq ==1 ) || (seq -old_seq !=1))
		{
			my_slotlist[inst->curslot_column].allocslot[row].u16SeqNum = seq-1;
			my_slotlist[inst->curslot_column].allocslot[row].m_distance = old_distance;
			pre_distance = old_distance ;
			inst->oldrange = 1;
		}
	}
	else if(devtype == CARD_5S)
	{
		my_old_dist = my_slotlist[inst->curslot_column].allocslot[inst->curslot_row].m_distance;
		old_seq = my_slotlist[inst->curslot_column].allocslot[inst->curslot_row].u16SeqNum;
		if(( pre_dist_right==0 && seq!=0 && seq -old_seq ==1) || (seq -old_seq !=1)  )
		{
			pre_distance = old_distance ;
			inst->oldrange = 1;
		}
	}
	
}

void instance_check_discon(uint16 addr)   //check if the card was disconnect or not 
{
	uint16 destaddr=0;
	memcpy(&destaddr,&my_cardslotmsg.dest_addr[0],2);
	if(destaddr == addr)
	{
		instance_set_revpolltype(1);
		event_timer_set(EVENT_SLEEP_EVENT);
	}
	else
	{
		instance_set_revpolltype(0);
	}
}

void reset_sub_alarmmsg()
{
	sub_alarmmsg.alarmaddr = 0;
	sub_alarmmsg.alarmstatus =0;
	sub_alarmmsg.excitid =0;
}


double convertdevicetimetosec8(uint8* dt)
{
    double f = 0;

    uint32 lo = 0;
    int8 hi = 0;

    memcpy(&lo, dt, 4);
    hi = dt[4] ;

    f = ((hi * 65536.00 * 65536.00) + lo) * DWT_TIME_UNITS ;  // seconds #define TIME_UNITS          (1.0/499.2e6/128.0) = 15.65e-12

    return f ;
}


void rx_power_lever(instance_data_t *inst)
{
	double rx_lever=0;
	dwt_readdignostics(&inst->devicelogdata.diag);
	double CIR = (double)inst->devicelogdata.diag.maxGrowthCIR;
	double NPC = (double)inst->devicelogdata.diag.rxPreamCount;
	
	rx_lever = 10 * log10((CIR*131072)/(NPC*NPC))- 115.72  ;//121.74 -----64MHz  pow(2, 17)

	inst->i8rssi = (int8)rx_lever;
	//DBG(PrintfUtil_vPrintf("rssi= %i ",inst->i8rssi);)
}

int16 rx_power_lever1(instance_data_t *inst)
{
 	double PathAmp1=0,PathAmp2=0,PathAmp3=0,rx_lever=0;
 	int16 result=0;
	dwt_readdignostics(&inst->devicelogdata.diag);
	PathAmp1 = (double)inst->devicelogdata.diag.firstPathAmp1;
	PathAmp2 = (double)inst->devicelogdata.diag.firstPathAmp2;
	PathAmp3 = (double)inst->devicelogdata.diag.firstPathAmp3;
	double NPC = (double)inst->devicelogdata.diag.rxPreamCount;
	rx_lever = 10 * log10((PathAmp1*PathAmp1+PathAmp2*PathAmp2+PathAmp3*PathAmp3 )/(NPC*NPC))- 115.72  ;//121.74 -----64MHz

	result = (int16)rx_lever;
	//DBG(PrintfUtil_vPrintf("rssi= -%i  -%i  \n",result,result+115);)
	return result;
}

uint8 check_inblinkmsg(instance_data_t *inst,uint16 stationid)  
{
	//0: do in blink and goto sleep at once
	//1: do in bink  but do not goto sleep at once
	//2: don't in bink and don't go to sleep
	//3:goto sleep at once
	int8 i8rssi;
	rx_power_lever(inst);
	i8rssi = inst->i8rssi;
	new_inblinkmsg.count++;
	if(i8rssi >= -92 )//||  || new_inblinkmsg.status ==1
	{
		if(stationid == new_inblinkmsg.stationID)
		{
			return 3;
		}
		new_inblinkmsg.i8rssi = i8rssi;
		new_inblinkmsg.stationID = stationid;
		return 0;      //in blink and sleep
	}
	else //if(stationid != new_inblinkmsg.stationID)
	{
		if(new_inblinkmsg.count == 1 )   // the first time rev blink
		{
			new_inblinkmsg.i8rssi = i8rssi;
			new_inblinkmsg.stationID = stationid;
			return 1;      //in blink 
		}
		else
		{
			if(stationid == new_inblinkmsg.stationID)
			{
				if(i8rssi > new_inblinkmsg.i8rssi)
					new_inblinkmsg.i8rssi = i8rssi;
				if(new_inblinkmsg.status ==1)
					return 2;    //wait next blink msg
				else
					return 1;
			}
			else
			{
				if(i8rssi > new_inblinkmsg.i8rssi)
				{
					new_inblinkmsg.i8rssi = i8rssi;
					new_inblinkmsg.stationID = stationid;
					return 1;
				}
				else 
					return 2;
			}
		}
	}

}


void clear_inblinkmsg(void)
{
	new_inblinkmsg.stationID = 0;
	new_inblinkmsg.i8rssi = 0;
	new_inblinkmsg.count =0;
	new_inblinkmsg.status = 0;
	instance_data[0].new_inblink =0;
	inblink_status =0;
}
// -------------------------------------------------------------------------------------------------------------------
// convert microseconds to device time
uint64 convertmicrosectodevicetimeu (double microsecu)
{
    uint64 dt;
    long double dtime;

    dtime = (microsecu / (double) DWT_TIME_UNITS) / 1e6 ;

    dt =  (uint64) (dtime) ;

    return dt;
}

double convertdevicetimetosec(int64 dt)
{
    double f = 0;

    f =  dt * DWT_TIME_UNITS ;  // seconds #define TIME_UNITS          (1.0/499.2e6/128.0) = 15.65e-12

    return f ;
}

void reportTOF1(instance_data_t *inst)
{
    double distance ;
    double tof ;
    int64 tofi ;

    // check for negative results and accept them making them proper negative integers
    tofi = (int64) inst->m_tof ;                          // make it signed
    if (tofi > 0x007FFFFFFFFF)                          // MP counter is 40 bits,  close up TOF may be negative
    {
        tofi -= 0x010000000000 ;                       // subtract fill 40 bit range to mak it negative
    }

    // convert to seconds (as floating point)
    tof = convertdevicetimetosec(tofi) * 0.25;          //this is divided by 4 to get single time of flight
    distance = tof * SPEED_OF_LIGHT;

    if ((distance < -0.5) || (distance > 20000.000))    // discard any results less than <50 cm or >20km
        return;

#if (CORRECT_RANGE_BIAS == 1)
    distance = distance - dwt_getrangebias(inst->configData.chan, (float) distance, inst->configData.prf);
#endif

    distance = fabs(distance) ;                         // make any (small) negatives positive.

    inst_m_idist = distance;
	return;
}

void reportTOF(instance_data_t *inst)
{
    double distance ;
    double tof ;
    double ltave;
    int64 tofi ;
	
    // check for negative results and accept them making them proper negative integers
    tofi = (int64) inst->tof ;                          // make it signed
    if (tofi > 0x007FFFFFFFFF)                          // MP counter is 40 bits,  close up TOF may be negative
    {
        tofi -= 0x010000000000 ;                       // subtract fill 40 bit range to mak it negative
    }

    // convert to seconds (as floating point)
    tof = convertdevicetimetosec(tofi) * 0.25;          //this is divided by 4 to get single time of flight
    distance = tof * SPEED_OF_LIGHT;

    if ((distance < -0.5) || (distance > 20000.000))    // discard any results less than <50 cm or >20km
        return;

#if (CORRECT_RANGE_BIAS == 1)
    distance = distance - dwt_getrangebias(inst->configData.chan, (float) distance, inst->configData.prf);
#endif

    distance = fabs(distance) ;                         // make any (small) negatives positive.

    inst_idist = distance;
	
    inst->longTermRangeSum+= distance ;
    inst->longTermRangeCount++ ;                          // for computing a long term average
    ltave = inst->longTermRangeSum / inst->longTermRangeCount ;

    inst_ldist = ltave ;

    inst->adist[inst->tofindex++] = distance;
/*
    if(distance < inst->idistmin)
        inst->idistmin = distance;

    if(distance > inst->idistmax)
        inst->idistmax = distance;
*/
    if(inst->tofindex == RTD_MED_SZ) inst->tofindex = 0;

    if(inst->tofcount == RTD_MED_SZ)
    {
        int i;
        double avg;

        avg = 0;
        for(i = 0; i < inst->tofcount; i++)
        {
            avg += inst->adist[i];
        }
        avg /= inst->tofcount;

        inst_adist = avg ;

    }
    else
        inst->tofcount++;
return ;
}// end of reportTOF

// -------------------------------------------------------------------------------------------------------------------
//
// function to construct the message/frame header bytes
//
// -------------------------------------------------------------------------------------------------------------------
//
void instanceconfigframeheader(instance_data_t *inst, int ackrequest)
{
	if(inst->mode == TAG)   
	{
		if(inst->testAppState ==TA_TXBLINK_WAIT_SEND ||
			inst->testAppState ==TA_TX_RANGING_ACK_SEND)
		{
			inst->msg.panID[0] = 0xdd;
			inst->msg.panID[1] = 0xdd;
		}
		else
		{
			inst->msg.panID[0] = 0xcc;
			inst->msg.panID[1] = 0xcc;
		}
	}
	else if(inst->mode == ANCHOR) 
	{
		if(inst->testAppState == TA_TXPOLL_WAIT_SEND || 
			inst->testAppState == TA_TXFINAL_WAIT_SEND||
			inst->testAppState == TA_HELP_CALL_SEND ||
			inst->testAppState == TA_EXCIT_WAIT_SEND ||
			inst->testAppState == TA_RETREAT_ACK_SEND ||
			inst->testAppState == TA_TDOA_WAIT_SEND )
			
		{
			inst->msg.panID[0] = 0xee; //sub station
			inst->msg.panID[1] = 0xee;
		}
		else
		{	
			inst->msg.panID[0] = 0xca;  //main station
			inst->msg.panID[1] = 0xde;
			
		}
	}
	else if(inst->mode == SUB_STA)
	{
		inst->msg.panID[0] = 0xcc;
		inst->msg.panID[1] = 0xcc;
	}

    //set frame type (0-2), SEC (3), Pending (4), ACK (5), PanIDcomp(6)
    inst->msg.frameCtrl[0] = 0x1 /*frame type 0x1 == data*/ | 0x40 /*PID comp*/;
    inst->msg.frameCtrl[0] |= (ackrequest ? 0x20 : 0x00);
#if (USING_64BIT_ADDR==1)
    //source/dest addressing modes and frame version
    inst->msg.frameCtrl[1] = 0xC /*dest extended address (64bits)*/ | 0xC0 /*src extended address (64bits)*/;
#else
    inst->msg.frameCtrl[1] =0x8 /*dest short address (16bits)*/ | 0x80 /*src short address (16bits)*/;
#endif

    inst->msg.seqNum = inst->cur_slot;	
		
	if(inst->mode == TAG) //anchor blink pro  
	{
		memcpy(&inst->msg.destAddr,&test_tof_an_addr,ADDR_BYTE_SIZE);  //blink process :0xffff ;
#if(POLL_HAVE_BLINK_MSG ==1)
		inst->msg.destAddr[0] = 0xFF;
		inst->msg.destAddr[1] = 0xFF;
#endif
	}
	else if(inst->mode == ANCHOR ||inst->mode == SUB_STA)
	{
		memcpy(&inst->msg.destAddr,&inst->rev_shortaddr16,ADDR_BYTE_SIZE);
		if(inst->mode == ANCHOR && 
			(inst->testAppState == TA_HELP_CALL_SEND
			||inst->testAppState == TA_EXCIT_WAIT_SEND 
			||inst->testAppState == TA_TDOA_WAIT_SEND 
			||inst->testAppState == TA_RETREAT_ACK_SEND))    //help ask
		{
			inst->msg.destAddr[0] = 0xFF;
			inst->msg.destAddr[1] = 0xFF;
		}
	}
	memcpy(&inst->msg.sourceAddr,&u16ShortAddr,ADDR_BYTE_SIZE);
}

// -------------------------------------------------------------------------------------------------------------------
//
// function to select the destination address (e.g. the address of the next anchor to poll)
//
// -------------------------------------------------------------------------------------------------------------------
//

void instsettagtorangewith(int tagID)
{
	int instance = 0 ;

	instance_data[instance].tagToRangeWith = tagID ;
	//instance_data[instance].tagList[instance_data[instance].tagToRangeWith] = tagID ;
}


// -------------------------------------------------------------------------------------------------------------------
//
// function to configure the mac frame data, prior to issuing the PD_DATA_REQUEST
//
// -------------------------------------------------------------------------------------------------------------------
//
void setupmacframedata(instance_data_t *inst, int len, int fcode, int ack)
{
    inst->macdata_msdu[FCODE] = fcode; //message function code (specifies if message is a poll, response or other...)

    if(len)
        memcpy(inst->msg.messageData, inst->macdata_msdu, len); //copy application data

    inst->psduLength = len + FRAME_CRTL_AND_ADDRESS + FRAME_CRC;
//	 inst->psduLength = len + 9 + FRAME_CRC;

    //inst->psduLength = adduserpayload(inst, inst->psduLength, len); //add any user data to the message payload

    instanceconfigframeheader(inst, ack); //set up frame header (with/without ack request)

    if(ack == ACK_REQUESTED)
        inst->wait4ack = DWT_RESPONSE_EXPECTED;

    inst->ackexpected = ack ; //used to ignore unexpected ACK frames
}

// -------------------------------------------------------------------------------------------------------------------
//
// Turn on the receiver with/without delay
//
void instancerxon(int delayed, uint64 delayedReceiveTime)
{
    if (delayed)
    {
        uint32 dtime;
        dtime =  (uint32) (delayedReceiveTime>>8);
        dwt_setdelayedtrxtime(dtime) ;
    }

#if (SNIFF_MODE == 1)
    dwt_setrxmode(RX_SNIFF, 0, 0x02, 0xFF); //Off time 0xFF, on time 0x2  DWT_RX_NORMAL
#endif
	if(instance_data[0].mode == SUB_STA)
		dwt_setrxmode(DWT_RX_NORMAL, 0, 0xFF); //Off time 0xFF, on time 0x2  DWT_RX_NORMAL
    dwt_rxenable(delayed) ;               // turn receiver on, immediate/delayed

} // end instancerxon()


int instancesendpacket(instance_data_t *inst, int delayedTx)
{
    int result = 0;

    dwt_writetxdata(inst->psduLength, (uint8 *)  &inst->msg, 0) ;   // write the frame data
    dwt_writetxfctrl(inst->psduLength, 0);
    if(delayedTx)
    {
        uint32 dtime;
        dtime = (uint32) (inst->delayedReplyTime>>8);
        dwt_setdelayedtrxtime(dtime) ;
    }

    if(inst->wait4ack)
    {
        //if the ACK is requested there is a 5ms timeout to stop RX if no ACK coming
        dwt_setrxtimeout(5000);  //units are us - wait for 5ms after RX on
    }

    //begin delayed TX of frame
    if (dwt_starttx(delayedTx | inst->wait4ack))  // delayed start was too late
    {
        result = 1; //late/error
    }


    return result;                                              // state changes
    // after sending we should return to TX ON STATE ?
}



int powertest(void)
{
    dwt_config_t    configData ;
    dwt_txconfig_t  configTx ;
    uint8 msg[127] = "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the l";


    SPI_ConfigFastRate(SPI_BaudRatePrescaler_16); //reduce the SPI speed before putting device into low power mode
    //
    //  reset device
    //
    dwt_softreset();

    //
    //  configure channel paramters
    //
    configData.chan = 2 ;
    configData.rxCode =  9 ;
    configData.txCode = 9 ;
    configData.prf = DWT_PRF_64M ;
    configData.dataRate = DWT_BR_110K ;
    configData.txPreambLength = DWT_PLEN_2048 ;
    configData.rxPAC = DWT_PAC64 ;
    configData.nsSFD = 1 ;
    configData.smartPowerEn = 0;

    dwt_configure(&configData, DWT_LOADANTDLY | DWT_LOADXTALTRIM) ;

    configTx.PGdly = txSpectrumConfig[configData.chan].PGdelay ;
    configTx.power = txSpectrumConfig[configData.chan].txPwr[configData.prf - DWT_PRF_16M];

    dwt_configuretxrf(&configTx);

    // the value here 0x1000 gives a period of 32.82 祍
    //this is setting 0x1000 as frame period (125MHz clock cycles) (time from Tx en - to next - Tx en)
    dwt_configcontinuousframemode(0x1000);

    dwt_writetxdata(127, (uint8 *)  msg, 0) ;
    dwt_writetxfctrl(127, 0);

    //to start the first frame - set TXSTRT
    dwt_starttx(DWT_START_TX_IMMEDIATE);

    //measure the power
    //Spectrum Analyser set:
    //FREQ to be channel default e.g. 3.9936 GHz for channel 2
    //SPAN to 1GHz
    //SWEEP TIME 1s
    //RBW and VBW 1MHz
    //measure channel power

    return DWT_SUCCESS ;
}


void xtalcalibration(void)
{
    int i;
    uint8 chan = 2 ;
    uint8 prf = DWT_PRF_16M ;
    dwt_txconfig_t  configTx ;

    SPI_ConfigFastRate(SPI_BaudRatePrescaler_16); //reduce the SPI speed before putting device into low power mode
    //
    //  reset device
    //
    dwt_softreset();

    //
    //  configure TX channel parameters
    //

    configTx.PGdly = txSpectrumConfig[chan].PGdelay ;
    configTx.power = txSpectrumConfig[chan].txPwr[prf - DWT_PRF_16M];

    dwt_configuretxrf(&configTx);

    dwt_configcwmode(chan);

    for(i=0; i<=0x1F; i++)
    {
        dwt_xtaltrim(i);
        //measure the frequency
        //Spectrum Analyser set:
        //FREQ to be channel default e.g. 3.9936 GHz for channel 2
        //SPAN to 10MHz
        //PEAK SEARCH
    }

    return;
}

// -------------------------------------------------------------------------------------------------------------------
//
// the main instance state machine (all the instance modes Tag, Anchor or Listener use the same statemachine....)
//
// -------------------------------------------------------------------------------------------------------------------
//
/*
TA_TXPOLL_WAIT_SEND,		//2
TA_TXFINAL_WAIT_SEND,		//3
TA_TXRESPONSE_WAIT_SEND,	//4
TA_TXREPORT_WAIT_SEND,		//5
TA_TX_WAIT_CONF,			//6

TA_RXE_WAIT,				//7
TA_RX_WAIT_DATA,			//8

TA_SLEEP,					//9
TA_SLEEP_DONE,				//10
TA_TXBLINK_WAIT_SEND,		//11
TA_TXRANGINGINIT_WAIT_SEND,  //12
TA_TX_RANGING_ACK_SEND		//13
TA_HELP_CALL_SEND , 		  //14
 TA_HELP_RESP_SEND ,		   //15
 TA_EVACUATE_ASK_SEND		  //16

*/
void txtdoa_wait_send()
{
	int send_len=0;
	uint16 version = OAD_UWB_CARD_VERSION;
	dwt_forcetrxoff() ;
	instance_data_t *inst  =(instance_data_t*) (&instance_data[0]);
	inst->macdata_msdu[1] = 0x0;     //SLOW_Speed
	
	inst->macdata_msdu[2] = my_cardslotmsg.status;
	inst->macdata_msdu[3] = my_cardslotmsg.u8DeviceType;   ///1s or 5s 
	
	inst->macdata_msdu[4] = (my_cardslotmsg.u16SeqNum >> 8)&0xff;
	inst->macdata_msdu[5] = (my_cardslotmsg.u16SeqNum)&0xff;
	if(inst->tdoarepvbat)
	{
		memcpy(&inst->macdata_msdu[6],&inst->cardbattery,2);
		memcpy(&inst->macdata_msdu[8],&version,2);
		inst->tdoarepvbat =0;
		send_len = 10;
	}
	else if(!inst->tdoarepvbat)
	{
		inst->cardbattery =0xFFFF;
		memcpy(&inst->macdata_msdu[6],&inst->cardbattery,2);
		send_len = 8;
	}

	inst->testAppState = TA_TDOA_WAIT_SEND;
	setupmacframedata(inst, send_len, RTLS_TDOA_BLINK_SEND, !ACK_REQUESTED);
	inst->previousState = TA_TDOA_WAIT_SEND ;
	if(instancesendpacket(inst, DWT_START_TX_IMMEDIATE))
	{
		 inst->done = INST_NOT_DONE_YET;

	}
	else
	{
		PrintfUtil_vPrintf("tdoa : u16SeqNum=%d!\n",my_cardslotmsg.u16SeqNum);
		mSleep(2); //sleep or printf ,if don't do this  ,will possible not send seccuss 
		//inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set below)
		dwt_setrxtimeout(0); 
	}
}
void tdoa_send()
{
	if(my_cardslotmsg.b1Used == IDLE)
	{
		instance_change_channel(ANCHOR_BLINK_CHANNEL);//ANCHOR_TOF_CHANNEL
		txtdoa_wait_send();
	//	instance_change_channel(ANCHOR_BLINK_CHANNEL);
	}
	
}

void txretreat_ack_send()//uint8 *help_payload,int len)
{
	uint8 newHelpAskSent=0; 		  //if help fail ,send again
	instance_data_t *inst  =(instance_data_t*) (&instance_data[0]);
	dwt_forcetrxoff() ;
	
	inst->testAppState =TA_RETREAT_ACK_SEND ;
	inst->macdata_msdu[1]=  my_cardslotmsg.status;
	setupmacframedata(inst, 2, RTLS_RETREAT_ACK_SEND, !ACK_REQUESTED);

	inst->previousState = TA_RETREAT_ACK_SEND ;
	if(instancesendpacket(inst, DWT_START_TX_IMMEDIATE))
	{
		EDBG(PrintfUtil_vPrintf("retreat ack fail! \n");)
		newHelpAskSent++;
		if(newHelpAskSent <3)
		{
			mSleep(3);
		}
	}
	else
	{
		DBG(PrintfUtil_vPrintf("retreat ack seccuss! \n");)
		inst->testAppState = TA_TX_WAIT_CONF ;
	//	dwt_setrxtimeout((RX_FWTO_TIME+2) * 1000);  //units are us - wait for 5ms after RX on
     	dwt_setrxtimeout(0); 

	}

}

void txhelp_call_send()//uint8 *help_payload,int len)
{
	uint8 newHelpAskSent=0; 		  //if help fail ,send again
	instance_data_t *inst  =(instance_data_t*) (&instance_data[0]);
HELPAGAIN:
	dwt_forcetrxoff() ;
//	memcpy(&inst->macdata_msdu[1],help_payload,len);
	
	inst->testAppState =TA_HELP_CALL_SEND ;
	inst->macdata_msdu[1]=  my_cardslotmsg.status;
	setupmacframedata(inst, 2, RTLS_MSG_HELP_CALL, !ACK_REQUESTED);

	inst->instToSleep=0;
	inst->previousState = TA_HELP_CALL_SEND ;
	if(instancesendpacket(inst, DWT_START_TX_IMMEDIATE))
	{
		EDBG(PrintfUtil_vPrintf("ask help fail! \n");)
		newHelpAskSent++;
		if(newHelpAskSent <3)
		{
			mSleep(3);
			goto HELPAGAIN;
		}
	}
	else
	{
		DBG(PrintfUtil_vPrintf("ask help seccuss! \n");)
		newHelpAskSent=0;
		inst->testAppState = TA_TX_WAIT_CONF ;
		inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;  //no timeout
	//	dwt_setrxtimeout((RX_FWTO_TIME+2) * 1000);  //units are us - wait for 5ms after RX on
     	dwt_setrxtimeout(0); 

	}

}

void txhelp_resp_send(instance_data_t *inst)
{
	uint8 newHelprespSent=0;
HELPRESPAGAIN:
	dwt_forcetrxoff() ;
	inst->macdata_msdu[1]= sub_alarmmsg.alarmstatus;  ///1:rev but the service counter haven't report ;  2:the service counter have report
	inst->testAppState =TA_HELP_RESP_SEND ;
	setupmacframedata(inst, 2, RTLS_MSG_HELP_RESP, !ACK_REQUESTED);

	inst->instToSleep=0;
	inst->previousState = TA_HELP_RESP_SEND ;
	mSleep(1);
	if(instancesendpacket(inst, DWT_START_TX_IMMEDIATE))
	{
		EDBG(PrintfUtil_vPrintf("help resp fail! \n");)
		newHelprespSent++;
		if(newHelprespSent <3)
		{
			mSleep(3);
			goto HELPRESPAGAIN;
		}
	}
	else
	{
		DBG(PrintfUtil_vPrintf("help resp seccuss! \n");)
		newHelprespSent=0;
     	dwt_setrxtimeout(0); 

	}

}



void ta_rxe_wait(instance_data_t *inst)
{

    if(inst->wait4ack == 0) //if this is set the RX will turn on automatically after TX
    {
        uint64 delayedReceiveTime = 0;

       if (inst->shouldDoDelayedRx) //we don't need to turn receiver on immediately, as we know the response will come in a while
        {
            delayedReceiveTime = (inst->txu.txTimeStamp + inst->rxOnDelay) & MASK_40BIT;

            if(inst->previousState == TA_TXBLINK_WAIT_SEND) //check if we need to use long response delay for the blink response
            {
            	if(inst->fixedReplyDelay_ms > FIXED_LONG_REPLY_DELAY)
            	{
            		delayedReceiveTime = (delayedReceiveTime + (DELAY_MULTIPLE*inst->fixedReplyDelay)) & MASK_40BIT;
            	}
            }
        }

        //turn RX on
  //      instancerxon(inst->shouldDoDelayedRx, delayedReceiveTime) ;   // turn RX on, with/without delay
        instancerxon(0,0);
    }
    else
    {
        inst->wait4ack = 0 ; //clear the flag, the next time we want to turn the RX on it might not be auto
    }

    inst->shouldDoDelayedRx = FALSE ; //clear the flag

    if (inst->mode != LISTENER)
    {
        if (inst->previousState != TA_TXREPORT_WAIT_SEND) //we are going to use anchor timeout and re-send the report
            inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //using RX FWTO
    }

    inst->testAppState = TA_RX_WAIT_DATA;   // let this state handle it
    dwt_setrxtimeout(0); 

    // end case TA_RXE_WAIT, don't break, but fall through into the TA_RX_WAIT_DATA state to process it immediately.
//    if(message == 0) break;
}

void txexcit_wait_send(uint16 excitid)
{
	instance_data_t *inst  =(instance_data_t*) (&instance_data[0]);
	dwt_forcetrxoff() ;
	inst->macdata_msdu[1] = my_cardslotmsg.status;
	inst->macdata_msdu[2] =(uint8)excitid;
	inst->testAppState = TA_EXCIT_WAIT_SEND ;
	setupmacframedata(inst, 3, RTLS_EXCIT_ASK_SEND, !ACK_REQUESTED); //RTLS_MSG_HELP_CALL  RTLS_EXCIT_ASK_SEND
	inst->instToSleep=0;
	inst->previousState = TA_EXCIT_WAIT_SEND ;
	if(instancesendpacket(inst, DWT_START_TX_IMMEDIATE))
	{
		DBG(PrintfUtil_vPrintf("excit send fail ! \n");)
		ta_rxe_wait(inst);
	}
	else
	{
		EDBG(PrintfUtil_vPrintf("excit send seccuss !\n");)
		inst->testAppState = TA_TX_WAIT_CONF;	// wait confirmation
	//	dwt_setrxtimeout((RX_FWTO_TIME) * 1000);  //units are us - wait for 5ms after RX on
     	dwt_setrxtimeout(0); 

	}
}

void txexcit_ack_send(void)
{
	instance_data_t *inst  =(instance_data_t*) (&instance_data[0]);
	dwt_forcetrxoff() ;
	inst->macdata_msdu[1] = my_cardslotmsg.status;
	setupmacframedata(inst, 1, RTLS_EXCIT_ACK_SEND, !ACK_REQUESTED);
	inst->instToSleep=0;
	inst->previousState = TA_EXCIT_ACK_SEND ;
	if(instancesendpacket(inst, DWT_START_TX_IMMEDIATE))
	{
		EDBG(PrintfUtil_vPrintf("txexcit_ack_send fail ");)
		ta_rxe_wait(inst);
	}
	else
	{
		inst->testAppState = TA_TX_WAIT_CONF;	// wait confirmation
		dwt_setrxtimeout((RX_FWTO_TIME) * 1000);  //units are us - wait for 5ms after RX on
   //  dwt_setrxtimeout(0); 

	}
}

void TdoaStationTest1(void)
{
	PrintfUtil_vPrintf("\n ************** board init TdoaStationTest1*************\n");
}
void TdoaStationTest2(void)
{
	PrintfUtil_vPrintf("\n ************** board init TdoaStationTest2*************\n");
}

#if 1

void TdoaSendTagPollInTof()
{
	instance_data_t* pstInstMsg = TdoaGetOldLocalInstStructurePtr();

	//根据设备速率类型配置发送缓冲区
	if (pstInstMsg->u8TxSpeedType == INST_MODE_TX_SPEED_SLOW)
	{
		pstInstMsg->macdata_msdu[TDOA_INST_FRAME_SPEED_TYPE_BIT] = TDOA_INST_MODE_SPEED_SLOW;
	}
	else if (pstInstMsg->u8TxSpeedType == INST_MODE_TX_SPEED_QUICK)
	{
		pstInstMsg->macdata_msdu[TDOA_INST_FRAME_SPEED_TYPE_BIT] = TDOA_INST_MODE_SPEED_QUCIK;
	}

	//配置消息的序列号
	pstInstMsg->seqnum ++;
	pstInstMsg->macdata_msdu[TDOA_INST_FRAME_SEQNUM_BIT_H] = (pstInstMsg->seqnum >> 8) & 0xff;
	pstInstMsg->macdata_msdu[TDOA_INST_FRAME_SEQNUM_BIT_L] = (pstInstMsg->seqnum) & 0xff;
	pstInstMsg->msg.seqNum = (uint8)pstInstMsg->seqnum;
		
	//配置实例发送的目的地址 此函数只有待测卡和快发卡会触发 目的地址可以设置为广播全f 
	pstInstMsg->u16shortaddr16dest = TDOA_INST_SEND_POLL_DEST_ADDR;
	//pstInstMsg->stTxMsgToPack.u8DestAddr = TDOA_INST_SEND_POLL_DEST_ADDR;

	//向设备mac层设置发送帧内容及类型 与控制寄存器的ctrl[1]对应
	setupmacframedata(pstInstMsg, 6, RTLS_TDOA_MSG_CARD_POLL, !TDOA_INST_ACK_REQUESTED);

	//进行消息发送
	if(instancesendpacket(pstInstMsg, DWT_START_TX_IMMEDIATE))
	{
		//若发送失败，则触发轮询发送事件，外部进行周期轮询此处不需要再次设置

		//当前序列号进行减一操作
		pstInstMsg->seqnum --;

	}
	
	PrintfUtil_vPrintf("shortaddr16 = %d CardSendCount  = %d QuickCardSendCount = %d seqNum = %d\n", 
		pstInstMsg->shortaddr16, u8TdoaCardSendCount, u16TdoaQuickCardSendCount, pstInstMsg->seqnum);
	//发送成功进行闪灯提示
	dwt_setleds(2);

}
#endif

void txblink_wait_send(instance_data_t *inst)
{
	uint8 temp,len=0;
	uint8 cur_list=0,cur_row=0;
	uint8 slot = inst->cur_slot;
	
	cur_list = slot % TOF_SLOT_LOC_PERIOD ;
	cur_row= slot/TOF_SLOT_LOC_PERIOD;      //0-4
	dwt_forcetrxoff() ;
//	if(inst->cur_slot_msg !=USED_TOF)   //blink after tof process
//		inst->cur_slot_msg =IDLE ;

	temp =abs( portGetTickCount() -instance_get_slot_starttick());  //+2 send process
	new_2blink_tick = temp;
	
	//1:Slot msg** 2:Slot_have_run_tick**3:Next_1s_idle_slot**4:Next_5s_idle_slot
	if(EVERY_SLOT_TIME -temp <=10)  ///0:all card ; 1:1s card  ; 2:5s card  3:only blink mode
	{
		inst->macdata_msdu[1]= BLINK_NO_RESP;      //Slot msg
	//	inst->only_blink_slot =1;                     //not need respose ,when send over go rev or out
		if(EVERY_SLOT_TIME -temp  <= 3){ 
			dwt_setrxtimeout(0);
			return;
		}
	}
	else
	{
	//	inst->only_blink_slot =0;      //need  response
		inst->macdata_msdu[1]= BLINK_ALL_RESP; 
		if(my_slotlist[cur_list].slotmode == CARD_5S)
			inst->macdata_msdu[1]=BLINK_5S_RESP; 
		else if(my_slotlist[cur_list].slotmode == OTHER)
		{
			if(my_slotlist[cur_list].allocslot[cur_row].b1Used == USED_BLINK)
				inst->macdata_msdu[1]=BLINK_ALL_RESP;
			else
				inst->macdata_msdu[1]=BLINK_5S_RESP; 
		}
	}

	inst->macdata_msdu[2]= (uint8)temp;     ///2 Slot_have_run_tick

	inst->macdata_msdu[3]= u16GetIdleSlot_TOF1s(&inst->next_idle_1sslot,slot);  ///3: 1s card idle slot
	inst->macdata_msdu[4]= u16GetIdleSlot_TOF5s(&inst->next_idle_5sslot,slot);  ///4: 5s card idle slot

	len = instance_inset_alarmmsg(inst,5);  //inset alarm massage
	test_tof_an_addr = 0xffff;

	setupmacframedata(inst, len+1, SIG_RX_BLINK, !ACK_REQUESTED);

	inst->instToSleep=0;
	inst->previousState = TA_TXBLINK_WAIT_SEND ;        //this will blink alway if rev resp fail
	if(instancesendpacket(inst, DWT_START_TX_IMMEDIATE))
	{
		 //error - TX FAILED
		 //inst->txu.txTimeStamp = 0;
		 EDBG(PrintfUtil_vPrintf("sN ");)
		 inst->testAppState = TA_TXBLINK_WAIT_SEND ;  // wait to receive a new poll
		 inst->done = INST_NOT_DONE_YET;

	}
	else
	{
		inst->testAppState = TA_TX_WAIT_CONF;//TA_RXE_WAIT;//TA_TX_WAIT_CONF ;	
		inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set below)

		dwt_setrxtimeout((RX_FWTO_TIME+3) * 1000);  //units are us - wait for 5ms after RX on
	}

}

void blink_in_slotend(instance_data_t *inst)
{
	instance_change_channel(ANCHOR_BLINK_CHANNEL);
	txblink_wait_send(inst);
}

void txranginginit_wait_send(instance_data_t *inst)
{

	//set destination address
	//memcpy(&inst->msg.destAddr, &inst->tagList[inst->tagToRangeWith], ADDR_BYTE_SIZE);

	RngInitTick =  portGetTickCount();
	 
	inst->macdata_msdu[1] = my_cardslotmsg.status;              //status
	inst->macdata_msdu[2] = my_cardslotmsg.u8DeviceType;                  //1s or 5s
	setupmacframedata(inst, 3, RTLS_DEMO_MSG_RNG_INIT, !ACK_REQUESTED);

	inst->testAppState = TA_TX_WAIT_CONF;												// wait confirmation
	inst->previousState = TA_TXRANGINGINIT_WAIT_SEND ;
//	temp = portGetTickCount()%2;
//	mSleep(temp);
	//if(instancesendpacket(inst, DWT_START_TX_IMMEDIATE))
	if(instancesendpacket(inst, DWT_START_TX_IMMEDIATE))
	{
		EDBG(PrintfUtil_vPrintf("send fail ");)
		inst->txu.txTimeStamp = 0;
		inst->shouldDoDelayedRx = FALSE ;	// no delay in turning on RX
		//inst->testAppState = TA_RXE_WAIT ;	// wait to receive a new blink or poll message
		ta_rxe_wait(inst);
	}
	else
	{
		PrintfUtil_vPrintf("send init  --rssi =%i  status =%d \n",inst->i8rssi,inblink_status);
		inst->testAppState = TA_TX_WAIT_CONF ;												 // wait confirmation
		inst->previousState = TA_TXRANGINGINIT_WAIT_SEND ;
		inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;  //no timeout
	//	dwt_setrxtimeout(0);
		dwt_setrxtimeout((RX_FWTO_TIME) * 1000);
	}
}

void txranging_ack_wait_send(instance_data_t *inst,uint8 allotslot)//,new_slot_msg_t *slotmsg)
{
	int send_len=0;
	dwt_forcetrxoff() ;
//	runtime =abs( portGetTickCount() -instance_get_slot_starttick()); //portGetTickCount()% EVERY_SLOT_TIME;
	
	inst->macdata_msdu[1] = (uint8)TOF_SLOT_LOC_PERIOD;
	inst->macdata_msdu[2] = (uint8)new_2blink_tick;

	memcpy(&inst->macdata_msdu[allot_slot_1],&inst->relpyAddress,ADDR_BYTE_SIZE);
//	temp = check_slot_list(test_tof_an_addr,inst->rev_card_type);
	if(allotslot ==255)
	{
		//if(4== instance_get_listslotmsg(inst->cur_slot,0)||inst->cur_slot_msg == USED_TOF)
		{ //don't allot itself
			if(inst->rev_card_type == CARD_5S)
				inst->macdata_msdu[5] = inst->next_idle_5sslot;
			else if(inst->rev_card_type == CARD_1S)
				inst->macdata_msdu[5] = inst->next_idle_1sslot;
		}
		//else
		//	inst->macdata_msdu[5] = inst->cur_slot;
	}
	else
		inst->macdata_msdu[5] = allotslot;
	
	send_len=6;
	
	setupmacframedata(inst, send_len, RTLS_TOF_MSG_TAG_ACK, !ACK_REQUESTED);

	inst->instToSleep=0;
	inst->previousState = TA_TX_RANGING_ACK_SEND ;
	if(instancesendpacket(inst,DWT_START_TX_IMMEDIATE))  // DWT_START_TX_DELAYED
	{
		 //error - TX FAILED
		 inst->testAppState = TA_TX_RANGING_ACK_SEND ;  // wait to receive a new poll
		 inst->done = INST_NOT_DONE_YET;

	}
	else
	{
		DBG(PrintfUtil_vPrintf("send Ack  %d |%d |%d ",inst->macdata_msdu[5],inst->cur_slot,allotslot );)
		inst->testAppState = TA_TX_WAIT_CONF ;                                               // wait confirmation
		inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set below)
	//	slotmsg->is_Idle = 1;
		dwt_setrxtimeout((RX_FWTO_TIME) * 1000);  //units are us - wait for 5ms after RX on
	//	dwt_setrxtimeout(0);

	}
}	

void txpoll_wait_send(instance_data_t *inst)
{
	int pollsendlen=0,seqnum=0,runtime=0;
	dwt_forcetrxoff() ;
	LastPollTick = portGetTickCount();
	runtime =abs(LastPollTick -instance_get_slot_starttick());
	
	if(inst->mode == TAG)
	{
		if(runtime >20) //have no enaugh time
		{
			if(1 == inst->change_devtype)
			{
				instance_set_cardidle(inst->curslot_row,inst->curslot_column);
				inst->change_devtype= 0;
			}
			blink_in_slotend(inst);
			return;
		}
		isnewblinkon = 0;
		if(slotfirstinpoll ==1){      //first time,get the seqnum, same slot but send again the seqnum don't plus 1
			test_tof_an_addr = get_curslot_destaddr(inst->cur_slot);
			seqnum = instance_get_curseq(inst->cur_slot);
			inst->cur_slot_seq = seqnum;
			slotfirstinpoll =0;
		}
		else
			seqnum = inst->cur_slot_seq;
	//	memcpy(&test_tof_an_addr,&slotmsg->tag_addr,ADDR_BYTE_SIZE);
	}
	else if(inst->mode == ANCHOR)
	{
		seqnum = my_cardslotmsg.u16SeqNum;
	/*	if(CARD_WAKE_UP_MAX-runtime  <= 11){  //EVERY_SLOT_TIME+10 -runtime  <= 11
			dwt_setrxtimeout(0);
			return;
		}*/
	}
	
	inst->macdata_msdu[POLL_VOLT+1] =(seqnum>>8)&0xff ;//(inst->seqnum >> 8)&0xff;
	inst->macdata_msdu[POLL_VOLT+2] = (seqnum)&0xff ;//(inst->seqnum)&0xff;
		
	if(inst->mode == TAG)
	{
		
#if (DR_DISCOVERY == 1)
		//NOTE the anchor address is set after receiving the ranging initialisation message
		inst->instToSleep = 1; //we'll sleep after this poll

#endif

		inst->macdata_msdu[POLL_TEMP] = instance_get_Alarmstatus(inst->cur_slot,inst->mode) ;  ///1
		inst->macdata_msdu[POLL_VOLT] = (uint8)runtime;                                        ///2
		
		//inst->macdata_msdu[POLL_VOLT+3] = inst->station_status;
		instance_check_chgdevtype(test_tof_an_addr,inst->cur_slot,&inst->macdata_msdu[POLL_VOLT+3]);
		pollsendlen = 6;

#if(POLL_HAVE_BLINK_MSG ==1)
		uint8 pu8Index=255;
		u16GetIdleSlot_TOF1s(&pu8Index,inst->cur_slot);
			inst->macdata_msdu[POLL_VOLT+4]= pu8Index;  ///3: 1s card idle slot
		if(!u16GetIdleSlot_TOF5s(&pu8Index,inst->cur_slot))
			inst->slotlist_full = TRUE;
		inst->macdata_msdu[POLL_VOLT+5]= pu8Index;  ///4: 5s card idle slot
		pollsendlen +=2;	

#endif
	}
	else if(inst->mode == ANCHOR)
	{
#if (MAIN_AND_SUB_RANGING ==1)
		//cur_range = (uint32)(inst_idist *100) &0xFFFF  ;
		inst->macdata_msdu[POLL_TEMP] = my_cardslotmsg.status;
		inst->macdata_msdu[POLL_VOLT] = my_cardslotmsg.u8DeviceType; 
		memcpy(&(inst->macdata_msdu[POLL_VOLT+3]),&my_cardslotmsg.m_distance, sizeof(uint32));
		pollsendlen =TAG_POLL_MSG_LEN+1;
	
#endif
	}
	inst->testAppState = TA_TXPOLL_WAIT_SEND;
	setupmacframedata(inst,pollsendlen, RTLS_DEMO_MSG_TAG_POLL, !ACK_REQUESTED);
		
	inst->previousState = TA_TXPOLL_WAIT_SEND ;

	if(instancesendpacket(inst, DWT_START_TX_IMMEDIATE))
	{
		 //error - TX FAILED
		 EDBG(PrintfUtil_vPrintf(" poll fail ");)
		 inst->testAppState = TA_TXPOLL_WAIT_SEND ;  // wait to receive a new poll
		 inst->done = INST_NOT_DONE_YET;

	}
	else
	{
		card_last_dist_msgtype =1;
		inst->testAppState = TA_TX_WAIT_CONF ;												 // wait confirmation
		
		inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set below)
	//	dwt_enableframefilter(DWT_FF_NOTYPE_EN); //disable frame filtering
	//	DBG(PrintfUtil_vPrintf("\n seq= %d |%d ",seqnum,test_tof_an_addr);)
		dwt_setrxtimeout((RX_FWTO_TIME) * 1000);	//units are us - wait for 5ms after RX on
		
	}

}

void card_send_poll2sub(instance_data_t *inst)
{
	PrintfUtil_vPrintf("m_distance =%d	new_tof_in = %d	\n", my_cardslotmsg.m_distance, new_tof_in);

	inst->rev_shortaddr16 += 10000;
	txpoll_wait_send(inst);
	if(new_tof_in >0)
		new_tof_in --;

}

void TofInstanceCardDataInit()
{
	memset(&gstTofInstCardDate, 0, sizeof(TOF_INST_CARD_DATA_S));
	gstTofInstCardDate.stTofCardState[0].u16DestAddr = 10001;
	gstTofInstCardDate.stTofCardState[1].u16DestAddr = 10002;
	gstTofInstCardDate.stTofCardState[2].u16DestAddr = 10003;
	
	return;
}

void CardSendPollToStation()
{

	uint16 u16StaCount;
	instance_data_t* pstInstMsg = TdoaGetOldLocalInstStructurePtr();

	//设置目的地址
	u16StaCount = gstTofInstCardDate.u16StaCount;
	
	PrintfUtil_vPrintf("u16StaCount =%d	u16StaRespFinalFrame = %d u8TdoaCardSendCount = %d\n", 
		u16StaCount, gstTofInstCardDate.stTofCardState[u16StaCount-1].u16StaRespFinalFrame, u8TdoaCardSendCount);
	//若人卡接收到基站响应的最后一帧，则进行下一个基站的测距poll帧发送
	if (gstTofInstCardDate.stTofCardState[u16StaCount-1].u16StaRespFinalFrame == TRUE)
	{
		//进行范围安全检测
		if (u16StaCount >= TOF_CARD_TO_STATION_NUM)
		{
			//进行TOF数据初始化
			TofInstanceCardDataInit();
			gstTofInstCardDate.u16CarsTofStart = FALSE; //关闭TOF测距
			
			//再次开启TDOA测距 TDOA事件自动加载不需要再次设置
			u8TdoaCardSendCount = 0;
			u16TdoaQuickCardSendCount = 0;
			
			//关闭卡的tof测距事件
			event_timer_unset(EVENT_SEND_TOF_POLL);
			return;
		}
		
		pstInstMsg->rev_shortaddr16 = gstTofInstCardDate.stTofCardState[u16StaCount].u16DestAddr;
	}
	else
	{
		pstInstMsg->rev_shortaddr16 = gstTofInstCardDate.stTofCardState[0].u16DestAddr; //人卡先和第一个基站进行tof测距
	}
	
	txpoll_wait_send(pstInstMsg);

}

//TA_TXRESPONSE_WAIT_SEND,	//4

void txresponse_wait_send(instance_data_t *inst)
{
	//program option octet and parameters (not used currently)
	uint8 send_len=0;
	dwt_forcetrxoff() ;

	LastRsepTick=portGetTickCount();

	inst->macdata_msdu[RES_R1] = my_cardslotmsg.status;//0x2; // "activity"
	if(inst->mode != SUB_STA)
	{
		uint16 version = OAD_UWB_CARD_VERSION;
		if(my_cardslotmsg.u16SeqNum - pre_seqnum !=1 ||card_last_dist_msgtype ==1 )
			pre_distance =0;
		card_last_dist_msgtype =0 ;
		memcpy(&inst->macdata_msdu[RES_R1+1],&pre_distance,sizeof(uint32));
		memcpy(&inst->macdata_msdu[RES_R1+5],&my_cardslotmsg.u16SeqNum,sizeof(uint16));
		if(inst->shouldrepvbat)
		{
			memcpy(&inst->macdata_msdu[RES_R1+7],&inst->cardbattery,2);
			memcpy(&inst->macdata_msdu[RES_R1+9],&version,2);
			send_len = 12;
		}
		else if(!inst->shouldrepvbat)
		{
			inst->cardbattery =0xFFFF;
			memcpy(&inst->macdata_msdu[RES_R1+7],&inst->cardbattery,2);
			//memcpy(&inst->macdata_msdu[RES_R1+9],&version,2);
			send_len = 10;
		}
		pre_distance =0;
	}
	else
		send_len = 2;
	//set destination address
	memcpy(&inst->msg.destAddr, &inst->relpyAddress, ADDR_BYTE_SIZE);

	setupmacframedata(inst, send_len, RTLS_DEMO_MSG_ANCH_RESP, !ACK_REQUESTED);

	inst->testAppState = TA_TX_WAIT_CONF;												// wait confirmation
	inst->previousState = TA_TXRESPONSE_WAIT_SEND ;

	if(instancesendpacket(inst,DWT_START_TX_DELAYED  ))//   DWT_START_TX_IMMEDIATE
	{
		//error - TX FAILED
		EDBG(PrintfUtil_vPrintf("RN ");)
		inst->txu.txTimeStamp = 0;
		//inst->testAppState = TA_RXE_WAIT ;	// wait to receive a new poll
		inst->shouldDoDelayedRx = FALSE ;	// no delay in turning on RX
		ta_rxe_wait(inst);
	}
	else
	{
		inst->shouldrepvbat =0;
		inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;  //no timeout
		dwt_setrxtimeout((RX_FWTO_TIME) * 1000);	//units are us - wait for 5ms after RX on
		if(inst->mode ==SUB_STA)
		{
		//	dwt_setrxtimeout(0);
		//	DBG(PrintfUtil_vPrintf("RY  id= %d \n",inst->rev_shortaddr16);)
			
		}
		else{
		//	dwt_setrxtimeout((RX_FWTO_TIME) * 1000);	
	//		DBG(PrintfUtil_vPrintf("RY ");)
		}
		mSleep(1);
	}

}

//TA_TXFINAL_WAIT_SEND,		//3
void txfinal_wait_send(instance_data_t *inst)//,new_slot_msg_t *slotmsg)
{
	uint64 tagCalculatedFinalTxTime ;
	uint32 temp0 =portGetTickCount();//LastPollTick = portGetTickCount();

	// Embbed into Final message: 40-bit pollTXTime,  40-bit respRxTime,  40-bit finalTxTime
	// Write Poll TX time field of Final message
	dwt_forcetrxoff() ;
	memcpy(&(inst->macdata_msdu[PTXT]), (uint8 *)&inst->txu.tagPollTxTime, 5);

	// Write Response RX time field of Final message
	memcpy(&(inst->macdata_msdu[RRXT]), (uint8 *)&inst->rxu.anchorRespRxTime, 5);

	// Calculate Time Final message will be sent and write this field of Final message
	// Sending time will be delayedReplyTime, snapped to ~125MHz or ~250MHz boundary by
	// zeroing its low 9 bits, and then having the TX antenna delay added
	tagCalculatedFinalTxTime = inst->delayedReplyTime & MASK_TXDTS; // 9 lower bits mask

	// getting antenna delay from the device and add it to the Calculated TX Time
	tagCalculatedFinalTxTime = tagCalculatedFinalTxTime + inst->txantennaDelay;

	tagCalculatedFinalTxTime &= MASK_40BIT;

	// Write Calculated TX time field of Final message
	memcpy(&(inst->macdata_msdu[FTXT]), (uint8 *)&tagCalculatedFinalTxTime, 5);

	//set destination address
	memcpy(&inst->msg.destAddr, &inst->relpyAddress, ADDR_BYTE_SIZE);

	inst->testAppState = TA_TXFINAL_WAIT_SEND;
	setupmacframedata(inst, TAG_FINAL_MSG_LEN, RTLS_DEMO_MSG_TAG_FINAL, !ACK_REQUESTED);

	if(instancesendpacket(inst, DWT_START_TX_DELAYED))//     DWT_START_TX_DELAYED
	{
		//error - TX FAILED
		inst->txu.txTimeStamp = 0;

		// initiate the re-transmission
		inst->testAppState = TA_TXPOLL_WAIT_SEND;//TA_TXE_WAIT ;
		inst->nextState = TA_TXPOLL_WAIT_SEND ;
#if (DEEP_SLEEP == 1)
		dwt_entersleepaftertx(0);
#endif
		return; //exit this switch case...
	}
	else
	{
		inst->testAppState = TA_TX_WAIT_CONF;												// wait confirmation
		inst->previousState = TA_TXFINAL_WAIT_SEND;
		dwt_setrxtimeout((RX_FWTO_TIME) * 1000);	//units are us - wait for 5ms after RX on
		inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out	(set below)
	}

	if(inst->mode !=ANCHOR)
	{
		uint32 temp =portGetTickCount();//LastPollTick = portGetTickCount();
		mSleep(2);
		DBG(PrintfUtil_vPrintf("poll_final=%d|%d  ",temp-LastPollTick,temp-temp0);)
		toftimesum = temp-toftimesum;
		
	}
	else if(inst->mode ==ANCHOR)
	{
		//PrintfUtil_vPrintf("card rev sub station report u8seqnum = %d\n", u8seqnum);
		PrintfUtil_vPrintf("range to sub_station over!");  //must printf or msleep(2);
		inst->newrange = 1;
	//	instance_set_insleep(); 
	}
}


void txreport_wait_send(instance_data_t *inst)
{
//	dwt_forcetrxoff() ;
	memcpy(&(inst->macdata_msdu[TOFR]), &my_cardslotmsg.m_distance, sizeof(uint32));

	setupmacframedata(inst, 5, RTLS_DEMO_MSG_ANCH_TOFR, !ACK_REQUESTED);

	if(instancesendpacket(inst, DWT_START_TX_IMMEDIATE ))// DWT_START_TX_IMMEDIATE
	{
		//error - TX FAILED
		EDBG(PrintfUtil_vPrintf("-report fail ");)
		inst->txu.txTimeStamp = 0;
		inst->delayedReplyTime = 0;
		inst->newReportSent++;
		//if fails to send, go back to receive mode and wait to receive a new poll
		if(inst->newReportSent < MAX_NUMBER_OF_REPORT_RETRYS) //re-try
		{
			//stay in this state
			inst->testAppState = TA_TXREPORT_WAIT_SEND ;
		}
		else
		{
			//inst->testAppState = TA_RXE_WAIT ;	// wait to receive a new poll
			ta_rxe_wait(inst);
		}
	}
	else
	{
		card_last_dist_msgtype =1;
		inst->testAppState = TA_TX_WAIT_CONF ;												 // wait confirmation
		inst->previousState = TA_TXREPORT_WAIT_SEND ;

		inst->newReportSent++;
		inst->delayedReplyTime = 0;
		inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT_TO;

	}
//	uint32 temp =portGetTickCount();//LastPollTick = portGetTickCount();
//	PrintfUtil_vPrintf("revF_sendRep_1=%d  ",temp-LastrevfinalTick);


}

void init_tag(instance_data_t *inst)
{
	PrintfUtil_vPrintf("********** TAG *************** \n");
	isnewblinkon=1;

	my_alarmlist.count = 0;
	
	dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN); //allow data, ack frames;
	dwt_setpanid(inst->panid);
//	dwt_seteui(inst->eui64);
#if (USING_64BIT_ADDR==0)
	dwt_setaddress16(u16ShortAddr);
	PrintfUtil_vPrintf("	 ADDR = %d \n",u16ShortAddr);
#endif
	inst->shortaddr16 = u16ShortAddr;
	//set source address into the message structure
	memcpy(&inst->msg.sourceAddr, &u16ShortAddr, ADDR_BYTE_SIZE);
	//change to next state - send a Poll message to 1st anchor in the list
#if (DR_DISCOVERY == 1)
	//inst->mode = TAG_TDOA ;
	inst->testAppState = TA_TXBLINK_WAIT_SEND;
//	memcpy(inst->blinkmsg.tagID, inst->eui64, ADDR_BYTE_SIZE);
#else
	inst->testAppState = TA_TXPOLL_WAIT_SEND;
#endif

	dwt_setautorxreenable(inst->rxautoreenable); //not necessary to auto RX re-enable as the receiver is on for a short time (Tag knows when the response is coming)
#if (DOUBLE_RX_BUFFER == 1)
	dwt_setdblrxbuffmode(0); //disable double RX buffer
#endif
#if (ENABLE_AUTO_ACK == 1)
	dwt_enableautoack(ACK_RESPONSE_TIME); //wait for 5 symbols before replying with the ACK
#endif



}


void init_anchor(instance_data_t *inst)
{
//	if(inst->mode == ANCHOR)
#ifdef DEC_UWB_ANCHOR
	{
		inst->panid = 0xdddd;
		DBG(PrintfUtil_vPrintf("CARD	 ADDR = %d \n",u16ShortAddr);)
	}
#endif
#ifdef DEC_UWB_SUB
	{
		inst->panid = 0xeeee;
		PrintfUtil_vPrintf("SUB STA	 ADDR = %d \n",u16ShortAddr);
	//	instance_clear_substa();
		reset_sub_alarmmsg();
	}
#endif
	inst->shortaddr16 = u16ShortAddr;
#if (DR_DISCOVERY == 0)
	uint8 eui64[8] ;
	memcpy(eui64, &inst->payload.anchorAddress, sizeof(uint64));

	dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN); //allow data, ack frames;
	dwt_seteui(eui64);
#else
	dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN); //allow data, ack frames;
//	dwt_enableframefilter(DWT_FF_NOTYPE_EN); //allow data, ack frames;

//	dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN|DWT_FF_COORD_EN|DWT_FF_MAC_EN); //allow data, ack frames; DWT_FF_MAC_EN

#endif
	dwt_setpanid(inst->panid);
	


#if (USING_64BIT_ADDR==0)
	{
		dwt_setaddress16(u16ShortAddr);
	}
#endif

#if (DR_DISCOVERY == 0)
	//set source address into the message structure
	memcpy(&inst->msg.sourceAddr, &inst->payload.anchorAddress, ADDR_BYTE_SIZE);
#else
	//set source address into the message structure
	memcpy(&inst->msg.sourceAddr, &u16ShortAddr, ADDR_BYTE_SIZE);
#endif
	// First time anchor listens we don't do a delayed RX
	inst->shouldDoDelayedRx = FALSE ;
	//change to next state - wait to receive a message
	//inst->testAppState = TA_RXE_WAIT ;
	
#if (ENABLE_AUTO_ACK == 1)
	dwt_setrxaftertxdelay(WAIT_FOR_RESPONSE_DLY); //set the RX after TX delay time
#endif

#if (DECA_BADF_ACCUMULATOR == 0) //can use RX auto re-enable when not logging/plotting errored frames
	inst->rxautoreenable = 1;
#endif
	dwt_setautorxreenable(inst->rxautoreenable);
//	dwt_setautorxreenable(1);

#if (DOUBLE_RX_BUFFER == 1)
	dwt_setdblrxbuffmode(0); //enable double RX buffer
#endif
	dwt_setrxtimeout(0);

	ta_rxe_wait(inst);

}

void dec_sleep_wakeup()
{
#if (DEEP_SLEEP == 1)
	{
		uint8 buffer[1000];
		//wake up device from low power mode
		//1000 takes 400us, 1300 takes 520us, 1500 takes 600us (SPI @ 20MHz)
		//then the function will wait 5ms to make sure the XTAL has stabilised
		if(dwt_spicswakeup(buffer, 1000) == DWT_ERROR) //1000 takes 400us, 1300 takes 520us, 1500 takes 600us (SPI @ 20MHz)
		{
			EDBG(PrintfUtil_vPrintf("FAILED to WAKE UP\n");)
		}
		//printf("SECCUSS to WAKE UP\n");

		//wake up device from low power mode
		//NOTE - in the ARM  code just drop chip select for 200us
		port_SPIx_clear_chip_select();	//CS low
		mSleep(1);	 //200 us to wake up then waits 5ms for DW1000 XTAL to stabilise
		port_SPIx_set_chip_select();  //CS high
		mSleep(1);

		//this is platform dependent - only program if DW EVK/EVB
		dwt_setleds(2);

		//MP bug - TX antenna delay needs reprogramming as it is not preserved
		dwt_settxantennadelay(tx_antennaDelay) ;

		//set EUI as it will not be preserved unless the EUI is programmed and loaded from NVM
		dwt_entersleepaftertx(0);
		//dwt_setinterrupt(DWT_INT_TFRS, 1); //re-enable the TX/RX interrupts

	//	dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | ( DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO /*| DWT_INT_RXPTO*/), 1);
		dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG   | DWT_INT_RFTO , 1);
	//	dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | (DWT_INT_ARFE | DWT_INT_RFSL | DWT_INT_SFDT | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO /*| DWT_INT_RXPTO*/), 1);
	}
#endif

}

//TA_TX_WAIT_CONF
void ta_tx_wait_conf(instance_data_t *inst,int message)
{
//  instancerxon(0, 0);
/*	if(message == DWT_SIG_RX_TIMEOUT) //got RX timeout - i.e. did not get the response (e.g. ACK)
	{
		//printf("RX timeout in TA_TX_WAIT_CONF (%d)\n", inst->previousState);
		//EDBG(PrintfUtil_vPrintf("-WOT ");)
		//if we got no ACKs after sending MAX_NUMBER_OF_REPORT_RETRYS reports go to wait for RX state
		if(inst->newReportSent && (inst->newReportSent >= MAX_NUMBER_OF_REPORT_RETRYS))
		{
			//don't change state - next event will the TX confirm from the sent report, so will just fall through to RX
			inst->shouldDoDelayedRx = FALSE ;
			inst->wait4ack = 0 ; //clear the flag as the ACK has been received
			dwt_setrxtimeout(0);
			if(inst->mode == ANCHOR&& inst->previousState == TA_TXREPORT_WAIT_SEND){
				//inst->testAppState = TA_RXE_WAIT;
				ta_rxe_wait(inst);
			}
		}
		else
		{
			//got timeout before TX confirm
			inst->testAppState = TA_RX_WAIT_DATA ;//TA_TXE_WAIT;
		//	DBG(PrintfUtil_vPrintf("(1)");)
		//	inst->nextState = inst->previousState ; // send poll / response / final / report (with ACK request)
		}
		return;
	}

	//NOTE: Can get the ACK before the TX confirm event for the frame requesting the ACK
	//this happens because if polling the ISR the RX event will be processed 1st and then the TX event
	//thus the reception of the ACK will be processed before the TX confirmation of the frame that requested it.
	if(message != DWT_SIG_TX_DONE) //wait for TX done confirmation
	{
		if(message == SIG_RX_ACK)
		{
			inst->wait4ack = 0 ; //clear the flag as the ACK has been received
			dwt_setrxtimeout(0); //clear/disable timeout
			//printf("RX ACK in TA_TX_WAIT_CONF... wait for TX confirm before changing state\n");
		}

		if(0)//(TAGTxTick > 50*CLOCKS_PER_MILLI && inst->mode == TAG)
		{
#if (DR_DISCOVERY == 1)
				inst->testAppState = TA_TXBLINK_WAIT_SEND;
				inst->done = INST_NOT_DONE_YET;
				inst->mode = TAG_TDOA ;
#endif
		}
		else
		{
			inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
		}
		return;

	}

	message = 0; //finished with the message

	//printf("got TX done stateCount %d\n", stateCount);
//	stateCount = 0;  //ARM counts about 165 times...
	inst->done = INST_NOT_DONE_YET;


	switch(inst->previousState)
	{
		case TA_TXBLINK_WAIT_SEND:
		case TA_TXPOLL_WAIT_SEND:
		case TA_TXRESPONSE_WAIT_SEND:
			inst->shouldDoDelayedRx = TRUE ;						// response is delayed so we can delay turning on RX
			break;

		case TA_TXREPORT_WAIT_SEND:
		case TA_TXFINAL_WAIT_SEND: // if expecting a report don't delay turning on RX
		case TA_TXRANGINGINIT_WAIT_SEND:
		default:
			inst->shouldDoDelayedRx = FALSE ;
		break;
	}
	if(inst->previousState == TA_TX_RANGING_ACK_SEND)
	{
#if(BLINK_AGAIN_IN_END ==1)
		//change the channel
		inst->testAppState = TA_TXBLINK_WAIT_SEND;
#else
		inst->testAppState = 100;
		instance_set_status(2);           //out
		return;
#endif
	}
	else*/
	{
		uint32 temp =portGetTickCount();
		if(inst->mode == ANCHOR)
		{
			if(inst->previousState==TA_TXFINAL_WAIT_SEND)
			{

				//	DBG(PrintfUtil_vPrintf("revF_sendRep_Y=%d  ",temp-instance_get_slot_starttick());)
				instance_set_insleep(); 
			//	inst->newrange = 1;
				return ;

			}
			else if(inst->previousState==TA_TXREPORT_WAIT_SEND)
			{

				PrintfUtil_vPrintf("revF_sendRep_Y=%d  ",temp-LastrevfinalTick);
				inst->newrange = 1;
				return;
			}
		}
		else if(inst->previousState== TA_TXBLINK_WAIT_SEND)
		{
			isnewblinkon = 0;
		}
		else if(inst->previousState== TA_TXPOLL_WAIT_SEND)
			inst->txu.tagPollTxTime = inst->txu.txTimeStamp;
		//inst->testAppState = TA_RXE_WAIT ;						// After sending, tag expects response/report, anchor waits to receive a final/new poll
		ta_rxe_wait(inst);
	} //fall into the next case (turn on the RX)

}

uint8 instance_framefilter(uint8 type,uint8 curmode)
{
	uint8 result=0;
	if(curmode == TAG)
	{
		switch(type)
		{
			case RTLS_DEMO_MSG_RNG_INIT:
			case RTLS_DEMO_MSG_ANCH_RESP:
			case RTLS_DEMO_MSG_ANCH_TOFR:
				result = 1;
				break;
			default:
				result = 0;
				break;
		}
	}
	else if(curmode == ANCHOR)
	{
		switch(type)
		{
			case RTLS_DEMO_MSG_RNG_INIT:
			case RTLS_DEMO_MSG_ANCH_TOFR:
			case RTLS_MSG_HELP_CALL:
			case RTLS_EXCIT_ASK_SEND:
			case RTLS_RETREAT_ACK_SEND:
				result = 0;
				break;
			default:
				result = 1;
				break;
		}
	}
	else if(curmode == SUB_STA)
	{
		switch(type)
		{
			case RTLS_DEMO_MSG_TAG_POLL:
			case RTLS_DEMO_MSG_TAG_FINAL:
			case RTLS_MSG_HELP_CALL:
			case RTLS_EXCIT_ASK_SEND:
			case RTLS_RETREAT_ACK_SEND:
			case RTLS_CAR_REVTOFCARD_MSG:
				result = 1;
				break;
			default:
				result = 0;
				break;
		}
	}
	else
		result = 0;
	return result;
}

int testapprun(instance_data_t *inst, int message)//new_slot_msg_t *slotmsg)
{
//	printf_event(inst,message);

    switch (inst->testAppState)
    {
        case TA_TXBLINK_WAIT_SEND :
            txblink_wait_send(inst);

            break ; // end case TA_TXBLINK_WAIT_SEND

        case TA_TXPOLL_WAIT_SEND :
            txpoll_wait_send(inst);
            break;
	//	case TA_TX_WAIT_CONF :
	///		ta_tx_wait_conf(inst,message);
	//		break;
		case TA_TDOA_END_TOF_START:
			//TDOA结束后开始进行tof测距
			CardSendPollToStation();
			break;
			
        case TA_RX_WAIT_DATA :                                                                     // Wait RX data

            switch (message)
            {

                case DWT_SIG_RX_OKAY :
                {
                    srd_msg *rxmsg = &inst->rxmsg;
                    uint16  srcAddr,destAddr;
					uint8 rev_curslot;
					int fcode = 0;

                    // 16 or 64 bit addresses
                    memcpy(&srcAddr, &(rxmsg->sourceAddr), ADDR_BYTE_SIZE);
					memcpy(&destAddr, &(rxmsg->destAddr), ADDR_BYTE_SIZE);
					fcode = rxmsg->messageData[FCODE];
					rev_curslot = rxmsg->seqNum;
					inst->relpyAddress = srcAddr; //remember who to send the reply to
				
					if(!instance_framefilter((uint8)fcode,inst->mode))
					{
						dwt_setrxtimeout(0);
						instancerxon(0, 0); //immediate enable
						return 0;
					}
#ifndef DEC_UWB_ANCHOR
					led_station_flash();
#endif
					switch(fcode)
					{
						case RTLS_TDOA_MSG_CARD_POLL:
							
							PrintfUtil_vPrintf("shortaddr16 = %d mode = %d\n", 
												inst->shortaddr16, inst->mode);
							//TDOA流程
							if (inst->mode == TAG)
							{
								//PrintfUtil_vPrintf("100003\n");
								//将实例中由接收器缓冲区获取的数据赋值给卡的全局缓冲区
								TdoaInstRxMsgToCardMsgProcNew();
								//修改实例状态
								inst->testAppState = TA_RX_WAIT_DATA;
								//清除数据缓冲区避免没有接收时使用上一次数据进行处理
								memset(&inst->rxmsg, 0, sizeof(srd_msg));
							//	pstInstMsg->stInstBaseData.eDwEventType = DWT_SIG_RX_NOERR; //改变接收状态，等待下一次接收
								//设置设备触发组包事件
								event_timer_set(EVENT_CHECKTDOA_REVMSG_EVENT); 
							}
							else
							{
								dwt_rxenable(0);
							}

							break;
							
							
							
						case RTLS_MSG_HELP_CALL:
							memcpy(&inst->rev_shortaddr16,&srcAddr,ADDR_BYTE_SIZE);
							if(sub_alarmmsg.alarmaddr ==inst->rev_shortaddr16 )
							{
								if(!(sub_alarmmsg.alarmstatus & UWB_CARD_STATUS_HELP) )  //reset ,0xff is  the first time 
								{
									reset_sub_alarmmsg();
								}
								else
								{
									//uart send to Stm32
									event_timer_set(EVENT_UART_SEND); 
									instance_set_sta_status(STATION_TYPE_ALARM);
									sub_alarmmsg.alarmstatus = rxmsg->messageData[1] ;
									ta_rxe_wait(inst);
								}
							}
							else
							{
								event_timer_set(EVENT_UART_SEND); 
								instance_set_sta_status(STATION_TYPE_ALARM);
								sub_alarmmsg.alarmaddr = inst->rev_shortaddr16;
								sub_alarmmsg.alarmstatus = rxmsg->messageData[1] ;
								
							}
							txhelp_resp_send(inst);
							break;
						case RTLS_RETREAT_ACK_SEND:
							event_timer_set(EVENT_UART_SEND); 
							rev_retreat_ack =1;
							sub_alarmmsg.alarmaddr = srcAddr;
							sub_alarmmsg.alarmstatus = rxmsg->messageData[1] ;
							break;
						case RTLS_MSG_HELP_RESP:
							helpreportoff = rxmsg->messageData[1];
							if(!(helpreportoff & UWB_CARD_STATUS_HELP) )
								my_cardslotmsg.status &=~UWB_CARD_STATUS_HELP;
							inst->help_excit_send |=0x01;
							DBG(PrintfUtil_vPrintf("********help resp ok!\n ");)
							break;
							
						case RTLS_EVACUATE_ASK_SEND:
							//set buzzer tweet
							event_timer_add(EVENT_URGENT_RETREAT,10);
							break;
						case RTLS_EXCIT_ASK_SEND:
							DBG(PrintfUtil_vPrintf("******RTLS_EXCIT_ASK_SEND  = %d  \n ",sub_alarmmsg.alarmstatus);)
							inst->rev_shortaddr16 = srcAddr;
							if(sub_alarmmsg.alarmaddr == srcAddr )  //reset ,0xff is  the first time 
							{
								if(!(sub_alarmmsg.alarmstatus & UWB_CARD_STATUS_IMPEL))
								{
									reset_sub_alarmmsg();
								}
								else
								{
									event_timer_set(EVENT_UART_SEND); 
									instance_set_sta_status(STATION_TYPE_EXCIT);
									sub_alarmmsg.alarmaddr = srcAddr ;
									sub_alarmmsg.alarmstatus = rxmsg->messageData[1] ;
									sub_alarmmsg.excitid = rxmsg->messageData[2]; 
								}
							}
							else
							{
								//uart send to Stm32
								event_timer_set(EVENT_UART_SEND); 
								instance_set_sta_status(STATION_TYPE_EXCIT);
								sub_alarmmsg.alarmaddr = srcAddr ;
								sub_alarmmsg.alarmstatus = rxmsg->messageData[1] ;
								sub_alarmmsg.excitid = rxmsg->messageData[2]; 
							}
							txexcit_ack_send();
							break;
						case RTLS_EXCIT_ACK_SEND:
							if(!(rxmsg->messageData[1] & UWB_CARD_STATUS_IMPEL) )
								my_cardslotmsg.status &=~UWB_CARD_STATUS_IMPEL;
							inst->help_excit_send |=0x10;
							break;
						case SIG_RX_BLINK :
						{
							u8seqnum ++;
						//	PrintfUtil_vPrintf("rev blink seq = %d\n", u8seqnum);
						
							uint8 slotmsg=0,tick,slot=0,i=0;
							if(inst->mode != ANCHOR)
								break;
							
							slotmsg = rxmsg->messageData[1];
							tick = rxmsg->messageData[2];
							inblink_status =0;
							
						//	PrintfUtil_vPrintf("messageData[5] = %d\n", rxmsg->messageData[5]);
							
							if(rxmsg->messageData[5] & STATION_TYPE_ALARM)       //all alarm
							{
								if(!(my_cardslotmsg.status &UWB_CARD_STATUS_RETREAT))
									event_timer_add(EVENT_URGENT_RETREAT,5 );
								//event_timer_add(EVENT_URGENT_RESET,600000);
								my_cardslotmsg.status |=UWB_CARD_STATUS_RETREAT;
							}
							else if(rxmsg->messageData[5]& STATION_TYPE_ALARM_ANY)
							{
								uint16 alarmaddr=0;
								//uint8 count =( rxmsg->messageData[5]>>5)&0xFF;
								for(i=0;i<rxmsg->messageData[5];i++)
								{
									memcpy(&alarmaddr,&rxmsg->messageData[6+2*i],ADDR_BYTE_SIZE);
									if(alarmaddr == inst->shortaddr16 && 
										(my_cardslotmsg.status &UWB_CARD_STATUS_RETREAT) )
									{
										event_timer_add(EVENT_URGENT_RETREAT,5 );
										my_cardslotmsg.status |=UWB_CARD_STATUS_RETREAT ;
										break;
									}	
								}
								if(0 == rxmsg->messageData[5])
									my_cardslotmsg.status &=~UWB_CARD_STATUS_RETREAT;
							}
							else if(rxmsg->messageData[5] &STATION_TYPE_ALARM_RESET)
							{
								if(my_cardslotmsg.status & UWB_CARD_STATUS_RETREAT )
									event_timer_add(EVENT_URGENT_RETREAT,5 );
								my_cardslotmsg.status &=~UWB_CARD_STATUS_RETREAT;
							}
						
							if(!instance_check_devchg(rxmsg->messageData[5]))  //change the dev type?
							{
							//	if(slotfirstinpoll ==1)  //only set when first rev the blink msg
								{
									inst->up_revrpoll_time = portGetTickCount()- instance_get_slot_starttick();
									slotfirstinpoll = 0;
								}
								if(my_cardslotmsg.u8DeviceType == CARD_1S){
									slot = rxmsg->messageData[3];
								}
								else if(my_cardslotmsg.u8DeviceType == CARD_5S){
									slot = rxmsg->messageData[4];
								}
								
								if(my_cardslotmsg.b1Used == USED_TOF && bool_check_poll==1
									&& my_cardslotmsg.dest_addr[0] == rxmsg->sourceAddr[0]
									&& my_cardslotmsg.dest_addr[1] == rxmsg->sourceAddr[1])
								{
								//	instance_check_discon(srcAddr);
									instance_set_revpolltype(1);
									instance_set_sleeptick(tick,rev_curslot,255);
									PrintfUtil_vPrintf(" -----tof process but rev blink----slot=%d |%d\n",rev_curslot,card_inblink_slot);
									//ta_rxe_wait(inst);
									instance_set_insleep();
									break;
								}
								else if(my_cardslotmsg.b1Used == IDLE) //idle
								{
									inblink_status = check_inblinkmsg(inst,srcAddr);
									//PrintfUtil_vPrintf(" -------rssi =%i  status =%d\n",inst->i8rssi,inblink_status);
									if(inblink_status == 2)
									{
										ta_rxe_wait(inst);
										break;
									}
									else if(inblink_status == 3)
									{
										instance_set_insleep();
										break;
									}
									//计算休眠时间
									if(new_inblinkmsg.count<2){   
										instance_set_sleeptick(tick,rev_curslot,slot);
									}
									
									if(slotmsg != BLINK_NO_RESP )	
									{
									
										if(slotmsg == BLINK_ALL_RESP || (slotmsg == BLINK_5S_RESP && my_cardslotmsg.u8DeviceType == CARD_5S))
										{
											//计算休眠时间
										//	instance_set_sleeptick(tick,rev_curslot,slot);
											//initiate ranging message
											inst->testAppState =TA_TXRANGINGINIT_WAIT_SEND;// TA_TXE_WAIT;
										//	inst->nextState = TA_TXRANGINGINIT_WAIT_SEND ;
											memcpy(&inst->rev_shortaddr16,&rxmsg->sourceAddr,ADDR_BYTE_SIZE);
											mSleep(1);	
											txranginginit_wait_send(inst);
										}
										else //ins't the same type card
										{
											ta_rxe_wait(inst);
										}
										
									}
									else if(slotmsg == BLINK_NO_RESP )   //don't need response the blink massage
									{
										ta_rxe_wait(inst);
									}
								}
							}
							else
							{
								PrintfUtil_vPrintf(" ------- change dev type status=%x \n",rxmsg->messageData[5]);
								ta_rxe_wait(inst);
							}
							
						}
                        break; //RTLS_DEMO_MSG_TAG_POLL

						case RTLS_DEMO_MSG_RNG_INIT:
						{
							
							uint8 temp=255;
							double delay = rxmsg->messageData[RES_T1] + (rxmsg->messageData[RES_T2] << 8); //in ms
							
							inst->testAppState = TA_TX_RANGING_ACK_SEND;//TA_RXE_WAIT ; // send next poll   ;//

							inst->rev_card_type = rxmsg->messageData[SLOT_MSG];
							
							test_tof_an_addr = inst->relpyAddress ;//get_curslot_destaddr(inst->cur_slot);
							if(inst->relpyAddress !=0)
							{
								//temp = check_slot_list(srcAddr,inst->rev_card_type,inst->cur_slot);
								if(temp ==255)
								{
									instance_inset_slotlist(inst,inst->relpyAddress  );//inst->cur_slot,inst->rev_card_type,srcAddr);
							//		PrintfUtil_vPrintf("\nin web:slot = %d ;addr= %d  \n",inst->cur_slot,inst->relpyAddress );
								}
							//	else
							//		PrintfUtil_vPrintf("\nOLD web:slot = %d ;addr= %d  \n",temp,inst->relpyAddress );
							//	memcpy(&inst->msg.destAddr, &inst->relpyAddress, ADDR_BYTE_SIZE);
							//	mSleep(1);
								txranging_ack_wait_send(inst,temp);
								PrintfUtil_vPrintf(" addr =%d ,slot=%d\n",srcAddr,rxmsg->seqNum);
							}
						}
                        break; //RTLS_DEMO_MSG_TAG_POLL
                        
						case RTLS_TOF_MSG_TAG_ACK:
						{
							PrintfUtil_vPrintf("rev ack rang init seq = %d\n", u8seqnum);
							
							uint8 tick,slot ;
							uint16 revaddr=0;
							//slot_sum = rxmsg->messageData[1];
							tick = rxmsg->messageData[2];   //inst->slottimer
							//slot = rxmsg->seqNum;
							slot = rxmsg->messageData[5];
							//		PrintfUtil_vPrintf("- time = %d  ; slot=%d \n",tick,slot);
							memcpy(&revaddr,&rxmsg->messageData[allot_slot_1],ADDR_BYTE_SIZE);

							if(revaddr ==u16ShortAddr)
							{
								my_cardslotmsg.u8cur_slot = rev_curslot;
								my_cardslotmsg.dest_addr[0] =rxmsg->sourceAddr[0];
								my_cardslotmsg.dest_addr[1] =rxmsg->sourceAddr[1];
								//计算休眠时间
								//	if(slotfirstinpoll ==1)
								inblink_uptime = inst->up_revrpoll_time;
								instance_set_sleeptick(tick,rev_curslot,slot);
								PrintfUtil_vPrintf("-in blink !  rev_curslot =%d ,slot=%d ,mysleep=%d  statino=%d\n",rev_curslot,slot,my_cardslotmsg.sleeptick,srcAddr);
								//instance_set_AnchorPanid((uint8)ANCHOR_TOF_PRO);
								//my_cardslotmsg.b1Used = USED_TOF;
								inst->new_inblink =1;
								new_inblinkmsg.status = 1;
								instance_set_revpolltype(1);
								if(inblink_status == 0 || new_inblinkmsg.count>=3)
									instance_set_insleep(); 
								new_tof_in =ALL_TOF_TIME;
								card_inblink_slot = slot;
							}
							if((my_cardslotmsg.status & UWB_CARD_STATUS_HELP) && !(inst->help_excit_send & 0x01))
								txhelp_call_send();
							else if((my_cardslotmsg.status & UWB_CARD_STATUS_IMPEL) && !(inst->help_excit_send & 0x10))
								event_timer_set(EVENT_EXCIT_EVENT);
						//	inst->is_intoflist =1;
							if(inblink_status != 0 && new_inblinkmsg.count<3)
								ta_rxe_wait(inst);
						}
						break;


						case RTLS_DEMO_MSG_TAG_POLL:
						{
							PrintfUtil_vPrintf("rev poll seq = %d\n", u8seqnum);
							toftimesum = portGetTickCount();
							inst->cur_slot = rev_curslot;
							inst->rev_shortaddr16 =srcAddr ;
							if(destAddr!=u16ShortAddr && destAddr!=0xffff)
								break;
							else if( inst->mode == ANCHOR && my_cardslotmsg.b1Used == IDLE)  //card reset or lost to much clear but rev tof again
							{
								if(destAddr==u16ShortAddr)
								{
									my_cardslotmsg.b1Used = USED_TOF;
									my_cardslotmsg.u8cur_slot = rev_curslot;
									memcpy(&my_cardslotmsg.dest_addr[0],&rxmsg->sourceAddr[0],ADDR_BYTE_SIZE);
								}
								else if(destAddr == 0xffff)
								{
#if(POLL_HAVE_BLINK_MSG ==1)
									uint8 slot=255;
									if(my_cardslotmsg.u8DeviceType == CARD_1S){
										slot = rxmsg->messageData[6];
									}
									else if(my_cardslotmsg.u8DeviceType == CARD_5S){
										slot = rxmsg->messageData[7];
									} 
									instance_set_sleeptick(rxmsg->messageData[2],rev_curslot,slot);
#endif
								}
							}
							
							instance_set_revpolltype(1);

							if(inst->mode == ANCHOR)
							{
								if(slotfirstinpoll ==1)  //only set when first rev the blink msg ,
								{
									inst->up_revrpoll_time = toftimesum- instance_get_slot_starttick();
									inst->have_rev_pollOblink =1;
									instance_set_sleeptick(rxmsg->messageData[2],rev_curslot,255);
									slotfirstinpoll =0;
								}

								if(rxmsg->messageData[1] == 0xFF     //retreat
								&&!(my_cardslotmsg.status &UWB_CARD_STATUS_RETREAT)) //not beep when have been stop it
								{
									event_timer_set(EVENT_URGENT_RETREAT);
									//event_timer_add(EVENT_URGENT_RESET,600000);
									my_cardslotmsg.status |=UWB_CARD_STATUS_RETREAT ;
								}
								else if(rxmsg->messageData[1] == 0x00   //reset retreat
								&&(my_cardslotmsg.status &UWB_CARD_STATUS_RETREAT))
								{
									event_timer_set(EVENT_URGENT_RETREAT);
									my_cardslotmsg.status &=~UWB_CARD_STATUS_RETREAT;
								}
							}
					//		if((destAddr==u16ShortAddr||destAddr==0xffff))  //have been in net   && my_cardslotmsg.b1Used == USED_TOF   || inst->mode == SUB_STA
							{

								inst->tagPollRxTime = inst->rxu.rxTimeStamp ; //Poll's Rx time


								delayedReplyTime = 0;
								delayedReplyTime += inst->tagPollRxTime + inst->fixedReplyDelay;
								//delayedReplyTime += inst->fixedReplyDelay;

								inst->delayedReplyTime = inst->tagPollRxTime + inst->fixedReplyDelay ;  // time we should send the response

								if(inst->mode == ANCHOR )
								{
									instance_check_devchg(rxmsg->messageData[POLL_VOLT+3]);
									inst->station_status = rxmsg->messageData[POLL_VOLT+3];
								}
								else if(inst->mode == SUB_STA)
								{
								//	reset_sub_alarmmsg();
									inst->seqnum = (rxmsg->messageData[POLL_VOLT+1] << 8)&0xffff;
									inst->seqnum += rxmsg->messageData[POLL_VOLT+2]&0xff;
									
									my_staslotmsg.u8DeviceType =rxmsg->messageData[POLL_VOLT];
									memcpy(&my_staslotmsg.m_distance, &(rxmsg->messageData[POLL_VOLT+3]), 4);   //main station distance
									memcpy(&my_staslotmsg.dest_addr, &rxmsg->sourceAddr[0], ADDR_BYTE_SIZE);   //main station distance
									my_staslotmsg.u16SeqNum = inst->seqnum;
									my_staslotmsg.status = rxmsg->messageData[1] ;
									
									
									if(my_staslotmsg.m_distance !=0)
										inst->oldrange = 1;
								}
								rx_power_lever(inst);
								inst->testAppState = TA_TXRESPONSE_WAIT_SEND ; // send our response

								txresponse_wait_send(inst);
							}

                        }
                        break; //RTLS_DEMO_MSG_TAG_POLL

                        case RTLS_DEMO_MSG_ANCH_RESP: //此方案为人卡发送最后一帧 基站接收并响应最后一帧
                        {
							uint8 row=0,column=0,i=0;
							uint16 battery=0,cardseq=0;
							uint32 old_dis =0;
							inst->tag2rxReport = rxmsg->messageData[RES_R2]; //check if the anchor is going to send the report
							                                     //if no report coming, go to sleep before sending the next poll
							inst->rxu.anchorRespRxTime = inst->rxu.rxTimeStamp ; //Response's Rx time

							inst->delayedReplyTime = inst->rxu.anchorRespRxTime + inst->fixedReplyDelay ;  // time we should send the response
							inst->delayedReplyTime &= MASK_TXDTS ;

							inst->testAppState = TA_TXFINAL_WAIT_SEND ; // send our response / the final
							if(inst->mode == TAG)
							{
								row=inst->curslot_row;      //0-4
								column = inst->curslot_column; //0-39
								my_staslotmsg.status = rxmsg->messageData[1] ;
							
								if(my_slotlist[column].allocslot[row].u8DeviceType == CARD_5S)
								{
									my_slotlist[column].allocslot[row].u8LostNum = 0;
									my_slotlist[column].allocslot[row].status = my_staslotmsg.status;
								}
								else if(my_slotlist[column].allocslot[row].u8DeviceType == CARD_1S)
								{
									for(i=0;i<MAX_CARD_CYCLE_SEC;i++)
									{
										my_slotlist[column].allocslot[i].u8LostNum = 0;
										my_slotlist[column].allocslot[i].status = my_staslotmsg.status;
									}
								}
								my_staslotmsg.u8DeviceType = my_slotlist[column].allocslot[row].u8DeviceType;
								
								memcpy(&cardseq,&rxmsg->messageData[RES_R1+5],2);
								my_slotlist[column].allocslot[row].u16SeqNum = cardseq;
								inst->seqnum = cardseq;
								my_staslotmsg.u16SeqNum = cardseq;

								memcpy(&old_dis,&rxmsg->messageData[RES_R1+1],sizeof(uint32));
								if(old_dis !=0)
									instance_check_predistance(inst,old_dis);
								
								
							}
							rx_power_lever(inst);
							//non_user_payload_len = ANCH_RESPONSE_MSG_LEN;
							txfinal_wait_send(inst);//,slotmsg);
							if(inst->mode == TAG)
							{
								memcpy(&battery,&rxmsg->messageData[RES_R1+7],2);
								if(battery!=0xFFFF)
								{
									uint16 version=0;
									memcpy(&version,&rxmsg->messageData[RES_R1+9],2);
									vProcessCardVersion(srcAddr,version, battery);
								}
							}
						}
						break; //RTLS_DEMO_MSG_ANCH_RESP

						case RTLS_DEMO_MSG_ANCH_TOFR:  //人卡收到测距结果 判断基站id准备进行下一个基站测距
						{
							uint8 u8StaNum;
							memcpy(&my_staslotmsg.m_distance, &(rxmsg->messageData[TOFR]), 4);
							//reportTOF1(inst);
							inst->newrange = 1;
							inst->get_anchoraddr = srcAddr;
							my_staslotmsg.dest_addr[0] = rxmsg->sourceAddr[0];
							my_staslotmsg.dest_addr[1] = rxmsg->sourceAddr[1];

							//non_user_payload_len = TOF_REPORT_MSG_LEN;
							set_curslot_distance(my_staslotmsg.m_distance,rev_curslot,0);
							uint32 temp =portGetTickCount();//LastPollTick = portGetTickCount();
							DBG(PrintfUtil_vPrintf("poll_revRep=%d ",temp-LastPollTick);)

							//人卡进行tof测距设置 若正确收到基站的测距报告则进行下一个基站tof测距
							for (u8StaNum = 0; u8StaNum < TOF_CARD_TO_STATION_NUM; u8StaNum++)
							{
								if (srcAddr == gstTofInstCardDate.stTofCardState[u8StaNum].u16DestAddr)
								{
									gstTofInstCardDate.stTofCardState[u8StaNum].u16StaRespFinalFrame = TRUE;
									gstTofInstCardDate.u16StaCount = u8StaNum + 1;
									event_timer_set(EVENT_SEND_TOF_POLL);
								}
							}

							
#if(BLINK_AGAIN_IN_END ==0)
						//	if(inst->seqnum %3 != 0) //do not blink if card do tof to sub station, if rev this event ,the card must be not send to sub 
								blink_in_slotend(inst);
#else
							ta_rxe_wait(inst);
#endif
                        }
                        break; //RTLS_DEMO_MSG_ANCH_TOFR

                        case RTLS_DEMO_MSG_TAG_FINAL: //基站接收将测距结果通过串口发送 并发送测距报告给人卡
                        {
							PrintfUtil_vPrintf("rev final seq = %d\n", u8seqnum);
							uint64 tRxT, tTxT, aRxT, aTxT ;
							uint64 tagFinalTxTime  = 0;
							uint64 tagFinalRxTime  = 0;
							uint64 tagPollTxTime  = 0;
							uint64 anchorRespRxTime  = 0;
							uint64 pollRespRTD  = 0;
							uint64 respFinalRTD  = 0;
							//	test_final_dea =0;
							LastrevfinalTick =portGetTickCount();//LastPollTick = portGetTickCount();

							// time of arrival of Final message
							tagFinalRxTime = inst->rxu.rxTimeStamp ; //Final's Rx time

#if (FIXED_REPORT_DELAY > 0)
							inst->delayedReplyTime = tagFinalRxTime + inst->fixedReportDelay ;
							inst->delayedReplyTime &= MASK_TXDTS ;
#else
							inst->delayedReplyTime = 0 ;
#endif
							//	test_final_dea |=0x01;
							// times measured at Tag extracted from the message buffer
							// extract 40bit times
							memcpy(&tagPollTxTime, &(rxmsg->messageData[PTXT]), 5);
							memcpy(&anchorRespRxTime, &(rxmsg->messageData[RRXT]), 5);
							memcpy(&tagFinalTxTime, &(rxmsg->messageData[FTXT]), 5);
							// poll response round trip delay time is calculated as
							// (anchorRespRxTime - tagPollTxTime) - (anchorRespTxTime - tagPollRxTime)
							aRxT = (anchorRespRxTime - tagPollTxTime) & MASK_40BIT;
							aTxT = (inst->txu.anchorRespTxTime - inst->tagPollRxTime) & MASK_40BIT;
							pollRespRTD = (aRxT - aTxT) & MASK_40BIT;

							// response final round trip delay time is calculated as
							// (tagFinalRxTime - anchorRespTxTime) - (tagFinalTxTime - anchorRespRxTime)
							tRxT = (tagFinalRxTime - inst->txu.anchorRespTxTime) & MASK_40BIT;
							tTxT = (tagFinalTxTime - anchorRespRxTime) & MASK_40BIT;
							respFinalRTD = (tRxT - tTxT) & MASK_40BIT;

							// add both round trip delay times
							inst->tof = ((pollRespRTD + respFinalRTD) & MASK_40BIT);

							reportTOF(inst);
							//	memcpy(&inst->get_anchoraddr,&rxmsg->sourceAddr,sizeof(uint64));
							inst->get_anchoraddr = srcAddr;


							uint8 u8StaNum = 0;
							//组装tof的结果数据准备往串口发送 人卡发送 基站接收
							if (inst->mode == TAG)
							{
								//此工程用于辅助测试 暂时进行缓冲区位置固定输入
								if (destAddr == 10001)	
								{
									TofCardOfStation(distlist[0]);
								}
								else if (destAddr == 10002)	
								{
									TofCardOfStation(distlist[1]);
								}
								else if (destAddr == 10003)	
								{
									TofCardOfStation(distlist[2]);
								}

								//若缓冲区前三个距离均不为0，则将测距结果进行串口发送
								if ((distlist[0].u32StationDistance != 0) &&
									(distlist[1].u32StationDistance != 0) &&
									(distlist[2].u32StationDistance != 0))
								{
									event_timer_set(EVENT_UART_SEND);
								}
							}
							
							//保持接收
							ta_rxe_wait(inst);

#if 0


							if(1)//(inst->sendTOFR2Tag)
							{
								
								if(inst->mode == ANCHOR)
								{
									ever_rev_dis =1;
									my_cardslotmsg.m_distance =(uint32)(inst_idist *100);
									pre_distance  = my_cardslotmsg.m_distance;
									pre_seqnum = my_cardslotmsg.u16SeqNum;
								//	DBG(PrintfUtil_vPrintf("m_distance =%d  tof=%d  \n",my_cardslotmsg.m_distance,(uint32)(inst->tof));)
#if (MAIN_AND_SUB_RANGING ==0)
									inst->testAppState = TA_TXREPORT_WAIT_SEND;
									//  inst->nextState = TA_TXREPORT_WAIT_SEND ; // send the report with the calculated time of flight   
									inst->newReportSent = 0; //set the new report flag
									//uint32 temp =portGetTickCount();//LastPollTick = portGetTickCount();
									//PrintfUtil_vPrintf("revF_RevEnd=%d|revP=%d | ",temp-LastrevfinalTick ,temp-toftimesum);
									txreport_wait_send(inst);
#else


									card_last_dist_msgtype =0;//if next slot it is equal to 0 , tof to sub station fail


									if(((my_cardslotmsg.u16SeqNum) %3 != 0)&&new_tof_in==0 && inst_idist >TOF_TO_SUB_MIN_DIST) //
									{
										
										inst->testAppState = TA_TXREPORT_WAIT_SEND;
										inst->newReportSent = 0; //set the new report flag

										//uint32 temp =portGetTickCount();//LastPollTick = portGetTickCount();
										//PrintfUtil_vPrintf("revF_RevEnd=%d|revP=%d | ",temp-LastrevfinalTick ,temp-toftimesum);
										PrintfUtil_vPrintf("u16SeqNum = %d m_distance =%d new_tof_in = %d \n",
											my_cardslotmsg.u16SeqNum, my_cardslotmsg.m_distance, new_tof_in);

										txreport_wait_send(inst);
									}
									else 
									{ 
										card_send_poll2sub(inst);
								}
					

#endif
								}
								else if(inst->mode == SUB_STA)
								{
									my_staslotmsg.s_distance = (uint32)(inst_idist *100) ;
									//DBG(PrintfUtil_vPrintf("S_distance =%d  tof=%d  \n",my_cardslotmsg.s_distance,inst_idist *100);)
									inst->testAppState = TA_RXE_WAIT;
									ta_rxe_wait(inst);
									inst->newrange = 1;
								}

								
                            }
                            else
                            {
#if (DOUBLE_RX_BUFFER == 0)
                                //inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
#endif
                                inst->shouldDoDelayedRx = FALSE ;               // no delay turning on RX
                                
								ta_rxe_wait(inst);
                            }
                        //    inst->relpyAddress = srcAddr; //remember who to send the reply to
#endif                       
                        }
								
                        break; //RTLS_DEMO_MSG_TAG_FINAL
#ifdef DEC_UWB_SUB

						case RTLS_CAR_REVTOFCARD_MSG:
						{
							uint8 sum=0,i=0,status=0,devtype=0,count=0;
							uint16 cardid;
							sum = rxmsg->messageData[1];
							//PrintfUtil_vPrintf("-----------------------count =%d \n ",sum);
							//count , m_distance(2)/ s_distance(2) / <cardid1, status, dev> (4)/ ......<cardidn, status, dev>(4)/ 
							if(sum)
							{
								count = Car_revcardlist.u8CardCnt;
								
								memcpy(&Car_revcardlist.m_distance,&rxmsg->messageData[2],2);
								memcpy(&Car_revcardlist.s_distance,&rxmsg->messageData[4],2);
								for(i=0;i<sum;i++)
								{
									memcpy(&cardid,&rxmsg->messageData[6+4*i],2);
									memcpy(&Car_revcardlist.cardmsg[count + i].u8cardaddr[0],&rxmsg->messageData[6+4*i],ADDR_BYTE_SIZE);
									status = rxmsg->messageData[8+4*i];
									devtype = rxmsg->messageData[9+4*i];
									Car_revcardlist.cardmsg[count + i].status = status;
									Car_revcardlist.cardmsg[count + i].devtype = devtype;
									instance_set_car_rev();
									PrintfUtil_vPrintf("-id= <%d > ",cardid);
								}
								Car_revcardlist.u8CardCnt +=sum;
								//PrintfUtil_vPrintf("\nmdis =%d  ;sdis=%d\n ",Car_revcardlist.m_distance,Car_revcardlist.s_distance);
							}
							ta_rxe_wait(inst);
						}	
						break;
#endif

                        default:
                        {
#if (DOUBLE_RX_BUFFER == 0)
                            //we got an unexpected function code...
                            //inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
#endif
                            inst->shouldDoDelayedRx = FALSE ;               // no delay turning on RX
                            
							ta_rxe_wait(inst);

                        }
                        break;
                    } //end switch (rxmsg->functionCode)


                    if((inst->instToSleep == 0) && (inst->mode == LISTENER) /*|| (inst->mode == ANCHOR)*/)//update received data, and go back to receiving frames
                    {

                        //instancelogrxdata(inst);
#if (DOUBLE_RX_BUFFER == 0)
                        //inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
#endif
                        inst->shouldDoDelayedRx = FALSE ;               // no delay turning on RX
                        
						ta_rxe_wait(inst);
                    }

                }
                break ;

                case DWT_SIG_RX_TIMEOUT :
                {
				//	PrintfUtil_vPrintf("OT ");
                    inst->rxTimeouts ++ ;
                    inst->done = INST_NOT_DONE_YET;
					instance_set_status(1);
                    if(inst->mode == ANCHOR || inst->mode == SUB_STA) //we did not receive the final - wait for next poll
                    {
                     	if(inst->mode == ANCHOR && inst->previousState == TA_TXPOLL_WAIT_SEND)//&&inst->rxTimeouts <=MAX_NUMBER_OF_POLL_RETRYS)
                 		{
                 			inst->testAppState = TA_TXPOLL_WAIT_SEND ;
								
                 		}
						else
                        {
                        	instance_set_status(0);
                        	if(inst->mode == ANCHOR &&
								inst->previousState == TA_TXFINAL_WAIT_SEND )
                    		{
                    			instance_set_insleep();
								break;
                    		}
                        	inst->rxTimeouts=0;
							//  dwt_forcetrxoff() ;
                            //inst->testAppState = TA_RXE_WAIT ;
                            dwt_setrxtimeout(0);
                            inst->ackexpected = 0 ; //timeout... so no acks expected anymore
                          
							ta_rxe_wait(inst);
							break;
                        }
                    }
					else if(inst->mode == TAG)
					{
						// initiate the re-transmission of the poll that was not responded to
						//    inst->testAppState = TA_TXE_WAIT ;
						//NOTE: Do not go back to "discovery" mode -  needs a boards reset to go back to "discovery" mode once is starts ranging
						uint16 temp;
						if(inst->cur_slot_msg == IDLE)// ||inst->previousState == TA_TXBLINK_WAIT_SEND)
							inst->testAppState = TA_TXBLINK_WAIT_SEND;
						else if(inst->cur_slot_msg == USED_TOF)
						{
							inst->testAppState = TA_TXPOLL_WAIT_SEND ; // TA_TXBLINK_WAIT_SEND;//
							if(inst->previousState == TA_TXBLINK_WAIT_SEND)
								inst->testAppState = TA_TXBLINK_WAIT_SEND;
							else if(inst->previousState ==TA_TXFINAL_WAIT_SEND)
								instance_set_status(0);
						}
					//	mSleep(10);
						if(inst->testAppState == TA_TXBLINK_WAIT_SEND)
						{
							temp =abs( portGetTickCount() -instance_get_slot_starttick());  //+2 send process
							if(EVERY_SLOT_TIME -13 > temp)
							{
								temp=EVERY_SLOT_TIME -13 - temp;
								mSleep(temp);
							}
						}
						dwt_forcetrxoff();
					}

                    message = 0; //clear the message as we have processed the event

                    //timeout - disable the radio (if using SW timeout the rx will not be off)
           //         dwt_forcetrxoff() ;
                }
                break ;

                case DWT_SIG_TX_AA_DONE: //ignore this event - just process the rx frame that was received before the ACK response
                case 0: //no event - wait in receive...
                {
                    //stay in Rx (fall-through from above state)
                    //if(DWT_SIG_TX_AA_DONE == message) printf("Got SIG_TX_AA_DONE in RX wait - ignore\n");
                    if(inst->done == INST_NOT_DONE_YET) inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
                }
                break;

                default :
                {
                    //printf("\nERROR - Unexpected message %d ??\n", message) ;
                    //assert(0) ;                                             // Unexpected Primitive what is going on ?
                }
                break ;

            }
            break ; // end case TA_RX_WAIT_DATA

            default:
                //printf("\nERROR - invalid state %d - what is going on??\n", inst->testAppState) ;
            break;
    } // end switch on testAppState

    return inst->done;
} // end testapprun()

// -------------------------------------------------------------------------------------------------------------------
#if NUM_INST != 1
#error These functions assume one instance only
#else
 


// -------------------------------------------------------------------------------------------------------------------
// Set this instance role as the Tag, Anchor or Listener
void instancesetrole(int inst_mode)
{
    // assume instance 0, for this
    instance_data[0].mode =  (uint8)inst_mode;                   // set the role
}

uint8 instancegetrole(void)
{
    return instance_data[0].mode;
}

int instancenewrange(void)
{
    if(instance_data[0].newrange)
    {
        instance_data[0].newrange = 0;
        return 1;
    }

    return 0;
}

int instancenewCar(void)
{
    if(instance_data[0].car_revcard)
    {
        instance_data[0].car_revcard = 0;
        return 1;
    }

    return 0;
}
// have been rev the car's cards msg list
void instance_set_car_rev(void)
{
	instance_data[0].car_revcard = 1;
}

int instanceoldrange()
{
    if(instance_data[0].oldrange)
    {
        instance_data[0].oldrange = 0;
        return 1;
    }

    return 0;
}

void reset_appstate()
{
	instance_data[0].testAppState=0;
	instance_data[0].previousState=0;
}
// -------------------------------------------------------------------------------------------------------------------
// function to clear counts/averages/range values
//
void instanceclearcounts(void)
{
    int instance = 0 ;

    instance_data[instance].rxTimeouts = 0 ;

    instance_data[instance].frame_sn = 0;
    instance_data[instance].longTermRangeSum  = 0;
    instance_data[instance].longTermRangeCount  = 0;

 //   instance_data[instance].idistmax = 0;
 //   instance_data[instance].idistmin = 1000;

    dwt_configeventcounters(1); //enable and clear

    instance_data[instance].frame_sn = 0;
 //   instance_data[instance].lastReportSN = 0xff;

    instance_data[instance].tofcount = 0 ;
    instance_data[instance].tofindex = 0 ;

#if (DEEP_SLEEP == 1)
    instance_data[instance].txmsgcount = 0;
    instance_data[instance].rxmsgcount = 0;
#endif
} // end instanceclearcounts()


// -------------------------------------------------------------------------------------------------------------------
// function to initialise instance structures
//
// Returns 0 on success and -1 on error
#if DECA_SUPPORT_SOUNDING==1
int instance_init(accBuff_t *buf)
#else
int instance_init(void)
#endif
{
    int instance = 0 ;
    int result;
    //uint16 temp = 0;

    instance_data[instance].shouldDoDelayedRx = FALSE ;

   // instance_data[instance].mode = LISTENER ;                                // assume listener,
    instance_data[instance].testAppState = TA_INIT ;

    instance_data[instance].anchorListIndex = 0 ;
    instance_data[instance].instToSleep = 0;

//    instance_data[instance].sentSN = 0;
//    instance_data[instance].ackdSN = 0;

    instance_data[instance].tofindex = 0;
    instance_data[instance].tofcount = 0;
//    instance_data[instance].last_update = -1 ;           // detect changes to status report

    // Reset the IC (might be needed if not getting here from POWER ON)
    // ARM code: Remove soft reset here as using hard reset in the inittestapplication() in the main.c file
    dwt_softreset();

#if (DEEP_SLEEP_AUTOWAKEUP == 1)
#if 1
    {
        double t;
        instance_data[instance].lp_osc_cal = dwt_calibratesleepcnt(); //calibrate low power oscillator
        //the lp_osc_cal value is number of XTAL/2 cycles in one cycle of LP OSC
        //to convert into seconds (38.4MHz/2 = 19.2MHz (XTAL/2) => 1/19.2MHz ns)
        //so to get a sleep time of 5s we need to program 5 / period and then >> 12 as the register holds upper 16-bits of 28-bit counter
        t = ((double)5/((double)instance_data[instance].lp_osc_cal/19.2e6));
        instance_data[instance].blinktime = (int) t;
        instance_data[instance].blinktime >>= 12;

        dwt_configuresleepcnt(instance_data[instance].blinktime);//configure sleep time

    }
#else
    instance_data[instance].blinktime = 0xf;
#endif
#endif

    //we can enable any configuration loding from NVM/ROM on initialisation
    result = dwt_initialise(DWT_LOADUCODE | DWT_LOADTXCONFIG | DWT_LOADANTDLY| DWT_LOADXTALTRIM) ;

    //temp = dwt_readtempvbat();
    //printf("Vbat = %d (0x%02x) \tVtemp = %d  (0x%02x)\n",temp&0xFF,temp&0xFF,(temp>>8)&0xFF,(temp>>8)&0xFF);

    // if using auto CRC check (DWT_INT_RFCG and DWT_INT_RFCE) are used instead of DWT_INT_RDFR flag
    // other errors which need to be checked (as they disable receiver) are
//    dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | (DWT_INT_ARFE | DWT_INT_RFSL | DWT_INT_SFDT | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO /*| DWT_INT_RXPTO*/), 1);
//	 dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | ( DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO /*| DWT_INT_RXPTO*/), 1);
	dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG   | DWT_INT_RFTO , 1);
	//this is platform dependant - only program if DW EVK/EVB
    dwt_setleds(2) ; //configure the GPIOs which control the leds on EVBs

    //this is set though the instance_data[instance].configData.smartPowerEn in the instance_config function
    //dwt_setsmarttxpower(0); //disable smart TX power


    if (DWT_SUCCESS != result)
    {
        return (-1) ;   // device initialise has failed
    }

    dwt_setcallbacks(instance_txcallback, instance_rxcallback);

    instanceclearcounts() ;

    instance_data[instance].panid = 0xdeca ;

#if (DR_DISCOVERY == 1)
    instance_data[instance].sendTOFR2Tag = 1;
#else
    instance_data[instance].sendTOFR2Tag = 0;
#endif

    instance_data[instance].tag2rxReport = 0; //Tag is not expecting the report

    instance_data[instance].fixedReportDelay = (convertmicrosectodevicetimeu (FIXED_REPORT_DELAY * 1000.0) & MASK_TXDTS);
    instance_data[instance].fixedReplyDelay = (convertmicrosectodevicetimeu (FIXED_REPLY_DELAY * 1000.0) & MASK_TXDTS);
    instance_data[instance].fixedReplyDelay_ms = FIXED_REPLY_DELAY ;
    instance_data[instance].rxOnDelay = convertmicrosectodevicetimeu ((FIXED_REPLY_DELAY - RX_ON_TIME) * 1000.0);
	instance_data[instance].tagBlinkSleepTime_us = BLINK_SLEEP_DELAY*1000 ;

    instance_data[instance].newReportSent = 0; //clear the flag
    instance_data[instance].wait4ack = 0;
    instance_data[instance].ackexpected = 0;
    instance_data[instance].stoptimer = 0;
    instance_data[instance].instancetimer_en = 0;

	instance_data[instance].slottimer = portGetTickCount();

    instance_data[instance].dwevent[0] = 0;
    instance_data[instance].dwevent[1] = 0;
    instance_data[instance].dweventCnt = 0;

    instance_data[instance].anchReportTimeout_us = 10*1000 ; //10 ms

    instance_data[instance].rxautoreenable = 0;

    //sample test calibration functions
    //xtalcalibration();
    //powertest();
    instance_data[instance].seqnum = 0;
//	instance_data[instance].is_intoflist = 0;
	instance_data[instance].slotlist_full = FALSE;
	instance_data[instance].up_revrpoll_time =0;
	instance_data[instance].have_rev_pollOblink =0;
	instance_data[instance].change_devtype =0 ;
	instance_data[instance].new_inblink =0;
	instance_data[instance].shouldrepvbat =0;
	instance_data[instance].tdoarepvbat =0;
	dwt_geteui(instance_data[instance].eui64);

	u16ShortAddr = instance_get_cardid();
	new_tof_in =0;
#ifdef DEC_UWB_TAG
	init_tag(&instance_data[instance]);

#else
	init_anchor(&instance_data[instance]);
#endif

    return 0 ;
}

int reinit_dw1000()
{
	int result;
	dwt_softreset();

#if (DEEP_SLEEP_AUTOWAKEUP == 1)
#if 1
	{
		double t;
		instance_data[instance].lp_osc_cal = dwt_calibratesleepcnt(); //calibrate low power oscillator
		//the lp_osc_cal value is number of XTAL/2 cycles in one cycle of LP OSC
		//to convert into seconds (38.4MHz/2 = 19.2MHz (XTAL/2) => 1/19.2MHz ns)
		//so to get a sleep time of 5s we need to program 5 / period and then >> 12 as the register holds upper 16-bits of 28-bit counter
		t = ((double)5/((double)instance_data[instance].lp_osc_cal/19.2e6));
		instance_data[instance].blinktime = (int) t;
		instance_data[instance].blinktime >>= 12;

		dwt_configuresleepcnt(instance_data[instance].blinktime);//configure sleep time

	}
#else
	instance_data[instance].blinktime = 0xf;
#endif
#endif

	//we can enable any configuration loding from NVM/ROM on initialisation
	result = dwt_initialise(DWT_LOADUCODE | DWT_LOADTXCONFIG | DWT_LOADANTDLY| DWT_LOADXTALTRIM) ;

	//temp = dwt_readtempvbat();
	//printf("Vbat = %d (0x%02x) \tVtemp = %d  (0x%02x)\n",temp&0xFF,temp&0xFF,(temp>>8)&0xFF,(temp>>8)&0xFF);

	// if using auto CRC check (DWT_INT_RFCG and DWT_INT_RFCE) are used instead of DWT_INT_RDFR flag
	// other errors which need to be checked (as they disable receiver) are
//	  dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | (DWT_INT_ARFE | DWT_INT_RFSL | DWT_INT_SFDT | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO /*| DWT_INT_RXPTO*/), 1);
//	 dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | ( DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO /*| DWT_INT_RXPTO*/), 1);
	dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG   | DWT_INT_RFTO , 1);
	//this is platform dependant - only program if DW EVK/EVB
	dwt_setleds(2) ; //configure the GPIOs which control the leds on EVBs

	//this is set though the instance_data[instance].configData.smartPowerEn in the instance_config function
	//dwt_setsmarttxpower(0); //disable smart TX power


	if (DWT_SUCCESS != result)
	{
		return (-1) ;	// device initialise has failed
	}

	dwt_setcallbacks(instance_txcallback, instance_rxcallback);

	instanceclearcounts() ;

	//sample test calibration functions
	//xtalcalibration();
	//powertest();
	dwt_geteui(instance_data[0].eui64);   //if not set this ,the dw1000 will be die

	//u16ShortAddr = instance_get_cardid();
	//new_tof_in =0;
	init_anchor(&instance_data[0]);


}


// -------------------------------------------------------------------------------------------------------------------
//
// Return the Device ID register value, enables higher level validation of physical device presence
//

uint32 instancereaddeviceid(void)
{
    return dwt_readdevid() ;
}


// -------------------------------------------------------------------------------------------------------------------
//
// function to allow application configuration be passed into instance and affect underlying device opetation
//
void instance_config(instanceConfig_t *config)
{
    int instance = 0 ;
    int use_nvmdata = DWT_LOADANTDLY | DWT_LOADXTALTRIM;
    uint32 power = 0;

    instance_data[instance].configData.chan = config->channelNumber ;
    instance_data[instance].configData.rxCode =  config->preambleCode ;
    instance_data[instance].configData.txCode = config->preambleCode ;
    instance_data[instance].configData.prf = config->pulseRepFreq ;
    instance_data[instance].configData.dataRate = config->dataRate ;
    instance_data[instance].configData.txPreambLength = config->preambleLen ;
    instance_data[instance].configData.rxPAC = config->pacSize ;
    instance_data[instance].configData.nsSFD = config->nsSFD ;
    instance_data[instance].configData.phrMode = DWT_PHRMODE_STD ;
    instance_data[instance].configData.sfdTO = DWT_SFDTOC_DEF; //default value

    instance_data[instance].configData.smartPowerEn = 0;

    //configure the channel parameters
    dwt_configure(&instance_data[instance].configData, use_nvmdata) ;

    instance_data[instance].configTX.PGdly = txSpectrumConfig[config->channelNumber].PGdelay ;

	//firstly check if there are calibrated TX power value in the DW1000 OTP
	power = dwt_getotptxpower(config->pulseRepFreq, instance_data[instance].configData.chan);

	if((power == 0x0) || (power == 0xFFFFFFFF)) //if there are no calibrated values... need to use defaults
    {
        power = txSpectrumConfig[config->channelNumber].txPwr[config->pulseRepFreq- DWT_PRF_16M];
    }

    //Configure TX power
	//if smart power is used then the value as read from NVM is used directly
	//if smart power is used the user needs to make sure to transmit only one frame per 1ms or TX spectrum power will be violated
    if(instance_data[instance].configData.smartPowerEn == 1)
    {
        instance_data[instance].configTX.power = power;
    }
	else //if the smart power if not used, then the low byte value (repeated) is used for the whole TX power register
    {
        uint8 pow = power & 0xFF ;
        instance_data[instance].configTX.power = (pow | (pow << 8) | (pow << 16) | (pow << 24));
    }
	instance_data[instance].configTX.power = 0x75898975;          //双功放使用
	dwt_setsmarttxpower(instance_data[instance].configData.smartPowerEn);

	//configure the tx spectrum parameters (power and PG delay)
    dwt_configuretxrf(&instance_data[instance].configTX);

	//check if to use the antenna delay calibration values as read from the NVM
    if((use_nvmdata & DWT_LOADANTDLY) == 0)
    {
        instance_data[instance].txantennaDelay = rfDelays[config->pulseRepFreq - DWT_PRF_16M];
        // -------------------------------------------------------------------------------------------------------------------
        // set the antenna delay, we assume that the RX is the same as TX.
        dwt_setrxantennadelay(instance_data[instance].txantennaDelay);
        dwt_settxantennadelay(instance_data[instance].txantennaDelay);
    }
    else
    {
        //get the antenna delay that was read from the OTP calibration area
        instance_data[instance].txantennaDelay = dwt_readantennadelay(config->pulseRepFreq) >> 1;

        // if nothing was actually programmed then set a reasonable value anyway
		if (instance_data[instance].txantennaDelay == 0)
		{
			instance_data[instance].txantennaDelay = rfDelays[config->pulseRepFreq - DWT_PRF_16M];
			// -------------------------------------------------------------------------------------------------------------------
			// set the antenna delay, we assume that the RX is the same as TX.
			dwt_setrxantennadelay(instance_data[instance].txantennaDelay);
			dwt_settxantennadelay(instance_data[instance].txantennaDelay);
		}


    }

	tx_antennaDelay = instance_data[instance].txantennaDelay;
//	instance_change_channel(2);
}


void instance_change_channel(uint8 channel)
{
	 int instance = 0 ;
	 int use_nvmdata = DWT_LOADANTDLY | DWT_LOADXTALTRIM;
    uint32 power = 0;
	uint8 channel1=channel;
    instance_data[instance].configData.chan = channel1 ;

    //configure the channel parameters
    dwt_configure(&instance_data[instance].configData, use_nvmdata) ;

    instance_data[instance].configTX.PGdly = txSpectrumConfig[channel].PGdelay ;

	//firstly check if there are calibrated TX power value in the DW1000 OTP
	power = dwt_getotptxpower(DWT_PRF_16M, instance_data[instance].configData.chan);
	
	if((power == 0x0) || (power == 0xFFFFFFFF)) //if there are no calibrated values... need to use defaults
	{
		power = txSpectrumConfig[channel].txPwr[DWT_PRF_16M- DWT_PRF_16M];
	}

	//Configure TX power
	//if smart power is used then the value as read from NVM is used directly
	//if smart power is used the user needs to make sure to transmit only one frame per 1ms or TX spectrum power will be violated
	if(instance_data[instance].configData.smartPowerEn == 1)
	{
		instance_data[instance].configTX.power = power;
	}
	else //if the smart power if not used, then the low byte value (repeated) is used for the whole TX power register
	{
		uint8 pow = power & 0xFF ;
		instance_data[instance].configTX.power = (pow | (pow << 8) | (pow << 16) | (pow << 24));
	}
	if(instance_data[instance].mode == ANCHOR)
	{
		if(channel==ANCHOR_TOF_CHANNEL)
			instance_data[instance].configTX.power = 0x75959575;     //16.5db
		else if(channel==ANCHOR_BLINK_CHANNEL)
			instance_data[instance].configTX.power = 0x75999975;     //18.5db
	}
	else
		instance_data[instance].configTX.power = Station_Power;         //10.5db
	dwt_setsmarttxpower(instance_data[instance].configData.smartPowerEn);

	//configure the tx spectrum parameters (power and PG delay)
	dwt_configuretxrf(&instance_data[instance].configTX);

	//check if to use the antenna delay calibration values as read from the NVM
	if((use_nvmdata & DWT_LOADANTDLY) == 0)
	{
		instance_data[instance].txantennaDelay = rfDelays[DWT_PRF_16M - DWT_PRF_16M];
		// -------------------------------------------------------------------------------------------------------------------
		// set the antenna delay, we assume that the RX is the same as TX.
		dwt_setrxantennadelay(instance_data[instance].txantennaDelay);
		dwt_settxantennadelay(instance_data[instance].txantennaDelay);
	}
	else
	{
		//get the antenna delay that was read from the OTP calibration area
		instance_data[instance].txantennaDelay = dwt_readantennadelay(DWT_PRF_16M) >> 1;

		// if nothing was actually programmed then set a reasonable value anyway
		if (instance_data[instance].txantennaDelay == 0)
		{
			instance_data[instance].txantennaDelay = rfDelays[DWT_PRF_16M - DWT_PRF_16M];
			// -------------------------------------------------------------------------------------------------------------------
			// set the antenna delay, we assume that the RX is the same as TX.
			dwt_setrxantennadelay(instance_data[instance].txantennaDelay);
			dwt_settxantennadelay(instance_data[instance].txantennaDelay);
		}


	}

	tx_antennaDelay = instance_data[instance].txantennaDelay;

}

// -------------------------------------------------------------------------------------------------------------------
// function to set the tag sleep time (in ms)
//
void instancesettagsleepdelay(uint32 sleepdelay) //sleep in ms
{
    int instance = 0 ;
    instance_data[instance].tagSleepTime_us = sleepdelay*((uint32)1000) ;
}

// -------------------------------------------------------------------------------------------------------------------
// function to set the fixed blink reply delay time (in ms)
//
void instancesetblinkreplydelay(double delayms) //delay in ms
{
	int instance = 0 ;
	instance_data[instance].fixedReplyDelay_ms = delayms ;
}
// -------------------------------------------------------------------------------------------------------------------
// function to set the fixed reply delay time (in ms)
//
void instancesetreplydelay(double delayms) //delay in ms
{
    int instance = 0 ;
    instance_data[instance].fixedReplyDelay = convertmicrosectodevicetimeu (delayms * 1e3) ;
    instance_data[instance].fixedReplyDelay_ms = delayms ;
    instance_data[instance].rxOnDelay = convertmicrosectodevicetimeu ((delayms - RX_ON_TIME) * 1e3);
    //printf("Set response delay time to %d ms.\n", (int) delayms);
}

// -------------------------------------------------------------------------------------------------------------------
// function to configure anchor instance whether to send TOF reports to Tag
//
void instancesetreporting(int anchorSendsTofReports)
{
    int instance = 0 ;
    instance_data[instance].sendTOFR2Tag = anchorSendsTofReports ;        // Set whether TOF reports are sent
    //Listener will only listen for reports when this is set (all other frames are filtered out)
    //instance_data[instance].listen4Reports = anchorSendsTofReports ;
}

#if (DR_DISCOVERY == 0)
// -------------------------------------------------------------------------------------------------------------------
//
// Set Payload parameters for the instance
//
// -------------------------------------------------------------------------------------------------------------------
void instancesetaddresses(instanceAddressConfig_t *plconfig)
{
    int instance = 0 ;

    instance_data[instance].payload = *plconfig ;       // copy configurations

    if(instance_data[instance].payload.sendReport == 1)
        instance_data[instance].sendTOFR2Tag = 1;
    else
        instance_data[instance].sendTOFR2Tag = 0;
}
#endif

// -------------------------------------------------------------------------------------------------------------------
//
// Access for low level debug
//
// -------------------------------------------------------------------------------------------------------------------


void instance_close(void)
{
#ifdef _MSC_VER
    uint8 buffer[1500];
    // close/tidy up any device functionality - i.e. wake it up if in sleep mode
    if(dwt_spicswakeup(buffer, 1500) == DWT_ERROR)
    {
        //printf("FAILED to WAKE UP\n");
    }
#else
    //wake up device from low power mode
    //NOTE - in the ARM  code just drop chip select for 200us
    port_SPIx_clear_chip_select();  //CS low
    Sleep(1);   //200 us to wake up then waits 5ms for DW1000 XTAL to stabilise
    port_SPIx_set_chip_select();  //CS high
    Sleep(5);
#endif

    dwt_entersleepaftertx(0); // clear the "enter deep sleep after tx" bit

    dwt_setinterrupt(0xFFFFFFFF, 0); //don't allow any interrupts

}


void instance_txcallback(const dwt_callback_data_t *txd)
{
    int instance = 0;
    uint8 txTimeStamp[5] = {0, 0, 0, 0, 0},buff[50];
    uint32 temp = 0;
    uint8 txevent = txd->event;
	if(instance_data[instance].previousState== TA_TDOA_WAIT_SEND)
		return;      //if don't do this will die
	instance_set_status(1);
    if(instance_data[instance].ackreq) //the ACK has been requested in the last RX frame - we got TX event, this means the ACK has been sent
    {
        txevent = DWT_SIG_TX_AA_DONE;
        instance_data[instance].ackreq = 0;
    }
	dwt_readrxdata((uint8 *)&buff[0], txd->datalength, 0); 
    if(txevent == DWT_SIG_TX_DONE)
    {
        //uint64 txtimestamp = 0;

        if(instance_data[instance].dweventCnt < 2) //if no outstanding event to process
        {
            //NOTE - we can only get TX good (done) while here
            //dwt_readtxtimestamp((uint8*) &instance_data[instance].txu.txTimeStamp);

            dwt_readtxtimestamp(txTimeStamp) ;
            temp = txTimeStamp[0] + (txTimeStamp[1] << 8) + (txTimeStamp[2] << 16) + (txTimeStamp[3] << 24);
            instance_data[instance].txu.txTimeStamp = txTimeStamp[4];
            instance_data[instance].txu.txTimeStamp <<= 32;
            instance_data[instance].txu.txTimeStamp += temp;

            instance_data[instance].stoptimer = 0;

            instance_data[instance].dwevent[instance_data[instance].dweventCnt++] = DWT_SIG_TX_DONE ;
        }

#if (DEEP_SLEEP == 1)
        instance_data[instance].txmsgcount++;
#endif
		ta_tx_wait_conf(&instance_data[instance],DWT_SIG_TX_DONE);
    }
    else if(txevent == DWT_SIG_TX_AA_DONE)
    {
        if(instance_data[instance].dweventCnt < 2) //if no outstanding event to process
		{
			//auto ACK confirmation
			instance_data[instance].dwevent[instance_data[instance].dweventCnt++] = DWT_SIG_TX_AA_DONE;
		}
	}

	if(txd->datalength >= 127) //full ,error
	{
		dwt_forcetrxoff() ;
		dwt_rxenable(0) ;
		NVIC_SystemReset(); 
	}
}

void instance_rxcallback(const dwt_callback_data_t *rxd)
{
    int instance = 0;
    uint8 rxTimeStamp[5]  = {0, 0, 0, 0, 0};
    uint32 temp = 0;
    uint8 rxd_event = 0;
    int bufferfull = 0;
	dwt_forcetrxoff() ;

	instance_set_status(1);
    if(rxd->event == DWT_SIG_RX_OKAY)
    {
        uint8 buffer[2];

        instance_data[instance].ackreq = rxd->aatset;
        dwt_readrxdata(buffer, 1, 0);  // Read Data Frame
        //at the moment using length and 1st byte to distinguish between different fame types and "blinks".
        switch(rxd->datalength)
        {
            case SIG_RX_ACK:
                rxd_event = SIG_RX_ACK;
                break;
 
            default:
                rxd_event = DWT_SIG_RX_OKAY;
                break;
        }
		
    	if(rxd_event == DWT_SIG_RX_OKAY)
	    {
	        if(instance_data[instance].dweventCnt < 2) //if no outstanding events to process
	        {
	            //dwt_readrxtimestamp((uint8*) &instance_data[instance].rxu.rxTimeStamp) ;

				dwt_readrxtimestamp(rxTimeStamp) ;
				temp =  rxTimeStamp[0] + (rxTimeStamp[1] << 8) + (rxTimeStamp[2] << 16) + (rxTimeStamp[3] << 24);
				instance_data[instance].rxu.rxTimeStamp = rxTimeStamp[4];
				instance_data[instance].rxu.rxTimeStamp <<= 32;
				instance_data[instance].rxu.rxTimeStamp += temp;

				instance_data[instance].rxLength = rxd->datalength;

				dwt_readrxdata((uint8 *)&instance_data[instance].rxmsg, rxd->datalength, 0);  // Read Data Frame

				//instance_readaccumulatordata();     // for diagnostic display in DecaRanging PC window
				
				instance_data[instance].stoptimer = 1;

				instance_data[instance].dwevent[instance_data[instance].dweventCnt++] = DWT_SIG_RX_OKAY;
				instance_data[instance].testAppState = TA_RX_WAIT_DATA;   // let this state handle it
				#if DECA_LOG_ENABLE==1
				#if DECA_KEEP_ACCUMULATOR==1
				{
					instance_data[instance].newAccumData = 1;
					instance_data[instance].erroredFrame = DWT_SIG_RX_NOERR;   //no error
					processSoundingData();
				}
				#endif
					logSoundingData(DWT_SIG_RX_NOERR);
				#endif
				
				//printf(" %d ", instance_data[instance].testAppState);
				//printf("RX time %f\n",convertdevicetimetosecu(instance_data[instance].rxu.rxTimeStamp));
	        }
	        else
	        {
	        	bufferfull = 1;
	        }
			rx_time++;

#if (DEEP_SLEEP == 1)
	        instance_data[instance].rxmsgcount++;
#endif
	    }

	}
    else if (rxd->event == DWT_SIG_RX_TIMEOUT)
    {
        if(instance_data[instance].dweventCnt < 2) //if no outstanding events to process
        {
            instance_data[instance].dwevent[instance_data[instance].dweventCnt++] = DWT_SIG_RX_TIMEOUT;
            //printf("RX timeout while in %d count %d\n", instance_data[instance].testAppState, instance_data[instance].eventCnt);
        }
        else
        {
        	bufferfull = 8;
        }
		//if(instance_data[instance].testAppState == TA_TX_WAIT_CONF )
		//	ta_tx_wait_conf(&instance_data[instance],DWT_SIG_RX_TIMEOUT);
			//printf("RX timeout ignored !!! %d (count %d) \n", instance_data[instance].testAppState, instance_data[instance].eventCnt);
    }
    else //assume other events are errors
    {
        //printf("RX error %d \n", instance_data[instance].testAppState);
        if(instance_data[instance].rxautoreenable == 0)
        {
            //re-enable the receiver
            instancerxon(0, 0); //immediate enable

        }

    }

    if(bufferfull > 0) //buffer full re-enable receiver
    {
    	bufferfull = 10;

#if (DOUBLE_RX_BUFFER == 0)
    	dwt_forcetrxoff() ;
    	dwt_rxenable(0) ;
#endif

    }
//	PrintfUtil_vPrintf("RX OK< %x >",instance_data[instance].rxmsg.messageData[0]);
}


// -------------------------------------------------------------------------------------------------------------------
double instance_get_ldist(void) //get long term average range
{
    double x = inst_ldist;

    return (x);
}

int instance_get_lcount(void) //get count of ranges used for calculation of lt avg
{
    int x = instance_data[0].longTermRangeCount;

    return (x);
}

double instance_get_idist(void) //get instantaneous range
{
    double x = inst_idist;

    return (x);
}

double instance_get_m_idist(void) //get instantaneous range
{
    double x = inst_m_idist;

    return (x);
}


int instance_get_rxf(void) //get number of Rxed frames
{
    int x = instance_data[0].rxmsgcount;

    return (x);
}

int instance_get_txf(void) //get number of Txed frames
{
    int x = instance_data[0].txmsgcount;

    return (x);
}


double instance_get_adist(void) //get average range
{
    double x = inst_adist;

    return (x);
}

uint16 instance_get_seqnum(void) //get seqnum
{
    
	if(instance_data[0].mode == ANCHOR )
		return my_cardslotmsg.u16SeqNum;
	else
		return instance_data[0].seqnum;
}

void instance_set_seqnum(void) //set seqnum
{
    instance_data[0].seqnum++;
	
}

uint8 instance_get_helpstatus(void)
{
	uint8 x;// =  helpreportoff ;
	if(my_cardslotmsg.status & UWB_CARD_STATUS_HELP)
		x = 0;
	else 
		x =1;
	return (x);

}

uint8 instance_get_retreatstatus(void)
{
	uint8 x;// =  helpreportoff ;
	if(my_cardslotmsg.status & UWB_CARD_STATUS_RETREAT)
	{
		x = 1;
		my_cardslotmsg.status &= ~UWB_CARD_STATUS_RETREAT_ACK;
	}
	else 
		x =0;
	return (x);

}


void instance_reset_helpstatus(void)
{
//	instance_set_AnchorPanid((uint8)ANCHOR_BLINK_PRO); //go to blink process's panid
	helpreportoff = 0;

}

uint8 instance_rev_poll(void)  //this slot the card ever rev poll or blink ,1:yes  0:no
{
	uint8 x = instance_data[0].have_rev_pollOblink;
	instance_data[0].have_rev_pollOblink =0;
	return (x);
}

void instance_set_idle(void) //set card idle status
{
    my_cardslotmsg.b1Used = IDLE;
//	my_cardslotmsg.u16SeqNum =0;
	my_cardslotmsg.u8cur_slot =0;
	my_cardslotmsg.u8LostNum =0;
	my_cardslotmsg.m_distance =0;
	instance_set_AnchorPanid((uint8)ANCHOR_BLINK_PRO);
	new_tof_in =0;
	clear_inblinkmsg();
}

uint16 instance_get_sleeptick()
{
	uint16 x = my_cardslotmsg.sleeptick;
	return (x);
}

uint16 instance_get_ifrevsig(void)
{
	uint16 x=instance_data[0].up_revrpoll_time ;
	return (x);
}

uint16 instance_get_uptick(void)
{
	uint16 x=instance_data[0].up_revrpoll_time ;
	instance_data[0].up_revrpoll_time=0;
	if(new_inblinkmsg.status ==1)   //use the time when it do in bink,or will have some wrong
		x= inblink_uptime;

	return (x);
}

int instance_get_anchoraddr(void)
{
	uint64 x = instance_data[0].get_anchoraddr ;
	int temp = x%16;
	return (temp);
}

int instance_get_status(void)
{
	int x = instance_data[0].is_newstatus;
	instance_data[0].is_newstatus =0;
	return (x);
}

void instance_set_status(int eventstatus)
{
	instance_data[0].is_newstatus = eventstatus;
}

void instance_set_event(int type)
{
	if(type ==0)
	{
		instance_data[0].testAppState = TA_TXBLINK_WAIT_SEND;
		instance_data[0].cur_slot_msg =IDLE;
	}
	else if(type ==1)
	{
		instance_data[0].testAppState = TA_TXPOLL_WAIT_SEND;
		instance_data[0].cur_slot_msg =USED_TOF;
	}
	else if(type ==2)
		instance_data[0].testAppState = TA_RX_WAIT_DATA;
}


void instance_set_card_status(uint8 cardstatus)        
{
	instance_data[0].cardstatus = cardstatus;
}

uint8 instance_get_card_status(void)        
{
	uint8 x = instance_data[0].cardstatus;
	return (x);
}

void instance_set_slot_starttick(uint32 tick)    //use to caculate the tick the station have run
{
	instance_data[0].slot_starttick = tick;
	slotfirstinpoll = 1;   //every slot wake up, this set
	rx_time =0;
}

uint32 instance_get_slot_starttick(void)
{
	uint32 x = instance_data[0].slot_starttick;
	return (x);

}

void instance_set_vbat(uint16 vdd)
{
	instance_data[0].cardbattery = vdd;
	instance_data[0].shouldrepvbat =1;
	instance_data[0].tdoarepvbat =1;
}

void instance_change_devtype(void)
{
	instance_data[0].change_devtype =1 ;
}

uint8 instance_getchange_devtype(void)
{
	uint8 x = instance_data[0].change_devtype ;
	instance_data[0].change_devtype =0;
	return x;
}

void instance_set_revpolltype(uint8 type)
{
//	PrintfUtil_vPrintf("-type =%d-  !",type);
	instance_data[0].revpolltype = type;
}

uint8 instance_get_revpolltype(void)
{
	uint8 x = instance_data[0].revpolltype ;
	instance_data[0].revpolltype =0;
	return x;
}

int8 instance_get_rssi(void)
{
	uint8 rssi;
	rssi= instance_data[0].i8rssi;
	return rssi;
}

void instance_set_helpexcit(uint8 type)
{
	if(type ==1)
		instance_data[0].help_excit_send &= 0x10;
	else if(type ==2)
		instance_data[0].help_excit_send &= 0x01;
}

uint8 instance_get_inblinkmsg(void)
{
	uint8 x = instance_data[0].new_inblink;
	instance_data[0].new_inblink=0;
	return x;
}

int8 instance_get_powerlever(void)
{
	int8 x = instance_data[0].i8rssi;
	instance_data[0].i8rssi=0;
	return x;

}
int instance_readaccumulatordata(void)
{
#if DECA_SUPPORT_SOUNDING==1
    int instance = 0;
    uint16 len = 992 ; //default (16M prf)

    if (instance_data[instance].configData.prf == DWT_PRF_64M)  // Figure out length to read
        len = 1016 ;

    instance_data[instance].buff.accumLength = len ;                                       // remember Length, then read the accumulator data

    len = len*4+1 ;   // extra 1 as first byte is dummy due to internal memory access delay

    dwt_readaccdata((uint8*)&(instance_data[instance].buff.accumData->dummy), len, 0);
#endif  // support_sounding
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------


int instance_run(uint8 type)
{
    int instance = 0 ;
    //int done = INST_NOT_DONE_YET;
    int message = instance_data[instance].dwevent[0];

    if(type ==0)
    {
		testapprun(&instance_data[instance], message);//,slotmsg); 
	}
	else
	{
		ta_rxe_wait(&instance_data[instance]);    //anchor wake up enter the rx status
			
	}


	if(message) // there was an event in the buffer
	{
		instance_data[instance].dwevent[0] = 0; //clear the buffer
		instance_data[instance].dweventCnt--;
		//printf("process event %d in (%d) ecount %d\n", message, state, instance_data[instance].dweventCnt);

		if(instance_data[instance].dwevent[1]) // there is another event in the buffer move it to front
		{
			instance_data[instance].dwevent[0] = instance_data[instance].dwevent[1];
			instance_data[instance].dwevent[1] = 0; //clear the buffer
			//instance_data[instance].dweventCnt = 1;
		}
	}
	instance_data[instance].dwevent[1] = 0; //clear the buffer

	//we've processed message
	message = 0;


    return 0 ;
}

#endif


/* ==========================================================

Notes:

Previously code handled multiple instances in a single console application

Now have changed it to do a single instance only. With minimal code changes...(i.e. kept [instance] index but it is always 0.

Windows application should call instance_init() once and then in the "main loop" call instance_run().

*/
