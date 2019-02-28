/**************************************************************************************************
  Filename:       AF.c
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    Application Framework - Device Description helper functions


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

#include "OSAL.h"
#include "AF.h"
#include "nwk_globals.h"
#include "nwk_util.h"
#include "aps_groups.h"
#include "ZDProfile.h"
#include "aps_frag.h"

#if ( AF_FLOAT_SUPPORT )
  #include "math.h"
#endif

#if defined ( MT_AF_CB_FUNC )
  #include "MT_AF.h"
#endif

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * @fn      afSend
 *
 * @brief   Helper macro for V1 API to invoke V2 API.
 *
 * input parameters
 *
 * @param  *dstAddr - Full ZB destination address: Nwk Addr + End Point.
 * @param   srcEP - Origination (i.e. respond to or ack to) End Point.
 * @param   cID - A valid cluster ID as specified by the Profile.
 * @param   len - Number of bytes of data pointed to by next param.
 * @param  *buf - A pointer to the data bytes to send.
 * @param   options - Valid bit mask of AF Tx Options as defined in AF.h.
 * @param  *transID - A pointer to a byte which can be modified and which will
 *                    be used as the transaction sequence number of the msg.
 *
 * output parameters
 *
 * @param  *transID - Incremented by one if the return value is success.
 *
 * @return  afStatus_t - See previous definition of afStatus_... types.
 */
#define afSend( dstAddr, srcEP, cID, len, buf, transID, options, radius ) \
        AF_DataRequest( (dstAddr), afFindEndPointDesc( (srcEP) ), \
                          (cID), (len), (buf), (transID), (options), (radius) )

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

epList_t *epList;

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

static void afBuildMSGIncoming( aps_FrameFormat_t *aff, endPointDesc_t *epDesc,
                zAddrType_t *SrcAddress, uint8 LinkQuality, byte SecurityUse,
                uint32 timestamp );

static epList_t *afFindEndPointDescList( byte EndPoint );

static pDescCB afGetDescCB( endPointDesc_t *epDesc );

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      afInit
 *
 * @brief   Initialization function for the AF.
 *
 * @param   none
 *
 * @return  none
 */
void afInit( void )
{
  // Start with no endpoint defined
  epList = NULL;
}

/*********************************************************************
 * @fn      afRegisterExtended
 *
 * @brief   Register an Application's EndPoint description.
 *
 * @param   epDesc - pointer to the Application's endpoint descriptor.
 * @param   descFn - pointer to descriptor callback function
 *
 * NOTE:  The memory that epDesc is pointing to must exist after this call.
 *
 * @return  Pointer to epList_t on success, NULL otherwise.
 */
epList_t *afRegisterExtended( endPointDesc_t *epDesc, pDescCB descFn )
{
  epList_t *ep;
  epList_t *epSearch;

  ep = osal_mem_alloc( sizeof ( epList_t ) );
  if ( ep )
  {
    // Fill in the new list entry
    ep->epDesc = epDesc;

    // Default to allow Match Descriptor.
    ep->flags = eEP_AllowMatch;
    ep->pfnDescCB = descFn;
    ep->nextDesc = NULL;

    // Does a list exist?
    if ( epList == NULL )
      epList = ep;  // Make this the first entry
    else
    {
      // Look for the end of the list
      epSearch = epList;
      while( epSearch->nextDesc != NULL )
        epSearch = epSearch->nextDesc;

      // Add new entry to end of list
      epSearch->nextDesc = ep;
    }
  }

  return ep;
}

/*********************************************************************
 * @fn      afRegister
 *
 * @brief   Register an Application's EndPoint description.
 *
 * @param   epDesc - pointer to the Application's endpoint descriptor.
 *
 * NOTE:  The memory that epDesc is pointing to must exist after this call.
 *
 * @return  afStatus_SUCCESS - Registered
 *          afStatus_MEM_FAIL - not enough memory to add descriptor
 */
