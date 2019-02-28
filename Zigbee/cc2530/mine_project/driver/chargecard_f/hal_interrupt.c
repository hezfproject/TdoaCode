#include "hal_interrupt.h"

#ifdef USEPORT0INT
#define PORT0PINCOUNT   (8)
#else
#define PORT0PINCOUNT   (0)
#endif

#ifdef USEPORT1INT
#define PORT1PINCOUNT   (8)
#else
#define PORT1PINCOUNT   (0)
#endif

#ifdef USEPORT2INT
#define PORT2PINCOUNT   (8)
#else
#define PORT2PINCOUNT   (0)
#endif

#define INTOFFSET (PORT0PINCOUNT + PORT1PINCOUNT + PORT2PINCOUNT)

static IntProc *aPortIntpfn[INTOFFSET];

#define PORTBITMAP(emPortNum, emBitNum) ((emPortNum << 3) + emBitNum)
#define REG_PORT_INTPROC(emPortNum, emBitNum, IOProc) 	\
do{												        \
	aPortIntpfn[PORTBITMAP(emPortNum, emBitNum)] = IOProc;	\
}while(0)


bool RegisterP012IntProc(PORTNUM emPortNum, BITNUM emBitNum, IntProc *IOProc)
{
    if (!IOProc || PORTBITMAP(emPortNum, emBitNum) >= INTOFFSET
            || aPortIntpfn[PORTBITMAP(emPortNum, emBitNum)])
        return FALSE;

    REG_PORT_INTPROC(emPortNum, emBitNum, IOProc);
    return TRUE;
}

#ifdef USEPORT0INT
HAL_ISR_FUNCTION(Port0Isr, P0INT_VECTOR)
{
    uint8 index;
    HAL_ENTER_ISR();

    for (index=BIT0; index<=BIT7; index++)
    {
        if (PORTINTJUDGE(P0, index) && aPortIntpfn[PORTBITMAP(PORT0, index)])
        {
            aPortIntpfn[PORTBITMAP(PORT0, index)]();
        }
    }
    /*
      Clear the CPU interrupt flag for Port_0
      PxIFG has to be cleared before PxIF
    */
    P0IFG = 0;
    P0IF = 0;

    CLEAR_SLEEP_MODE();
    HAL_EXIT_ISR();
}
#endif

#ifdef USEPORT1INT
HAL_ISR_FUNCTION(Port1Isr, P1INT_VECTOR)
{
    uint8 index;
    HAL_ENTER_ISR();

    for (index=BIT0; index<=BIT7; index++)
    {
        if (PORTINTJUDGE(P1, index) && aPortIntpfn[PORTBITMAP(PORT1, index)])
        {
            aPortIntpfn[PORTBITMAP(PORT1, index)]();
        }
    }
    /*
      Clear the CPU interrupt flag for Port_1
      PxIFG has to be cleared before PxIF
    */
    P1IFG = 0;
    P1IF = 0;

    CLEAR_SLEEP_MODE();
    HAL_EXIT_ISR();
}
#endif
