/*! ------------------------------------------------------------------------------------------------------------------
 * FILE: deca_spi.c - SPI Interface Access Functions
 *
 * Copyright 2009, 2012 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * Author(s): Billy Verso / Zoran Skrba
 */

#include "deca_spi.h"
#include "deca_device_api.h"
#include "port.h"

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: openspi()
 *
 * Low level abstract function to open and initialise access to the SPI device.
 * returns 0 for success, or -1 for error
 */
int openspi(/*SPI_TypeDef* SPIx*/)
{
	// done by port.c, default SPI used is SPI1

	return 0;

} // end openspi()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: closespi()
 *
 * Low level abstract function to close the the SPI device.
 * returns 0 for success, or -1 for error
 */
int closespi(void)
{
	while (port_SPIx_busy_sending()); //wait for tx buffer to empty

	port_SPIx_disable();

	return 0;

} // end closespi()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: writetospi()
 *
 * Low level abstract function to write to the SPI
 * Takes two separate byte buffers for write header and write data
 * returns 0 for success, or -1 for error
 */
int writetospi
(
    uint16       headerLength,
    const uint8 *headerBuffer,
    uint32       bodylength,
    const uint8 *bodyBuffer
)
{

	int i=0;

    decaIrqStatus_t  stat ;

    stat = decamutexon() ;

    /* Wait for SPIx Tx buffer empty */
    //while (port_SPIx_busy_sending());

    port_SPIx_clear_chip_select();  //CS low

    for(i=0; i<headerLength; i++)
    {
    	port_SPIx_send_data(headerBuffer[i]); //send data on the SPI

    	while (port_SPIx_no_data()); //wait for rx buffer to fill

    	port_SPIx_receive_data(); //this clears RXNE bit
    }

    for(i=0; i<bodylength; i++)
    {
		port_SPIx_send_data(bodyBuffer[i]); //send data on the SPI

		while (port_SPIx_no_data()); //wait for rx buffer to fill

		port_SPIx_receive_data(); //this clears RXNE bit
	}

    port_SPIx_set_chip_select();  //CS high

    decamutexoff(stat) ;

    return 0;
} // end writetospi()


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: readfromspi()
 *
 * Low level abstract function to read from the SPI
 * Takes two separate byte buffers for write header and read data
 * returns the offset into read buffer where first byte of read data may be found,
 * or returns -1 if there was an error
 */
int readfromspi
(
    uint16       headerLength,
    const uint8 *headerBuffer,
    uint32       readlength,
    uint8       *readBuffer
)
{

	int i=0;

    decaIrqStatus_t  stat ;

    stat = decamutexon() ;

    /* Wait for SPIx Tx buffer empty */
    //while (port_SPIx_busy_sending());

    port_SPIx_clear_chip_select();  //CS low

    for(i=0; i<headerLength; i++)
    {
    	port_SPIx_send_data(headerBuffer[i]); //send data on the SPI

     	while (port_SPIx_no_data()); //wait for rx buffer to fill

    	port_SPIx_receive_data(); //this clears RXNE bit
    }

    for(i=0; i<readlength; i++)
    {
    	port_SPIx_send_data(0); //send data on the SPI

    	while (port_SPIx_no_data()); //wait for rx buffer to fill

    	readBuffer[i] = port_SPIx_receive_data(); //this clears RXNE bit
    }

    port_SPIx_set_chip_select();  //CS high

    decamutexoff(stat) ;

    return 0;
} // end readfromspi()

#if (EVB1000_LCD_SUPPORT == 1)

void writetoLCD
(
    uint32       bodylength,
    uint8        rs_enable,
    const uint8 *bodyBuffer
)
{

	int i = 0;
	int sleep = 0;
	//int j = 10000;

	if(rs_enable)
	{
    	port_LCD_RS_set();
    }
	else
	{
		if(bodylength == 1)
		{
			if(bodyBuffer[0] & 0x3) //if this is command = 1 or 2 - exsecution time is > 1ms
				sleep = 1 ;
		}
    	port_LCD_RS_clear();
    }

    port_SPIy_clear_chip_select();  //CS low


    //while(j--); //delay

    for(i=0; i<bodylength; i++)
    {
		port_SPIy_send_data(bodyBuffer[i]); //send data on the SPI

		while (port_SPIy_no_data()); //wait for rx buffer to fill

		port_SPIy_receive_data(); //this clears RXNE bit
	}

    //j = 10000;

    port_LCD_RS_clear();

    //while(j--); //delay

    port_SPIy_set_chip_select();  //CS high

    if(sleep)
        Delay_ms(2);
    	//Sleep(2);
} // end writetoLCD()
#endif