afStatus_t afRegister( endPointDesc_t *epDesc )
{
  epList_t *ep = afRegisterExtended( epDesc, NULL );

  return ((ep == NULL) ? afStatus_MEM_FAIL : afStatus_SUCCESS);
}


/*********************************************************************
 * @fn          afDataConfirm
 *
 * @brief       This function will generate the Data Confirm back to
 *              the application.
 *
 * @param       endPoint - confirm end point
 * @param       transID - transaction ID from APSDE_DATA_REQUEST
 * @param       status - status of APSDE_DATA_REQUEST
 *
 * @return      none
 */
void afDataConfirm( uint8 endPoint, uint8 transID, ZStatus_t status )
{
  endPointDesc_t *epDesc;
  afDataConfirm_t *msgPtr;

  // Find the endpoint description
  epDesc = afFindEndPointDesc( endPoint );
  if ( epDesc == NULL )
    return;

#if defined ( MT_AF_CB_FUNC )
    // If MT has subscribed for this callback, don't send as a message.
  if ( ( AFCB_CHECK( endPoint, *(epDesc->task_id), SPI_CB_AF_DATA_CNF ) ) )
  {
    // Send callback if it's subscribed
    MT_AfDataConfirm ( endPoint, transID, status );
    return;
  }
#endif

  // Determine the incoming command type
  msgPtr = (afDataConfirm_t *)osal_msg_allocate( sizeof(afDataConfirm_t) );
  if ( msgPtr )
  {
    // Build the Data Confirm message
    msgPtr->hdr.event = AF_DATA_CONFIRM_CMD;
    msgPtr->hdr.status = status;
    msgPtr->endpoint = endPoint;
    msgPtr->transID = transID;

    // send message through task message
    osal_msg_send( *(epDesc->task_id), (byte *)msgPtr );
  }
}

/*********************************************************************
 * @fn          afIncomingData
 *
 * @brief       Transfer a data PDU (ASDU) from the APS sub-layer to the AF.
 *
 * @param       aff  - pointer to APS frame format
 * @param       SrcAddress  - Source address
 * @param       LinkQuality - incoming message's link quality
 * @param       SecurityUse - Security enable/disable
 *
 * @return      none
 */
void afIncomingData( aps_FrameFormat_t *aff, zAddrType_t *SrcAddress,
                     uint8 LinkQuality, byte SecurityUse, uint32 timestamp )
{
  endPointDesc_t *epDesc = NULL;
  uint16 epProfileID = 0xFFFF;  // Invalid Profile ID
  epList_t *pList;
  uint8 grpEp;

  if ( ((aff->FrmCtrl & APS_DELIVERYMODE_MASK) == APS_FC_DM_GROUP) )
  {
    // Find the first endpoint for this group
    grpEp = aps_FindGroupForEndpoint( aff->GroupID, APS_GROUPS_FIND_FIRST );
    if ( grpEp == APS_GROUPS_EP_NOT_FOUND )
      return;   // No endpoint found

    epDesc = afFindEndPointDesc( grpEp );
    if ( epDesc == NULL )
      return;   // Endpoint descriptor not found

    pList = afFindEndPointDescList( epDesc->endPoint );
  }
  else if ( aff->DstEndPoint == AF_BROADCAST_ENDPOINT )
  {
    // Set the list
    if ( (pList = epList) )
    {
      epDesc = pList->epDesc;
    }
  }
  else if ( (epDesc = afFindEndPointDesc( aff->DstEndPoint )) )
  {
    pList = afFindEndPointDescList( epDesc->endPoint );
  }

  while ( epDesc )
  {
    if ( pList->pfnDescCB )
    {
      uint16 *pID = (uint16 *)(pList->pfnDescCB(
                                 AF_DESCRIPTOR_PROFILE_ID, epDesc->endPoint ));
      if ( pID )
      {
        epProfileID = *pID;
        osal_mem_free( pID );
      }
    }
    else if ( epDesc->simpleDesc )
    {
      epProfileID = epDesc->simpleDesc->AppProfId;
    }

    if ( (aff->ProfileID == epProfileID) ||
         ((epDesc->endPoint == ZDO_EP) && (aff->ProfileID == ZDO_PROFILE_ID)) )
    {
      {
        afBuildMSGIncoming( aff, epDesc, SrcAddress, LinkQuality, SecurityUse, timestamp );
      }
    }

    if ( ((aff->FrmCtrl & APS_DELIVERYMODE_MASK) == APS_FC_DM_GROUP) )
    {
      // Find the next endpoint for this group
      grpEp = aps_FindGroupForEndpoint( aff->GroupID, grpEp );
      if ( grpEp == APS_GROUPS_EP_NOT_FOUND )
        return;   // No endpoint found

      epDesc = afFindEndPointDesc( grpEp );
      if ( epDesc == NULL )
        return;   // Endpoint descriptor not found

      pList = afFindEndPointDescList( epDesc->endPoint );
    }
    else if ( aff->DstEndPoint == AF_BROADCAST_ENDPOINT )
    {
      pList = pList->nextDesc;
      if ( pList )
        epDesc = pList->epDesc;
      else
        epDesc = NULL;
    }
    else
      epDesc = NULL;
  }
}

