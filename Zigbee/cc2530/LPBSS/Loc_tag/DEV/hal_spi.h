#include "ioCC2530.h"

#include <types.h>

#define SSN  P1_4
#define SCK  P1_5
#define MOSI P1_6
#define MISO P1_7
#define LOW   0
#define HIGH  1
#define RLED P1_1
#define YLED P1_0

/* ------------------------------------------------------------------------------------------------
 *                                      Compiler Abstraction
 * ------------------------------------------------------------------------------------------------
 */
 
#ifdef __IAR_SYSTEMS_ICC__
#pragma language=extended
#define SFR(name,addr)   __sfr   __no_init  volatile  unsigned char  name @ addr;
#define SFRBIT(name, addr, bit7, bit6, bit5, bit4, bit3, bit2, bit1, bit0) \
__sfr __no_init volatile union \
{                              \
  unsigned char name;          \
  struct {                     \
    unsigned char bit0 : 1;    \
    unsigned char bit1 : 1;    \
    unsigned char bit2 : 1;    \
    unsigned char bit3 : 1;    \
    unsigned char bit4 : 1;    \
    unsigned char bit5 : 1;    \
    unsigned char bit6 : 1;    \
    unsigned char bit7 : 1;    \
  };                           \
} @ addr;
#define SBIT(name,addr) /* not in use for IAR C Compiler */
#define XREG(addr)       ((unsigned char volatile __xdata *) 0)[addr]
#define PXREG(addr)      ((unsigned char volatile __xdata *) addr)
#define VECT(num,addr)   addr

#elif defined __IAR_SYSTEMS_ASM__
#define SFR(name,addr)   name  DEFINE  addr
SFRBITMACRO MACRO t, addr, bit7 , bit6, bit5, bit4, bit3, bit2, bit1, bit0
t    DEFINE addr
bit7 DEFINE addr.7
bit6 DEFINE addr.6
bit5 DEFINE addr.5
bit4 DEFINE addr.4    // NB: do not modify indentation of this macro
bit3 DEFINE addr.3
bit2 DEFINE addr.2
bit1 DEFINE addr.1
bit0 DEFINE addr.0
            ENDM
#define SFRBIT(name, addr, bit7, bit6, bit5, bit4, bit3, bit2, bit1, bit0) \
    SFRBITMACRO <name>, <addr>, <bit7>, <bit6>, <bit5>, <bit4>, <bit3>, <bit2>, <bit1>, <bit0>
#define SBIT(name,addr)  name  DEFINE  addr
#define XREG(addr)       addr
#define PXREG(addr)      addr
#define VECT(num,addr)   addr
/* IAR assembler uses some predefined registers. The following prevents name collisions. */
#define SP   SPx
#define ACC  ACCx
#define B    Bx
#define PSW  PSWx
#define CY   CYx
#define AC   ACx
#define F0   F0x
#define RS1  RS1x
#define RS0  RS0x
#define OV   OVx
#define P    Px

#else
#error "Unrecognized compiler."
#endif


void SPI_CC_Init(void);
void SPI_Read_Register(UINT8 data);
void SPI_Write_Data(UINT8 data);
void SpiReadWriteRegister(UINT8 addr,UINT8 data);
