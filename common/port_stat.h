/******************************************************
 * define FPGA port status
 *****************************************************/

#ifndef _PORT_STAT_H_
#define _PORT_STAT_H_

#include "protocol_types.h"

#define PORTS_EXCEPT_ETH_NUM	12
#define PORTS_NUM	13

typedef struct
{
	char livestat;
	char devtype;
	int lostcnt;
	int totalcnt;
	pan_addr_t neighbor;
} portstat_t;

#endif