/*********************************************************************
 * @fn          afBuildMSGIncoming
 *
 * @brief       Build the message for the app
 *
 * @param
 *
 * @return      pointer to next in data buffer
 */
static void afBuildMSGIncoming( aps_FrameFormat_t *aff, endPointDesc_t *epDesc,
                 zAddrType_t *SrcAddress, uint8 LinkQuality, byte SecurityUse,
                 uint32 timestamp )
{
  afIncomingMSGPacket_t *MSGpkt;
  const byte len = sizeof( afIncomingMSGPacket_t ) + aff->asduLength;
  byte *asdu = aff->asdu;
  MSGpkt = (afIncomingMSGPacket_t *)osal_msg_allocate( len );

  if ( MSGpkt == NULL )
  {
    return;
  }

  MSGpkt->hdr.event = AF_INCOMING_MSG_CMD;
  MSGpkt->groupId = aff->GroupID;
  MSGpkt->clusterId = aff->ClusterID;
  afCopyAddress( &MSGpkt->srcAddr, SrcAddress );
  MSGpkt->srcAddr.endPoint = aff->SrcEndPoint;
  MSGpkt->endPoint = epDesc->endPoint;
  MSGpkt->wasBroadcast = aff->wasBroadcast;
  MSGpkt->LinkQuality = LinkQuality;
  MSGpkt->SecurityUse = SecurityUse;
  MSGpkt->timestamp = timestamp;

  MSGpkt->cmd.TransSeqNumber = 0;
  MSGpkt->cmd.DataLength = aff->asduLength;

  if ( MSGpkt->cmd.DataLength )
  {
    MSGpkt->cmd.Data = (byte *)(MSGpkt + 1);
    osal_memcpy( MSGpkt->cmd.Data, asdu, MSGpkt->cmd.DataLength );
  }
  else
  {
    MSGpkt->cmd.Data = NULL;
  }

#if defined ( MT_AF_CB_FUNC )
  // If MT has subscribed for this callback, don't send as a message.
  if AFCB_CHECK(MSGpkt->endPoint, *(epDesc->task_id), SPI_CB_AF_DATA_IND)
  {
    af_MTCB_IncomingData( (void *)MSGpkt );
    // Release the memory.
    osal_msg_deallocate( (void *)MSGpkt );
  }
  else
#endif
  {
    // Send message through task message.
    osal_msg_send( *(epDesc->task_id), (uint8 *)MSGpkt );
  }
}

