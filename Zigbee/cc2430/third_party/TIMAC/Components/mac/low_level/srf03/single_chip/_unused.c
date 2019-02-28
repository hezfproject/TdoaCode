//*******************************************************************************************
//*******************************************************************************************
//
//  Internal storage area for code that is unused but may be useful at some point.
//  Please do not distribute to customers!
//
//*******************************************************************************************
//*******************************************************************************************


/*--------------------------------------------------------------------------
 *  From mac_mcu.c, this hack removes interrupt context by executing
 *  opcode RETI.  Appeared to work OK.  It's pretty hacky though. Compiler
 *  specific too.
 */

#define CLEAR_INT_STATE_WITH_RETI()    HAL_CRITICAL_STATEMENT( asm ("lcall 0x0090") )
const __root __code unsigned char reti @ 0x0090 = 0x32;

/* from macMcuRfIsr() */

  else
  {
    MAC_ASSERT((RFIF & IRQ_FIFOP) & rfim);  /* spurious interrupt */

    /* disable further FIFOP interrupts */
    RFIM &= ~IM_FIFOP;
    S1CON = 0x00;
    CLEAR_INT_STATE_WITH_RETI();

    do
    {
      macRxThresholdIsr();
      RFIF = ~IRQ_FIFOP;
    } while (RFSTATUS & FIFOP);

    /* enable interrupts */
    RFIM |= IM_FIFOP;
  }
