/**************************************************************************************************
Filename:       MobilePhone_cfg.h
Revised:        $Date: 2011/05/13 23:40:24 $
Revision:       $Revision: 1.3 $

**************************************************************************************************/

#ifndef MOBILEPHONE_CFG_H
#define MOBILEPHONE_CFG_H

/**************************************************************************************************
*                                        User's  Defines
**************************************************************************************************/

#define MP_MAC_BEACON_ORDER      15            /* Setting beacon order to 15 will disable the beacon */
#define MP_MAC_SUPERFRAME_ORDER  15            /* Setting superframe order to 15 will disable the superframe */

#define MP_KEY_INT_ENABLED       TRUE         
/*
* FALSE = Key Polling
* TRUE  = Key interrupt
*
* Notes: Key interrupt will not work well with 2430 EB because
*        all the operations using up/down/left/right switch will
*        no longer work. Normally S1 + up/down/left/right is used
*        to invoke the switches but on the 2430 EB board,  the
*        GPIO for S1 is used by the LCD.
*/

/* secharch nwk timeout */
#define MP_INIT_NWK_TIMEOUT            20000

/* persist audio timeout */
#define MP_STOP_AUDIO_TIMEOUT 	3000


#define MP_POWER_LONGPRESS_TIMEOUT      1000

/* Maximun number of scan result that will be accepted */
#define MP_MAC_MAXSCAN_RESULTS      	   5              

/* mobile sleep interval */
#define MP_WORK_INTERVAL		3000

/* work time after one poll*/
#define MP_WORK_TIMEOUT 		300 

/*spcetial short address */
#define MP_SHORT_GATEWAYNMBR 0
#define MP_SHORT_INVALIDNMMBR 0xFFFF

/* cell switche params, threhold in RSSI  */
#define MP_CELL_THREHOLD1   (-35)
#define MP_CELL_THREHOLD2   (-60)
#define MP_CELL_COMPTIME        2
#define MP_CELL_DIFFRSSI	    6     	 

#define MP_REJOINTIMES				3
#define MP_NWKTOLERANCE_TIME		8

/* voice defination*/
#define VOICE_PER_RAW_DATA_LEN AMBE_RAWDATA_LEN_2400
#define VOICE_IDX_THRESHOLD  12

//#define MOBILEPHONE_NWK_ADDR		    0xFFF4

/* cmd retry times */
#define MP_SIGNAL_RETRY_TIME   3000 
#define MP_HANG_UP_WAIT_TIME   5000 


#define MP_MIN_RSSI -120

#endif  /* MOBILEPHONE_CFG_H */
