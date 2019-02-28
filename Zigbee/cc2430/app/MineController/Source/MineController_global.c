/*
#include "MineApp_MP.h"
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
#include "hal_audio.h"
#include "KeyAudioISR.h"
#include "lcd_interface.h"
#endif
#include "mac_pib.h"
*/
#include "MineController_global.h"

/*********************************************************************
* LOCAL VARIABLES
*/

/*identify current MP status: idle/calling/called/talking*/
static uint8 nWorkStatus;

/* indicate the system is searching network */
//static bool MineApp_SearchingNWK = FALSE;
/*********************************************************************
* FUNCTIONS
*/
bool MineApp_JudgeStatus(uint8 WorkStatus)
{
	return (nWorkStatus == WorkStatus);
}

void MineApp_SetStatus(uint8 WorkStatus)
{
	nWorkStatus = WorkStatus;
}

/*
void MineApp_StartSearchNWK(void)
{
    if(!MineApp_SearchingNWK)
    {
        ZDApp_StartUpFromApp(0);
        MineApp_SearchingNWK = TRUE;
    }
}
void MineApp_SearchNWKStopped(void)
{
    MineApp_SearchingNWK = FALSE;
}*/