/*********************************************************************
 * @fn      AF_DataRequest
 *
 * @brief   Common functionality for invoking APSDE_DataReq() for both
 *          SendMulti and MSG-Send.
 *
 * input parameters
 *
 * @param  *dstAddr - Full ZB destination address: Nwk Addr + End Point.
 * @param  *srcEP - Origination (i.e. respond to or ack to) End Point Descr.
 * @param   cID - A valid cluster ID as specified by the Profile.
 * @param   len - Number of bytes of data pointed to by next param.
 * @param  *buf - A pointer to the data bytes to send.
 * @param  *transID - A pointer to a byte which can be modified and which will
 *                    be used as the transaction sequence number of the msg.
 * @param   options - Valid bit mask of Tx options.
 * @param   radius - Normally set to AF_DEFAULT_RADIUS.
 *
 * output parameters
 *
 * @param  *transID - Incremented by one if the return value is success.
 *
 * @return  afStatus_t - See previous definition of afStatus_... types.
 */
afStatus_t AF_DataRequest( afAddrType_t *dstAddr, endPointDesc_t *srcEP,
                           uint16 cID, uint16 len, uint8 *buf, uint8 *transID,
                           uint8 options, uint8 radius )
{
  pDescCB pfnDescCB;
  ZStatus_t stat;
  APSDE_DataReq_t req;
  afDataReqMTU_t mtu;

  // Verify source end point
  if ( srcEP == NULL )
  {
    return afStatus_INVALID_PARAMETER;
  }
  
#if !defined( REFLECTOR )
  if ( dstAddr->addrMode == afAddrNotPresent )
  {
    return afStatus_INVALID_PARAMETER;
  }
#endif  

  // Verify destination address
  req.dstAddr.addr.shortAddr = dstAddr->addr.shortAddr;

  // Validate broadcasting
  if ( ( dstAddr->addrMode == afAddr16Bit     ) ||
       ( dstAddr->addrMode == afAddrBroadcast )    )
  {
      // Check for valid broadcast values
      if( ADDR_NOT_BCAST != NLME_IsAddressBroadcast( dstAddr->addr.shortAddr )  )
      {
        // Force mode to broadcast
        dstAddr->addrMode = afAddrBroadcast;
      }
      else
      {
        // Address is not a valid broadcast type
        if ( dstAddr->addrMode == afAddrBroadcast )
        {
          return afStatus_INVALID_PARAMETER;
        }
    }
  }
  else if ( dstAddr->addrMode != afAddrGroup &&
            dstAddr->addrMode != afAddrNotPresent )
  {
    return afStatus_INVALID_PARAMETER;
  }
  req.dstAddr.addrMode = dstAddr->addrMode;

  req.profileID = ZDO_PROFILE_ID;

  if ( (pfnDescCB = afGetDescCB( srcEP )) )
  {
    uint16 *pID = (uint16 *)(pfnDescCB(
                                 AF_DESCRIPTOR_PROFILE_ID, srcEP->endPoint ));
    if ( pID )
    {
      req.profileID = *pID;
      osal_mem_free( pID );
    }
  }
  else if ( srcEP->simpleDesc )
  {
    req.profileID = srcEP->simpleDesc->AppProfId;
  }

  req.txOptions = 0;

  if ( ( options & AF_ACK_REQUEST              ) &&
       ( req.dstAddr.addrMode != AddrBroadcast ) &&
       ( req.dstAddr.addrMode != AddrGroup     )    )
  {
    req.txOptions |=  APS_TX_OPTIONS_ACK;
  }

  if ( options & AF_SKIP_ROUTING )
  {
    req.txOptions |=  APS_TX_OPTIONS_SKIP_ROUTING;
  }

  if ( options & AF_EN_SECURITY )
  {
    req.txOptions |= APS_TX_OPTIONS_SECURITY_ENABLE;
    mtu.aps.secure = TRUE;
  }
  else
  {
    mtu.aps.secure = FALSE;
  }

  mtu.kvp = FALSE;

  req.transID       = *transID;
  req.srcEP         = srcEP->endPoint;
  req.dstEP         = dstAddr->endPoint;
  req.clusterID     = cID;
  req.asduLen       = len;
  req.asdu          = buf;
  req.discoverRoute = TRUE;//(uint8)((options & AF_DISCV_ROUTE) ? 1 : 0);
  req.radiusCounter = radius;

  if (len > afDataReqMTU( &mtu ) )
  {
    if (apsfSendFragmented)
    {
      req.txOptions |= AF_FRAGMENTED | APS_TX_OPTIONS_ACK;
      stat = (*apsfSendFragmented)( &req );
    }
    else
    {
      stat = afStatus_INVALID_PARAMETER;
    }
  }
  else
  {
    stat = APSDE_DataReq( &req );
  }

  /*
   * If this is an EndPoint-to-EndPoint message on the same device, it will not
   * get added to the NWK databufs. So it will not go OTA and it will not get
   * a MACCB_DATA_CONFIRM_CMD callback. Thus it is necessary to generate the
   * AF_DATA_CONFIRM_CMD here. Note that APSDE_DataConfirm() only generates one
   * message with the first in line TransSeqNumber, even on a multi message.
   * Also note that a reflected msg will not have its confirmation generated
   * here.
   */
  if ( (req.dstAddr.addrMode == Addr16Bit) &&
       (req.dstAddr.addr.shortAddr == NLME_GetShortAddr()) )
  {
    afDataConfirm( srcEP->endPoint, *transID, stat );
  }

  if ( stat == afStatus_SUCCESS )
  {
    (*transID)++;
  }

  return (afStatus_t)stat;
}

