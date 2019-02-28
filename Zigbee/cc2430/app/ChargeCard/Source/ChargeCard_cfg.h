/************************************************************************/
/* Charge card configs                                                   */
/************************************************************************/

#ifndef ChargeCard_CFG_H
#define ChargeCard_CFG_H
/************************************************************************/
/* defines                                                              */
/************************************************************************/

/* Channel */

#define CHARGECARD_RETREATSUPPRESS_TIME	10 	 	/* 10 minutes */

#define CHARGECARD_HEALTH_MAX 		  	  3		
#define CHARGECARD_MAX_BLAST_FAIL 		  5		

#define  CHARGECARD_KEY_LONGPRESS_TIME 		1500           /* Key press time in ms */

#define  CHARGECARD_LED_FLASH_TIME_INIT		3000   	 /* Led flash time when inital*/
#define  CHARGECARD_LED_FLASH_TIME_LOWBATTERY		300      /* Led flash time when no power*/

#define  CHARGECARD_MAC_BEACON_ORDER		15                /* Setting beacon order to 15 will disable the beacon */
#define  CHARGECARD_MAC_SUPERFRAME_ORDER	15            /* Setting superframe order to 15 will disable the superframe */
#define  CHARGECARD_VDD_LIMT_NORMAL			38             /* Vdd limit 3.8v, become normal when back to this value*/
#define  CHARGECARD_VDD_LIMT_LOW				35             /* Vdd limit 3.5v*/
#define  CHARGECARD_VDD_LIMT_VERYLOW			33             /* Vdd limit 3.3v*/
#define  CHARGECARD_ALERT_TIME	10           	       	/*Alert period  in second */

#define CHARGECARD_POLL_INTERVAL        1         			/* poll rate */
#define CHARGECARD_POLL_TIMEOUT         50         			/* poll wait time in ms */

#endif
