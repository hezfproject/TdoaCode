/**************************************************************************************************
  Filename:       LocationInterface.c
  Revised:        $Date: 2009/05/26 19:50:28 $
  Revision:       $Revision: 1.3 $

  Description:    Simple location interface..
  **************************************************************************************************/
/*********************************************************************
 * INCLUDES
 */
#include "LocationInterface.h"
#include "osal.h"
#include "iocc2430.h"
/*********************************************************************
 * TYPEDEFS
 */
typedef struct _blastAcc_t
{
  struct _blastAcc_t *next;
  uint16 addr;
  int16 acc;
  byte cnt;
} blastAcc_t;

/*********************************************************************
 * LOCAL VARIABLES
 */
static blastAcc_t *blastPtr;
static int8 rssi;

/*********************************************************************
 * @fn      addBlast
 *
 * @brief   Add or initialize an RSSI blast for the given Network Address.
 *
 * @param   uint16 - Network address of the Blind Node blasting.
 *
 * @return  none
 */
void addBlast( uint16 addr)
{
	blastAcc_t *ptr = blastPtr;

	rssi = RSSIL;
	while ( ptr )
	{
		if ( ptr->addr == addr )
		{
			break;
		}
		ptr = ptr->next;
	}

	if ( ptr )
	{
		ptr->acc += rssi;
		ptr->cnt++;
	}
	else
	{
		ptr = (blastAcc_t *)osal_mem_alloc( sizeof( blastAcc_t ) );

		if ( ptr )
		{
			ptr->next = blastPtr;
			blastPtr = ptr;

			ptr->addr = addr;
			ptr->acc = rssi;
			ptr->cnt = 1;
		}
	}
}

/*********************************************************************
 * @fn      rssiRsp
 *
 * @brief   Respond to requester with average of their RSSI blasts.
 *
 * @param   uint16 - Network address of the Blind Node requesting.
 *
 * @return  average rssi value.
 */
int16 rssiRsp( uint16 addr)
{
	blastAcc_t *ptr = blastPtr;
	blastAcc_t *prev = NULL;
	int16 avgrssi = 0;

	rssi = RSSIL;
	while ( ptr )
	{
		if ( ptr->addr == addr )
		{
			break;
		}
		prev = ptr;
		ptr = ptr->next;
	}

	if ( ptr )
	{
		avgrssi = (ptr->acc + rssi) / (ptr->cnt + 1);

		if ( prev )
		{
			prev->next = ptr->next;
		}
		else
		{
			blastPtr = ptr->next;
		}
		osal_mem_free(ptr);
	}

	return avgrssi;
}

