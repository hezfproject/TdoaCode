/*******************************************************************************
  Filename:     types.h
  Revised:        $Date: 18:56 2012Äê5ÔÂ7ÈÕ
  Revision:       $Revision: 1.0 $

  Description:  type definitions

*******************************************************************************/

#ifndef _TYPES_H
#define _TYPES_H

/*******************************************************************************
 * TYPEDEFS
 */

typedef signed   char   INT8;
typedef unsigned char   UINT8;

typedef signed   short  INT16;
typedef unsigned short  UINT16;

typedef signed   long   INT32;
typedef unsigned long   UINT32;

typedef unsigned char   bool;

#define VOID    void
typedef VOID (*ISR_PFN)(VOID);
typedef VOID (*VFPTR)(VOID);

typedef enum
{
    false = 0,
    true = 1
}BOOL;

#ifdef PROTO_TYPE_ADAP
typedef UINT16          uint16;
typedef UINT32          uint32;
typedef INT8            int8;
typedef INT32           int32;
typedef INT16           int16;
typedef UINT8           uint8;
#endif

/*******************************************************************************
 * Compiler abstraction
 */

/*****************************************************
 * IAR MSP430
 */
#ifdef __IAR_SYSTEMS_ICC__

#define _PRAGMA(x) _Pragma(#x)

#if defined __ICC430__

#ifndef CODE
#define CODE
#endif
#ifndef XDATA
#define XDATA
#endif
#define FAR
#define NOP()  asm("NOP")

#define HAL_ISR_FUNC_DECLARATION(f,v)   \
    _PRAGMA(vector=v##_VECTOR) __interrupt VOID f(VOID)
#define HAL_ISR_FUNC_PROTOTYPE(f,v)     \
    _PRAGMA(vector=v##_VECTOR) __interrupt VOID f(VOID)
#define HAL_ISR_FUNCTION(f,v)           \
    HAL_ISR_FUNC_PROTOTYPE(f,v); HAL_ISR_FUNC_DECLARATION(f,v)


/*****************************************************
 * IAR 8051
 */
#elif defined __ICC8051__

#ifndef BSP_H
#define CODE   __code
#define XDATA  __xdata
#endif

#define FAR
#define NOP()  asm("NOP")

#define HAL_MCU_LITTLE_ENDIAN()   __LITTLE_ENDIAN__
#define HAL_ISR_FUNC_DECLARATION(f,v)   \
    _PRAGMA(vector=v) __near_func __interrupt VOID f(VOID)
#define HAL_ISR_FUNC_PROTOTYPE(f,v)     \
    _PRAGMA(vector=v) __near_func __interrupt VOID f(VOID)
#define HAL_ISR_FUNCTION(f,v)           \
    HAL_ISR_FUNC_PROTOTYPE(f,v); HAL_ISR_FUNC_DECLARATION(f,v)

#else
#error "Unsupported architecture"
#endif

/*****************************************************
 * Other compilers
 */
#else
#error "Unsupported compiler"
#endif

/*** Generic Status Return Values ***/
#define SUCCESS                   0x00
#define FAILURE                   0x01
//#define NULL                      ((VOID*)0)
#endif