/*********************************************************************
 * @fn      afFindEndPointDescList
 *
 * @brief   Find the endpoint description entry from the endpoint
 *          number.
 *
 * @param   EndPoint - Application Endpoint to look for
 *
 * @return  the address to the endpoint/interface description entry
 */
static epList_t *afFindEndPointDescList( byte EndPoint )
{
  epList_t *epSearch;

  // Start at the beginning
  epSearch = epList;

  // Look through the list until the end
  while ( epSearch )
  {
    // Is there a match?
    if ( epSearch->epDesc->endPoint == EndPoint )
    {
      return ( epSearch );
    }
    else
      epSearch = epSearch->nextDesc;  // Next entry
  }

  return ( (epList_t *)NULL );
}

/*********************************************************************
 * @fn      afFindEndPointDesc
 *
 * @brief   Find the endpoint description entry from the endpoint
 *          number.
 *
 * @param   EndPoint - Application Endpoint to look for
 *
 * @return  the address to the endpoint/interface description entry
 */
endPointDesc_t *afFindEndPointDesc( byte EndPoint )
{
  epList_t *epSearch;

  // Look for the endpoint
  epSearch = afFindEndPointDescList( EndPoint );

  if ( epSearch )
    return ( epSearch->epDesc );
  else
    return ( (endPointDesc_t *)NULL );
}

/*********************************************************************
 * @fn      afFindSimpleDesc
 *
 * @brief   Find the Simple Descriptor from the endpoint number.
 *
 * @param   EP - Application Endpoint to look for.
 *
 * @return  Non-zero to indicate that the descriptor memory must be freed.
 */
byte afFindSimpleDesc( SimpleDescriptionFormat_t **ppDesc, byte EP )
{
  epList_t *epItem = afFindEndPointDescList( EP );
  byte rtrn = FALSE;

  if ( epItem )
  {
    if ( epItem->pfnDescCB )
    {
      *ppDesc = epItem->pfnDescCB( AF_DESCRIPTOR_SIMPLE, EP );
      rtrn = TRUE;
    }
    else
    {
      *ppDesc = epItem->epDesc->simpleDesc;
    }
  }
  else
  {
    *ppDesc = NULL;
  }

  return rtrn;
}

