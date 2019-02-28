#ifndef _CRC_H_
#define _CRC_H_

#include "CommonTypes.h"

/*
 * Function for calculating CRC
 * Note: If this function is called the first time, the third parameter should be 0xFFFF.
 * 		 If a large data is split to many frame to calculate CRC, usLastCRC should be set to the
 * 		 CRC of previous frame.
 */
uint_16 CRC16(uint_8 * puchMsg, uint_16 usDataLen, uint_16 usLastCRC);

#endif

