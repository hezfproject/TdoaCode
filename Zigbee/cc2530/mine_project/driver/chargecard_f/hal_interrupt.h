#ifndef HAL_INTERRUPT_H
#define HAL_INTERRUPT_H

#include "hal_board.h"
#include "hal_mcu.h"

#define USEPORT0INT

#define USEPORT1INT

//#define USEPORT2INT

#define IEN(x) (x##IEN)
#define IFG(port) (port##IFG)
#define PORTINTJUDGE(port, index) ((IEN(port) & BV(index)) && (IFG(port) & BV(index)))

typedef void IntProc(void);

typedef enum
{
    BIT0,
    BIT1,
    BIT2,
    BIT3,
    BIT4,
    BIT5,
    BIT6,
    BIT7
} BITNUM;

typedef enum
{
#ifdef USEPORT0INT
    PORT0,
#endif
#ifdef USEPORT1INT
    PORT1,
#endif
#ifdef USEPORT2INT
    PORT2
#endif
} PORTNUM;

bool RegisterP012IntProc(PORTNUM emPortNum, BITNUM emBitNum, IntProc *IOProc);

#endif

