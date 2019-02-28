/*****************************************************************************
 *
 * MODULE:              config.h
 *
 * COMPONENT:           config.h,v
 *
 * VERSION:
 *
 * REVISION:            1.2
 *
 * DATED:               2006/09/07 14:23:55
 *
 * STATUS:              Exp
 *
 * AUTHOR:
 *
 * DESCRIPTION:
 *
 * LAST MODIFIED BY:
 *                      $Modtime: $
 *
 ****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2009. All rights reserved
 *
 ****************************************************************************/

#ifndef  CONFIG_H_INCLUDED
#define  CONFIG_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/* Network parameters */
#define PAN_ID                      0xCAFE
#define COORDINATOR_ADR             0x0000
#define END_DEVICE_START_ADR        0x0001
#define MAX_END_DEVICES             1

#define DATALEN 100

/* Defines the channels to scan. Each bit represents one channel. All channels
   in the channels (11-26) in the 2.4GHz band are scanned. */
#define SCAN_CHANNELS 		        0x01UL<<0x10;
#define CHANNEL_MIN                 11

/* Duration (ms) = 15.36ms x (2^ACTIVE_SCAN_DURATION + 1) */
#define ACTIVE_SCAN_DURATION        3
/* Duration (ms) = 15.36ms x (2^ENERGY_SCAN_DURATION + 1) */
#define ENERGY_SCAN_DURATION        3

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* CONFIG_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
