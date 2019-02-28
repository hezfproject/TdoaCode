/**************************************************************************************************
  Filename:       MT_AF.c
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    MonitorTest functions for the AF layer.


  Copyright 2004-2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "MTEL.h"
#include "MT_AF.h"
#include "nwk.h"
#include "OnBoard.h"
#include "SPIMgr.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

#if defined ( MT_AF_CB_FUNC )
uint16 _afCallbackSub;
#endif

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

#if defined ( MT_AF_FUNC )
/*********************************************************************
 * @fn      MT_afCommandProcessing
 *
 * @brief
 *
 *   Process all the AF commands that are issued by test tool
 *
 * @param   cmd_id - Command ID
 * @param   len    - Length of received SPI data message
 * @param   data   - pointer to received SPI data message
 *
 * @return  none
 */
void MT_afCommandProcessing( uint16 cmd_id , byte len , byte *pData )
{
  byte i;
  endPointDesc_t *epDesc;
  uint8 af_stat = afStatus_FAILED;

  switch (cmd_id)
  {
    case SPI_CMD_AF_INIT:
      afInit();
      break;

    case SPI_CMD_AF_REGISTER:

      if ( afFindEndPointDesc( *pData ) != NULL )
      {
        af_stat = ZApsDuplicateEntry;
      }
      else
      {
        af_stat = ZMemError;
        epDesc = ( endPointDesc_t * )osal_mem_alloc( sizeof( endPointDesc_t ) );
        if ( epDesc )
        {
          epDesc->simpleDesc = ( SimpleDescriptionFormat_t * )
                      osal_mem_alloc( sizeof( SimpleDescriptionFormat_t ) );
          if ( epDesc->simpleDesc )
          {
            af_stat = ZSuccess;
            epDesc->task_id = &MT_TaskID;
            epDesc->endPoint = *pData++;
            epDesc->simpleDesc->EndPoint = epDesc->endPoint;
            epDesc->simpleDesc->AppProfId = BUILD_UINT16( pData[1],pData[0]);
            pData += 2;
            epDesc->simpleDesc->AppDeviceId = BUILD_UINT16( pData[1],pData[0]);
            pData += 2;
            epDesc->simpleDesc->AppDevVer = (*pData++) & AF_APP_DEV_VER_MASK ;
            epDesc->simpleDesc->Reserved = (*pData++) & AF_APP_FLAGS_MASK ;

            epDesc->simpleDesc->AppNumInClusters = *pData++;
            if (epDesc->simpleDesc->AppNumInClusters)
            {
              epDesc->simpleDesc->pAppInClusterList = (uint16 *)
                  osal_mem_alloc((epDesc->simpleDesc->AppNumInClusters)*sizeof(uint16));
              if(  epDesc->simpleDesc->pAppInClusterList )
              {
                for (i=0; i<(epDesc->simpleDesc->AppNumInClusters); i++, pData+=2)
                {
                  epDesc->simpleDesc->pAppInClusterList[i] = BUILD_UINT16(pData[1], pData[0]);
                }
              }
              else
              {
                af_stat = ZMemError;
              }
            }
            epDesc->simpleDesc->AppNumOutClusters = *pData++;
            if (epDesc->simpleDesc->AppNumOutClusters)
            {
              epDesc->simpleDesc->pAppOutClusterList = (uint16 *)
                osal_mem_alloc((epDesc->simpleDesc->AppNumOutClusters)*sizeof(uint16));
              if ( epDesc->simpleDesc->AppNumOutClusters )
              {
                for (i=0; i<(epDesc->simpleDesc->AppNumOutClusters); i++, pData+=2)
                {
                  epDesc->simpleDesc->pAppOutClusterList[i] = BUILD_UINT16(pData[1], pData[0]);
                }
              }
              else
              {
                af_stat = ZMemError;
              }
            }
            epDesc->latencyReq = (afNetworkLatencyReq_t)*pData;
          }
        }

        if ( af_stat == ZSuccess )
        {
          af_stat = afRegister( epDesc );
        }

        if ( af_stat != ZSuccess )
        {
          if (epDesc->simpleDesc)
          {
            if (epDesc->simpleDesc->pAppInClusterList)
              osal_mem_free(epDesc->simpleDesc->pAppInClusterList);

            if (epDesc->simpleDesc->AppNumOutClusters)
              osal_mem_free(epDesc->simpleDesc->pAppOutClusterList);

            osal_mem_free(epDesc->simpleDesc);
          }

          if (epDesc)
            osal_mem_free(epDesc);
        }
      }

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_AF_DEFAULT;
      MT_SendSPIRespMsg( af_stat, SPI_CMD_AF_REGISTER, len, SPI_RESP_LEN_AF_DEFAULT );

      break;

    case SPI_CMD_AF_SENDMSG:
    {
      afAddrType_t dstAddr;
      endPointDesc_t *srcEP;

      /* transId, srcEP, dstAddr, dstEP, cId, len, data, txoptions, radius */

      dstAddr.addrMode = afAddr16Bit;
      dstAddr.addr.shortAddr = BUILD_UINT16( pData[3], pData[2] );
      dstAddr.endPoint = pData[4];
      srcEP = afFindEndPointDesc( pData[1] );

      af_stat = AF_DataRequest( &dstAddr, srcEP, BUILD_UINT16( pData[6], pData[5] ),
                                 pData[7], pData+8, pData, *(pData+8+pData[7]), *(pData+9+pData[7]) );
                                                                         //pData[9], pData[10] );

      len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_AF_DEFAULT;
      MT_SendSPIRespMsg( af_stat, SPI_CMD_AF_SENDMSG,
                             len, SPI_RESP_LEN_AF_DEFAULT );
    }
    break;

  }
}
/***************************************************************************************************
 * @fn      MT_AfDataConfirm
 *
 * @brief   Process
 *
 * @param
 *
 * @return  none
 ***************************************************************************************************/
