/**************************************************************************************************
  Filename:       DQUE.h
  Revised:        $Date: 2009/05/22 02:36:00 $
  Revision:       $Revision: 1.1 $
  Description:     This file contains an simple  Dqueue implement by array.
**************************************************************************************************/
#ifndef DQUE_H
#define DQUE_H
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
#define ROOTSEG __near_func
#else
#define ROOTSEG
#endif

/*********************************************************************
 * INCLUDES
 */
#include "hal_types.h"
#include "hal_defs.h"
/*********************************************************************
 * CONSTANTS
 */
/*********************************************************************
 * MACROS
 */
#define DQUE_SUCCESS  0
#define DQUE_INVALID_DATA 1
#define DQUE_EXCEED_THR 2
#define DQUE_READ_ERR    3
#define DQUE_INVALID_LEN  4

/*********************************************************************
 * TYPEDEFS
 */
typedef struct
{
  uint8* dataHead;
  uint8* dataTail;
  int16 maxBufSize;
  int16 itemSize;
  int16 dataSize;
  uint8 *pBuffer;
}DQUE_t, *DQUE;


/**************************************************************************************************
  *FUNCTIONS
  */
uint8 Dque_Init(DQUE queue, int16 bufsize, uint8* pBuf, int16 itemSize);
ROOTSEG uint8 Dque_AddDataToTail(DQUE queue, uint8* pData, int16 dataSize);
ROOTSEG uint8 Dque_GetDataFromHead(DQUE queue, uint8* pData, int16 datasize);
ROOTSEG uint8 Dque_GetItemFromHead(DQUE queue, uint8* pData);
ROOTSEG uint8 Dque_AddItemToTail(DQUE queue, uint8* pData);
ROOTSEG bool Dque_IsFull(DQUE queue);
ROOTSEG bool Dque_IsEmpty(DQUE queue);
ROOTSEG int16 Dque_GetDataSize(DQUE queue);
#endif

