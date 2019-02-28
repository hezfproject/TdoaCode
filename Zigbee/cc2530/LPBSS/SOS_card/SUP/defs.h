/*************************************************????**********************
  Filename:       defs.h
  Revised:        $Date: 18:56 2012Äê5ÔÂ7ÈÕ
  Revision:       $Revision: 1.0 $

  Description:    This file contains useful macros and data types

*********************************************************?********************/

#ifndef _DEFS_H
#define _DEFS_H


/* -----------------------------------------------------------------------------
 *                                             Macros
 * -----------------------------------------------------------------------------
 */

#ifndef BV
#define BV(n)      (1 << (UINT32)(n))
#endif

#ifndef BF
#define BF(x,b,s)  (((x) & (b)) >> (s))
#endif

#ifndef MIN
#define MIN(n,m)   (((n) < (m)) ? (n) : (m))
#endif

#ifndef MAX
#define MAX(n,m)   (((n) < (m)) ? (m) : (n))
#endif

#ifndef ABS
#define ABS(n)     (((n) < 0) ? -(n) : (n))
#endif


/* takes a byte out of a UINT16 : var - UINT16,
*  ByteNum - byte to take out (0 - 3)
*/
#define BREAK_UINT16( var, ByteNum ) \
          (UINT8)((UINT16)(((var) >>((ByteNum) * 8)) & 0x00FF))


#define BUILD_UINT16(loByte, hiByte) \
          ((UINT16)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

#define BUILD_UINT8(hiByte, loByte) \
          ((UINT8)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))

#define HI_UINT8(a) (((a) >> 4) & 0x0F)
#define LO_UINT8(a) ((a) & 0x0F)

/*
 *  This macro is for use by other macros to form a fully valid C statement.
 *  Without this, the if/else conditionals could show unexpected behavior.
 *
 *  For example, use...
 *    #define SET_REGS()  st( ioreg1 = 0; ioreg2 = 0; )
 *  instead of ...
 *    #define SET_REGS()  { ioreg1 = 0; ioreg2 = 0; }
 *  or
 *    #define  SET_REGS()    ioreg1 = 0; ioreg2 = 0;
 *  The last macro would not behave as expected in the if/else construct.
 *  The second to last macro will cause a compiler error in certain uses
 *  of if/else construct
 *
 *  It is not necessary, or recommended, to use this macro where there is
 *  already a valid C statement.  For example, the following is redundant...
 *    #define CALL_FUNC()   st(  func();  )
 *  This should simply be...
 *    #define CALL_FUNC()   func()
 *
 * (The while condition below evaluates false without generating a
 *  constant-controlling-loop type of warning on most compilers.)
 */
#define st(x)           do { x } while (0)

/**************************************************************************************************
 */
#endif
