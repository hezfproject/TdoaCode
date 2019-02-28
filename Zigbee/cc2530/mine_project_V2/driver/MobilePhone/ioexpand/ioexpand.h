#ifndef _IOEXPAND_H_
#define _IOEXPAND_H_

#include "Hal_types.h"

void ioexpand_init(void);

uint8 ioexpand_write(uint8 p0_val, uint8 p1_val);
uint8 ioexpand_read(uint8* p0_val,  uint8* p1_val);

uint8 ioexpand_setdir(uint8 p0_val, uint8 p1_val);
uint8 ioexpand_getdir(uint8* p0_val, uint8* p1_val);

uint8 ioexpand_inv(uint8 p0_val, uint8 p1_val);

#endif