/*********************************************************************
 * @fn      afGetDescCB
 *
 * @brief   Get the Descriptor callback function.
 *
 * @param   epDesc - pointer to the endpoint descriptor
 *
 * @return  function pointer or NULL
 */
static pDescCB afGetDescCB( endPointDesc_t *epDesc )
{
  epList_t *epSearch;

  // Start at the beginning
  epSearch = epList;

  // Look through the list until the end
  while ( epSearch )
  {
    // Is there a match?
    if ( epSearch->epDesc == epDesc )
    {
      return ( epSearch->pfnDescCB );
    }
    else
      epSearch = epSearch->nextDesc;  // Next entry
  }

  return ( (pDescCB)NULL );
}

/*********************************************************************
 * @fn      afDataReqMTU
 *
 * @brief   Get the Data Request MTU(Max Transport Unit).
 *
 * @param   fields - afDataReqMTU_t
 *
 * @return  uint8(MTU)
 */
uint8 afDataReqMTU( afDataReqMTU_t* fields )
{
  uint8 len;
  uint8 hdr;

  if ( fields->kvp == TRUE )
  {
    hdr = AF_HDR_KVP_MAX_LEN;
  }
  else
  {
    hdr = AF_HDR_V1_1_MAX_LEN;
  }

  len = (uint8)(APSDE_DataReqMTU(&fields->aps) - hdr);

  return len;
}

/*********************************************************************
 * @fn      afGetMatch
 *
 * @brief   Set the allow response flag.
 *
 * @param   ep - Application Endpoint to look for
 * @param   action - true - allow response, false - no response
 *
 * @return  TRUE allow responses, FALSE no response
 */
uint8 afGetMatch( uint8 ep )
{
  epList_t *epSearch;

  // Look for the endpoint
  epSearch = afFindEndPointDescList( ep );

  if ( epSearch )
  {
    if ( epSearch->flags & eEP_AllowMatch )
      return ( TRUE );
    else
      return ( FALSE );
  }
  else
    return ( FALSE );
}

/*********************************************************************
 * @fn      afSetMatch
 *
 * @brief   Set the allow response flag.
 *
 * @param   ep - Application Endpoint to look for
 * @param   action - true - allow response, false - no response
 *
 * @return  TRUE if success, FALSE if endpoint not found
 */
uint8 afSetMatch( uint8 ep, uint8 action )
{
  epList_t *epSearch;

  // Look for the endpoint
  epSearch = afFindEndPointDescList( ep );

  if ( epSearch )
  {
    if ( action )
    {
      epSearch->flags |= eEP_AllowMatch;
    }
    else
    {
      epSearch->flags &= (0xff ^ eEP_AllowMatch);
    }
    return ( TRUE );
  }
  else
    return ( FALSE );
}

/*********************************************************************
 * @fn      afNumEndPoints
 *
 * @brief   Returns the number of endpoints defined (including 0)
 *
 * @param   none
 *
 * @return  number of endpoints
 */
byte afNumEndPoints( void )
{
  epList_t *epSearch;
  byte endpoints;

  // Start at the beginning
  epSearch = epList;
  endpoints = 0;

  while ( epSearch )
  {
    endpoints++;
    epSearch = epSearch->nextDesc;
  }

  return ( endpoints );
}

/*********************************************************************
 * @fn      afEndPoints
 *
 * @brief   Fills in the passed in buffer with the endpoint (numbers).
 *          Use afNumEndPoints to find out how big a buffer to supply.
 *
 * @param   epBuf - pointer to mem used
 *
 * @return  void
 */
void afEndPoints( byte *epBuf, byte skipZDO )
{
  epList_t *epSearch;
  byte endPoint;

  // Start at the beginning
  epSearch = epList;

  while ( epSearch )
  {
    endPoint = epSearch->epDesc->endPoint;

    if ( !skipZDO || endPoint != 0 )
      *epBuf++ = endPoint;

    epSearch = epSearch->nextDesc;
  }
}

/*********************************************************************
 * Semi-Precision fuctions
 */

