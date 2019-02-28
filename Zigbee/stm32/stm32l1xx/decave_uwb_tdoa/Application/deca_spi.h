/*! ------------------------------------------------------------------------------------------------------------------
 * FILE: deca_spi.h - SPI Interface Access Functions
 *
 * Copyright 2009, 2012 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * Author(s): Billy Verso / Zoran Skrba
 */

#ifndef _DECA_SPI_H_
#define _DECA_SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "type_def.h"

#define DECA_MAX_SPI_HEADER_LENGTH      (3)                     // max number of bytes in header (for formating & sizing)
#define EVB1000_LCD_SUPPORT				(1)
/*! ------------------------------------------------------------------------------------------------------------------
 * Function: openspi()
 *
 * Low level abstract function to open and initialise access to the SPI device.
 * returns 0 for success, or -1 for error
 */
int openspi(void) ;

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: closespi()
 *
 * Low level abstract function to close the the SPI device.
 * returns 0 for success, or -1 for error
 */
int closespi(void) ;




#if (EVB1000_LCD_SUPPORT == 1)
/*! ------------------------------------------------------------------------------------------------------------------
 * Function: writetoLCD()
 *
 * Low level abstract function to write data to the LCD display via SPI2 peripheral
 * Takes byte buffer and rs_enable signals
 * or returns -1 if there was an error
 */
void writetoLCD
(
    uint32       bodylength,
    uint8        rs_enable,
    const uint8 *bodyBuffer
);
#else
#define writetoLCD(x)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _DECA_SPI_H_ */



