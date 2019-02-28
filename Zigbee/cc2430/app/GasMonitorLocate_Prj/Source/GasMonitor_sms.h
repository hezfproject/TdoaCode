/**************************************************************************************************
  Filename:       GasMonitor_sms.h
  Revised:        $Date: 2011/06/01 22:53:46 $
  Revision:       $Revision: 1.1 $

  Description:    A module to process sms of gasmonitor
  **************************************************************************************************/
#ifndef GASMONITOR_SMS_H
#define GASMONITOR_SMS_H
/*********************************************************************
* INCLUDES
*/
#include "ZComDef.h"
#include "hal_types.h"

/*************************************************************************************************
*CONSTANTS
*/
#define GASSMS_MAX_LINES 	8

#define GASSMS_STATUES_SUCCESS 			ZSuccess  //0
#define GASSMS_STATUES_OPER_FAILED		NV_OPER_FAILED  //16
#define GASSMS_STATUES_ALREADY_EXIST 	ZFailure  // 1
#define GASSMS_STATUES_INVALID_PARAM	ZInvalidParameter   // 2
#define GASSMS_STATUES_INVALID_LEN		INVALID_LEN // 4
#define GASSMS_STATUES_BUFFER_FULL            ZBufferFull 		// 0x11

/*************************************************************************************************
*EXTERN FUNCTIONS
*/
uint8 GasSms_NV_init ( void );
uint8 GasSms_SaveSms ( const char * pContant, uint8 len, uint16 seqnum, termNbr_t nmbr );
uint8 GasSms_ReadSms ( char * pContant, uint8 *pLen, uint8 num );
uint8 GasSms_GetSmsNmbr ( termNbr_t* pNmbr, uint8 num );
uint8 GasSms_GetSmsReadFlag ( bool* pIsReaded, uint8 num );
uint8 GasSms_DelSms ( uint8 num );
uint8 GasSms_ClearSms ( void );
uint8 GasSms_GetSMSCnt ( void );
uint8 GasSms_GetUnReadSMSCnt ( void );
uint8 GasSms_GetLineCnt ( const char* pSms, uint8 len,uint8 width );
uint8  GasSms_LCDPrint ( const char* pSms, uint8 len, uint8 smsstartLine,uint8 screenstartline, uint8 screenHeight,uint8 screenwidth );
uint8  GasSms_FillOverFlowSms ( uint16 seqnum,const  termNbr_t nmbr );
bool  GasSms_IsHaveOverFlowSms ( void );
#endif

