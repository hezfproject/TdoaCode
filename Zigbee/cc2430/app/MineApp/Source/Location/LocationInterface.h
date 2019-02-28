/**************************************************************************************************
  Filename:       LocationInterface.h
  Revised:        $Date: 2009/05/26 01:34:24 $
  Revision:       $Revision: 1.2 $

  Description:    Simple location interface.
  **************************************************************************************************/
  
#ifndef LOCATIONINTERFACE_H
#define LOCATIONINTERFACE_H

/*************************************************************************************************
  *INCLUDES
  */
#include "hal_types.h"

/**************************************************************************************************
  *FUNCTIONS
  */
void addBlast( uint16 addr);
int16 rssiRsp(uint16 addr);

#endif

