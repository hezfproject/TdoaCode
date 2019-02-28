/************************************************************************/
/* Charge card configs                                                   */
/************************************************************************/

#ifndef CHARGEED_CFG_H
#define CHARGEED_CFG_H


/************************************************************************/
/* defines                                                              */
/************************************************************************/

/* Channel */

#define CHARGEED_SYNC_WINLEN      	  35         		/* 15 ms for one sync window*/
#define CHARGEED_MIN_SYNCNUM 		 2				/* min sync  locnodes number */
#define CHARGEED_MAX_SYNCNUM 	 	2				/* max sync  locnodes number */

#define CHARGEED_SYNC_INTERVAL 	 	   1				/* 6 ms sync one time */
#define CHARGEED_RETREATSUPPRESS_TIME	10 	 	/* 10 minutes */

#define CHARGEED_HEALTH_MAX 		  	  3		
#define CHARGEED_MAX_BLAST_FAIL 		  5		

#define  CHARGEED_KEY_LONGPRESS_TIME 		1500           /* Key press time in ms */

#define  CHARGEED_LED_FLASH_TIME_INIT		3000   	 /* Led flash time when inital*/
#define  CHARGEED_LED_FLASH_TIME_LOWBATTERY		300      /* Led flash time when no power*/

#define  CHARGEED_MAC_BEACON_ORDER		15                /* Setting beacon order to 15 will disable the beacon */
#define  CHARGEED_MAC_SUPERFRAME_ORDER	15            /* Setting superframe order to 15 will disable the superframe */
#define  CHARGEED_VDD_LIMT_NORMAL 			38             /* Vdd limit 3.8v, become normal when back to this value*/
#define  CHARGEED_VDD_LIMT_LOW				36             /* Vdd limit 3.5v*/
#define  CHARGEED_VDD_LIMT_VERYLOW			33             /* Vdd limit 3.3v*/
#define  CHARGEED_ALERT_TIME	10           	       	/*Alert period  in second */

#define CHARGEED_POLL_INTERVAL        1         
#define CHARGEED_POLL_TIMEOUT         50         

#endif
