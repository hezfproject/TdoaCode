#ifndef _HAL_BEEPER_H_
#define _HAL_BEEPER_H_

#include "hal_mcu.h"

#define BEEPER          P2_0
#define BEEPER_BIT      0

void HalBeepInit(void);

void HalBeepBegin(void);

void HalBeepStop(void);

#endif
