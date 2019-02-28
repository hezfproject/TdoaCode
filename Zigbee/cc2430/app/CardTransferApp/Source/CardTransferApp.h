/**************************************************************************************************
  Filename:       CardTransferApp.h
  Revised:        $Date: 2010/08/04 23:47:04 $
  Revision:       $Revision: 1.1 $

  Description:    This file contains the Ammeter Transfer Application definitions.

**************************************************************************************************/

#ifndef CARDTRANSFERAPP_H
#define CARDTRANSFERAPP_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#if (defined CARDTRANSFER_COLLECTOR)
#include "CardTransferApp_Collector.h"
#elif (defined CARDTRANSFER_REPEATER)
#include "CardTransferApp_Repeater.h"
#elif (defined CARDTRANSFER_DATACENTER)
#include "CardTransferApp_DataCenter.h"
#endif

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
