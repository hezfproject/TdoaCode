#ifndef _CRC_H_
#define _CRC_H_



/*
 * Function for calculating CRC
 * Note: If this function is called the first time, the third parameter should be 0xFFFF.
 * 		 If a large data is split to many frame to calculate CRC, usLastCRC should be set to the
 * 		 CRC of previous frame.
 */
uint16 CRC16(uint8 * puchMsg, uint16 usDataLen, uint16 usLastCRC);

#endif