#if ( AF_FLOAT_SUPPORT )
/*********************************************************************
 * @fn      afCnvtSP_uint16
 *
 * @brief   Converts uint16 to semi-precision structure format
 *
 * @param   sp - semi-precision structure format
 *
 * @return  16 bit value for semiprecision.
 */
uint16 afCnvtSP_uint16( afSemiPrecision_t sp )
{
  return ( ((sp.sign & 0x0001) << 15)
              | ((sp.exponent & 0x001F) << 10)
              | (sp.mantissa & 0x03FF) );
}

/*********************************************************************
 * @fn      afCnvtuint16_SP
 *
 * @brief   Converts uint16 to semi-precision structure format
 *
 * @param   rawSP - Raw representation of SemiPrecision
 *
 * @return  SemiPrecision conversion.
 */
afSemiPrecision_t afCnvtuint16_SP( uint16 rawSP )
{
  afSemiPrecision_t sp = {0,0,0};

  sp.sign = ((rawSP >> 15) & 0x0001);
  sp.exponent = ((rawSP >> 10) & 0x001F);
  sp.mantissa = (rawSP & 0x03FF);
  return ( sp );
}

/*********************************************************************
 * @fn      afCnvtFloat_SP
 *
 * @brief   Converts float to semi-precision structure format
 *
 * @param   f - float value to convert from
 *
 * NOTE: This function will convert to the closest possible
 *       representation in a 16 bit format.  Meaning that
 *       the number may not be exact.  For example, 10.7 will
 *       convert to 10.69531, because .7 is a repeating binary
 *       number.  The mantissa for afSemiPrecision_t is 10 bits
 *       and .69531 is the 10 bit representative of .7.
 *
 * @return  SemiPrecision conversion.
 */
afSemiPrecision_t afCnvtFloat_SP( float f )
{
  afSemiPrecision_t sp = {0,0,0};
  unsigned long mantissa;
  unsigned int oldexp;
  int tempexp;
  float calcMant;
  unsigned long *uf;

  if ( f < 0 )
  {
    sp.sign = 1;
    f = f * -1;
  }
  else
    sp.sign = 0;

  if ( f == 0 )
  {
    sp.exponent = (unsigned int)0;
    sp.mantissa = (unsigned int)0;
  }
  else
  {
    uf = (void*)&f;

    mantissa = *uf & 0x7fffff;
    oldexp = (unsigned int)((*uf >> 23) & 0xff);
    tempexp = oldexp - 127;

    calcMant = (float)((float)(mantissa) / (float)(0x800000));
    mantissa = (unsigned long)(calcMant * 1024);

    sp.exponent = (unsigned int)(tempexp + 15);
    sp.mantissa = (unsigned int)(mantissa);
  }

  return ( sp );
}

/*********************************************************************
 * @fn      afCnvtSP_Float
 *
 * @brief   Converts semi-precision structure format to float
 *
 * @param   sp - afSemiPrecision format to convert from
 *
 * @return  float
 */
float afCnvtSP_Float( afSemiPrecision_t sp )
{
  float a, b, c;

  if ( sp.exponent == 0 && sp.mantissa == 0 )
  {
    a = 0;
    b = 0;
  }
  else
  {
    a = (float)((float)1 + (float)((float)(sp.mantissa)/1024));

#if defined( __MWERKS__ )
    b = powf( 2.0, (float)((float)sp.exponent - 15.0) );
#else
    b = (float)pow( 2.0, sp.exponent - 15 );
#endif
  }

  if ( sp.sign )
    c = a * b * -1;
  else
    c = a * b;

  return ( c );
}
#endif

void afCopyAddress ( afAddrType_t *afAddr, zAddrType_t *zAddr )
{
  afAddr->addrMode = (afAddrMode_t)zAddr->addrMode;
  afAddr->addr.shortAddr = zAddr->addr.shortAddr;
}

/*********************************************************************
*********************************************************************/

