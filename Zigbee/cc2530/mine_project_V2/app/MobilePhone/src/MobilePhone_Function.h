/**************************************************************************************************
Filename:       Mobile_Fuction.h
Revised:        $Date: 2011/04/01 22:15:59 $
Revision:       $Revision: 1.2 $

Description:    A Mine Application task except voice function of mobile phone.
**************************************************************************************************/

#ifndef MP_MP_FUCTION_H
#define MP_MP_FUCTION_H

#include "hal_types.h"

/*************************************************************************************************
*CONSTANTS
*/
extern   uint8 MP_Function_TaskID;
extern   uint16 timeSyncCnt ;
#ifdef CFG_STATION_SIMULATE
extern   uint16 V2SimulateStationID;
#endif
#ifdef CFG_TEST_WIRELESS
extern   uint16 testDevID;
extern   uint16 recCount;
extern   uint16 sentSeq;
extern   uint16 dstRecCount;
extern   uint16 recErrCount;
extern   uint8  handleSeq;
#endif

#if defined(CFG_STATION_SIMULATE)||defined(CFG_TEST_WIRELESS)

#include "app_protocol.h"
// send signal in common channel, for AVAILABLE, IDLE, RSSI, BUSY cmd
typedef struct
{
	app_header_t tof_head;

	uint8 u8StationStatus; 	// tof_station_status_te
	uint8 u8AvailableMs;		// the available time for card to send join request (card use this to generate a random number)
	uint16 u16CurSlot;		// station's curSlot

	//fixme, this is not a 4-Byte struct
	uint8 u8RunMs;			// station run ms in this slot
	uint8 u8LocIdle; 			// 1: has idle loc slot, 0: has not
} rf_tof_station_signal_ts;

typedef union
{
	app_header_t tof_head;
	rf_tof_station_signal_ts rf_tof_station_signal;
}RfTofWrapper_tu;
#endif


/*************************************************************************************************
*MACROS
*/
#define MP_FUNC_UPDATE_EVENT                    0x0001
#define MP_FUNC_MENULIB_EVENT                   0x0002
#define MP_FUNC_RESET_EVENT			            0x0004
#define MP_FUNC_CONTINUESPRESS_TEST_EVENT       0x0008
#define MP_FUNC_PERIODIC_SLEEP_EVENT            0x0010
#define MP_FUNC_SCANPREP_EVENT                  0x0020
#define MP_FUNC_PERIODIC_SCAN_EVENT             0x0040  // periodically do channel scan to get nearby stations
#define MP_FUNC_PERIODIC_SCANEND_EVENT          0x0080  // periodically do channel scan to get nearby stations
#define MP_FUNC_POLL_EVENT            	        0x0100       // must be done after channel scan finish
#define MP_FUNC_JOIN_SWITCH_EVENT               0x0200       // must be done after channel scan finish
#ifdef CFG_STATION_SIMULATE
#define MP_FUNC_SIMULATESTATION_EVENT           0x0400
#endif
#ifdef CFG_TEST_WIRELESS
#define MP_FUNC_TESTWIRELESS_EVENT              0x0800
#endif
/*********************************************************************
* FUNCTIONS
*/
void MP_Function_Init( uint8 task_id );
uint16 MP_Function_ProcessEvent( uint8 task_id, uint16 events);
void MP_StartMenuLibEvt (uint16 timeout);
void MP_StopMenuLibEvt (void);

#endif

