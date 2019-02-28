#ifndef HAL_CAPTURE_H
#define HAL_CAPTURE_H

#include "hal_board.h"
#include "hal_mcu.h"
#include "hal_interrupt.h"

#define DATA_CAPTURE    P0
#define DATA_CAPTURE_A BIT4             // sensitive
#define DATA_CAPTURE_B BIT5             // dull

extern bool halCaptureInit(void);

#endif
