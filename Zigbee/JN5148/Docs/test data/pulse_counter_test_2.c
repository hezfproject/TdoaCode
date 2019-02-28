
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <AppApiTof.h>
#include <LedControl.h>
#include <Button.h>

#include "config.h"
#include "app_protocol.h"
#include "JN5148_util.h"
#include "printf_util.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessSysEvent();
PRIVATE void vProcessAppEvent();


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Handles from the MAC */
PRIVATE void *s_pvMac;
PRIVATE MAC_Pib_s *s_psMacPib;
PRIVATE MAC_ExtAddr_s psMacAddr;


/****************************************************************************
 *
 * NAME: AppColdStart
 *
 ****************************************************************************/

PUBLIC void AppColdStart(void)
{
	vAHI_WatchdogStop();
	u32AppQApiInit(NULL, NULL, NULL);
	u32AHI_Init();

	PrintfUtil_u8Init(0, 2);
	PrintfUtil_vPrintf("cold start\n");

	bAHI_PulseCounterConfigure(E_AHI_PC_0, 0, 0, FALSE, FALSE);
	bAHI_StartPulseCounter(E_AHI_PC_0);

	uint16 u16CountTmp;
	bAHI_Read16BitCounter(E_AHI_PC_0, &u16CountTmp);
	PrintfUtil_vPrintf("Count: %d\n", u16CountTmp);

	while (1)
	{
		vProcessSysEvent();
		vProcessAppEvent();
	}
}


/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
	AppColdStart();
	PrintfUtil_vPrintf("warm start\n");
}

/****************************************************************************
 *
 * NAME: vProcessAppEvent
 *
 ****************************************************************************/
PRIVATE void vProcessAppEvent()
{
}

/****************************************************************************
 *
 * NAME: vProcessSysEvent
 *
 ****************************************************************************/
PRIVATE void vProcessSysEvent()
{
	MAC_MlmeDcfmInd_s *psMlmeInd;
	MAC_McpsDcfmInd_s *psMcpsInd;
	AppQApiHwInd_s    *psAHI_Ind;

	do
	{
		psMcpsInd = psAppQApiReadMcpsInd();
		if (psMcpsInd != NULL)
		{
			vProcessIncomingMcps(psMcpsInd);
			vAppQApiReturnMcpsIndBuffer(psMcpsInd);
		}
	} while (psMcpsInd != NULL);

	do
	{
		psMlmeInd = psAppQApiReadMlmeInd();
		if (psMlmeInd != NULL)
		{
			vProcessIncomingMlme(psMlmeInd);
			vAppQApiReturnMlmeIndBuffer(psMlmeInd);
		}
	} while (psMlmeInd != NULL);

	do
	{
		psAHI_Ind = psAppQApiReadHwInd();
		if (psAHI_Ind != NULL)
		{
			vProcessIncomingHwEvent(psAHI_Ind);
			vAppQApiReturnHwIndBuffer(psAHI_Ind);
		}
	} while (psAHI_Ind != NULL);
}

/****************************************************************************
 *
 * NAME: vProcessIncomingMcps
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd)
{
	switch(psMcpsInd->u8Type)
	{
		case MAC_MCPS_IND_DATA:  /* Incoming data frame */
			vHandleMcpsDataInd(psMcpsInd);
			break;
		case MAC_MCPS_DCFM_DATA: /* Incoming acknowledgement or ack timeout */
			vHandleMcpsDataDcfm(psMcpsInd);
			break;
		default:
			break;
	}
}


/****************************************************************************
 *
 * NAME: vHandleMcpsDataInd
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
}


/****************************************************************************
 *
 * NAME: vProcessIncomingMlme
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd)
{
    switch (psMlmeInd->u8Type)
    {
    case MAC_MLME_IND_ASSOCIATE: /* Incoming association request */
        break;

    case MAC_MLME_DCFM_SCAN: /* Incoming scan results */
        break;

    default:
        break;
    }
}

/****************************************************************************
 *
 * NAME: vProcessIncomingHwEvent
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind)
{
}

/****************************************************************************
 *
 * NAME: vHandleMcpsDataDcfm
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd)
{
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/








