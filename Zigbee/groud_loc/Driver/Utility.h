/******************************************************************************
 Copyright 2011, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "CC_DEF.h"

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Global variables define


//-----------------------------------------------------------------------------
// Function define

void PrintVersionMessage( void ) ;


NtrxBufferPtr LTrim( NtrxBufferPtr Buffer ) ;
uint8 GetStrLen( NtrxBufferPtr Buffer ) ;
NtrxBufferPtr ConvertCharToData( NtrxBufferPtr Buffer, uint8 *Data ) ;
NtrxBufferPtr ConvertDistanceToStr( float d, NtrxBufferPtr Buffer ) ;

void con_RegisterList( void ) ;
void con_PutRegister( uint8 Addr ) ;


uint8 EepromRead( uint16 Addr ) ;
void EepromWrite( uint16 Addr, uint8 Data ) ;

uint8 calcFCS(uint8 *pBuf, uint8 len);

//-----------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif

#endif /* _UTILITY_H_ */

//-----------------------------------------------------------------------------

