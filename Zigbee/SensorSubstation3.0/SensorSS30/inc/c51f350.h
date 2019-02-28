#ifndef _C51F350_H_
#define _C51F350_H_

#include "type.h"
#include "uart0.h"
#include "spi_slave.h"
#include "timer0.h"
#include "adc0.h"
#include "pca0.h"
#include "io.h"

#define WATCHDOG
//#define DEBUG

#define SYSTEMCLOCK             12250000    // Internal oscillator frequency in Hz

#define OPEN_INT()    EA = 1
#define CLOSE_INT()   EA = 0

#define WATCHDOG_SAVE(save) save = (PCA0MD & (1 << 6))

#define WATCHDOG_SET(set) PCA0MD &= ~(1 << 6);PCA0MD |= ((!(!set)) << 6)

#ifdef WATCHDOG
/* about 60 - 70ms */
#define WATCHDOG_ON()       \
do{                         \
    WATCHDOG_SET(0);        \
    PCA0MD &= ~(0x8E);      \
    PCA0CPL2 = 255;         \
    WATCHDOG_SET(1);        \
}while(0)
#else
#define WATCHDOG_ON()
#endif

#ifdef WATCHDOG
#define WATCHDOG_OFF()  WATCHDOG_SET(0)
#define WATCHDOG_FEED() PCA0CPH2 = 0x0
#else
#define WATCHDOG_OFF()  WATCHDOG_SET(0)
#define WATCHDOG_FEED()
#endif

#define WAIT_SINGAL_20(_SINGAL) \
while(_SINGAL)                  \
    WATCHDOG_FEED();            \
_SINGAL = 1

#define WAIT_SINGAL_21(_SINGAL) \
while(!_SINGAL)                 \
    WATCHDOG_FEED();            \
_SINGAL = 0

#endif