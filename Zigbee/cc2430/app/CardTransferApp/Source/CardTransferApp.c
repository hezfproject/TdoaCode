/**************************************************************************************************
  Filename:       CardTransferApp.c
  Revised:        $Date: 2010/08/04 23:47:04 $
  Revision:       $Revision: 1.1 $

  Description -   Card Data Transfer Application.

**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "CardTransferApp.h"

#if (defined CARDTRANSFER_COLLECTOR)
#include "CardTransferApp_Collector.c"
#elif (defined CARDTRANSFER_REPEATER)
#include "CardTransferApp_Repeater.c"
#elif (defined CARDTRANSFER_DATACENTER)
#include "CardTransferApp_DataCenter.c"
#endif

