#ifndef _MIC2000_CRC_H
#define _MIC2000_CRC_H

#include "../CommonTypes.h"

/*
 * Function for calculating CRC, working ONLY with MIC2000 sensor collector
 */
uint_16 MICcrc(uint_8 *data, uint_8 len);

#endif