void MT_AfDataConfirm( uint8 endPoint, uint8 transID, ZStatus_t status )
{
  uint8 buf[3];

  buf[0] = status;
  buf[1] = endPoint;
  buf[2] = transID;

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
  MT_BuildAndSendZToolCB( SPI_CB_AF_DATA_CNF, 3, buf );
#endif
}
#endif  // #if defined ( MT_AF_FUNC )

#if defined ( MT_AF_CB_FUNC )
/*********************************************************************
 * @fn          af_MTCB_IncomingData
 *
 * @brief       Process the callback subscription for AF Incoming data.
 *
 * @param       pkt - Incoming AF data.
 *
 * @return      none
 */
void af_MTCB_IncomingData( void *pkt )
{
  afIncomingMSGPacket_t *MSGpkt = (afIncomingMSGPacket_t *)pkt;
  uint8 *memPtr, *ptr;
  uint8 len = 9 + MSGpkt->cmd.DataLength;

  memPtr = osal_mem_alloc( len );
  if ( memPtr )
  {
    ptr = memPtr;

    //*ptr++ = MSGpkt->hdr.event;
    *ptr++ = HI_UINT16( MSGpkt->srcAddr.addr.shortAddr );
    *ptr++ = LO_UINT16( MSGpkt->srcAddr.addr.shortAddr );
    *ptr++ = MSGpkt->srcAddr.endPoint;
    *ptr++ = MSGpkt->endPoint;
    *ptr++ = HI_UINT16( MSGpkt->clusterId );
    *ptr++ = LO_UINT16( MSGpkt->clusterId );
    *ptr++ = MSGpkt->cmd.DataLength;
    ptr = osal_memcpy( ptr, MSGpkt->cmd.Data, MSGpkt->cmd.DataLength );
    *ptr++ = MSGpkt->wasBroadcast;
    *ptr++ = MSGpkt->LinkQuality;

#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
    MT_BuildAndSendZToolCB( SPI_CB_AF_DATA_IND, len, memPtr );
#endif
    osal_mem_free( memPtr );
  }
}
#endif  // #if defined ( MT_AF_CB_FUNC )

/*********************************************************************
*********************************************************************/
