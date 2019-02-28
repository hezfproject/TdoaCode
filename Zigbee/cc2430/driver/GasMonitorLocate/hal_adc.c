/**************************************************************************************************
  Filename:       hal_adc.c
  Revised:        $Date: 2011/06/01 22:55:27 $
  Revision:       $Revision: 1.1 $

  Description:    This file contains the interface to the HAL ADC.


  Copyright 2006-2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

/**************************************************************************************************
 *                                           INCLUDES
 **************************************************************************************************/
#include  "hal_mcu.h"
#include  "hal_defs.h"
#include  "hal_types.h"
#include  "hal_adc.h"

/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/
#define HAL_ADC_EOC         0x80    /* End of Conversion bit */
#define HAL_ADC_START       0x40    /* Starts Conversion */

#define HAL_ADC_STSEL_EXT   0x00    /* External Trigger */
#define HAL_ADC_STSEL_FULL  0x10    /* Full Speed, No Trigger */
#define HAL_ADC_STSEL_T1C0  0x20    /* Timer1, Channel 0 Compare Event Trigger */
#define HAL_ADC_STSEL_ST    0x30    /* ADCCON1.ST =1 Trigger */

#define HAL_ADC_RAND_NORM   0x00    /* Normal Operation */
#define HAL_ADC_RAND_LFSR   0x04    /* Clock LFSR */
#define HAL_ADC_RAND_SEED   0x08    /* Seed Modulator */
#define HAL_ADC_RAND_STOP   0x0c    /* Stop Random Generator */
#define HAL_ADC_RAND_BITS   0x0c    /* Bits [3:2] */

#define HAL_ADC_REF_125V    0x00    /* Internal 1.25V Reference */
#define HAL_ADC_REF_AIN7    0x40    /* AIN7 Reference */
#define HAL_ADC_REF_AVDD    0x80    /* AVDD_SOC Pin Reference */
#define HAL_ADC_REF_DIFF    0xc0    /* AIN7,AIN6 Differential Reference */
#define HAL_ADC_REF_BITS    0xc0    /* Bits [7:6] */

#define HAL_ADC_DEC_064     0x00    /* Decimate by 64 : 8-bit resolution */
#define HAL_ADC_DEC_128     0x10    /* Decimate by 128 : 10-bit resolution */
#define HAL_ADC_DEC_256     0x20    /* Decimate by 256 : 12-bit resolution */
#define HAL_ADC_DEC_512     0x30    /* Decimate by 512 : 14-bit resolution */
#define HAL_ADC_DEC_BITS    0x30    /* Bits [5:4] */

#define HAL_ADC_CHN_AIN0    0x00    /* AIN0 */
#define HAL_ADC_CHN_AIN1    0x01    /* AIN1 */
#define HAL_ADC_CHN_AIN2    0x02    /* AIN2 */
#define HAL_ADC_CHN_AIN3    0x03    /* AIN3 */
#define HAL_ADC_CHN_AIN4    0x04    /* AIN4 */
#define HAL_ADC_CHN_AIN5    0x05    /* AIN5 */
#define HAL_ADC_CHN_AIN6    0x06    /* AIN6 */
#define HAL_ADC_CHN_AIN7    0x07    /* AIN7 */
#define HAL_ADC_CHN_A0A1    0x08    /* AIN0,AIN1 */
#define HAL_ADC_CHN_A2A3    0x09    /* AIN2,AIN3 */
#define HAL_ADC_CHN_A4A5    0x0a    /* AIN4,AIN5 */
#define HAL_ADC_CHN_A6A7    0x0b    /* AIN6,AIN7 */
#define HAL_ADC_CHN_GND     0x0c    /* GND */
#define HAL_ADC_CHN_VREF    0x0d    /* Positive voltage reference */
#define HAL_ADC_CHN_TEMP    0x0e    /* Temperature sensor */
#define HAL_ADC_CHN_VDD3    0x0f    /* VDD/3 */
#define HAL_ADC_CHN_BITS    0x0f    /* Bits [3:0] */

#define HAL_ADC_STSEL       HAL_ADC_STSEL_ST
#define HAL_ADC_RAND_GEN    HAL_ADC_RAND_STOP
#define HAL_ADC_REF_VOLT    HAL_ADC_REF_125V
#define HAL_ADC_DEC_RATE    HAL_ADC_DEC_064
#define HAL_ADC_SCHN        HAL_ADC_CHN_VDD3
#define HAL_ADC_ECHN        HAL_ADC_CHN_GND

/*
#define HAL_VCC42V  25164
#define HAL_VCC41V  24582
#define HAL_VCC40V  24024
#define HAL_VCC39V  23414
#define HAL_VCC38V  22850
#define HAL_VCC37V  22286
#define HAL_VCC36V  21454
#define HAL_VCC35V  20904
#define HAL_VCC34V  20348
#define HAL_VCC33V  19792
#define HAL_VCC32V  19236
*/

#if 0
#define HAL_VCC38V  1489
#define HAL_VCC37V  1452
#define HAL_VCC36V  1414
#define HAL_VCC35V  1378
#define HAL_VCC34V  1341
#define HAL_VCC33V  1301
#define HAL_VCCNULL  0
#else 
#define HAL_VCC38V  1430
#define HAL_VCC37V  1392
#define HAL_VCC36V  1353
#define HAL_VCC35V  1315
#define HAL_VCC34V  1276
#define HAL_VCC33V  1237
#define HAL_VCC32V  1198
#define HAL_VCCNULL  0
#endif
/* Vdd limit values */

static __code const uint16 HalAdcVddLimit[] =
{
  0x369C,       /*  VDD Limit - 1.6v  */
  0x3A06,       /*  VDD Limit - 1.7v  */
  0x3D70,       /*  VDD Limit - 1.8v  */
  0x40D9,       /*  VDD Limit - 1.9v  */
  0x4443,       /*  VDD Limit - 2.0v  */
  0x47AD,       /*  VDD Limit - 2.1v  */
  0x4B17,       /*  VDD Limit - 2.2v  */
  0x4E81,       /*  VDD Limit - 2.3v  */
  0x51EA,       /*  VDD Limit - 2.4v  */
};
static __code const uint16 HalAdcGasVddLimit[] =
{
  HAL_VCC33V,       /*  VDD Limit - 3.3v  */
  HAL_VCC34V,       /*  VDD Limit - 3.4v  */
  HAL_VCC35V,       /*  VDD Limit - 3.5v  */
  HAL_VCC36V,       /*  VDD Limit - 3.6v  */
  HAL_VCC37V,       /*  VDD Limit - 3.7v  */
};

static __code const uint16 HalAdcVccTable[]=
{
  HAL_VCC35V,       /*  VDD Limit - 3.5v  */
  HAL_VCC36V,       /*  VDD Limit - 3.6v  */
  HAL_VCC37V,       /*  VDD Limit - 3.7v  */
  HAL_VCC38V,       /*  VDD Limit - 3.8v  */
};
/**************************************************************************************************
 *                                              MACROS
 **************************************************************************************************/
#define HAL_ADC_CLR_EOC()   asm("PUSH A"); asm("MOV A,ADCL"); asm("MOV A,ADCH"); asm("POP A");

/**************************************************************************************************
 *                                            TYPEDEFS
 **************************************************************************************************/

/**************************************************************************************************
 *                                         GLOBAL VARIABLES
 **************************************************************************************************/

/**************************************************************************************************
 *                                          FUNCTIONS - API
 **************************************************************************************************/
extern bool HalAdcCheckVdd (uint8 limit);
static uint8 HalAdcVdd2Level(uint16 vdd);
static void GetVddAdc(uint16 *pValue);
/**************************************************************************************************
 * @fn      HalAdcInit
 *
 * @brief   Initialize ADC Service
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalAdcInit (void)
{
#if (HAL_ADC == TRUE)
  volatile uint8  tmp;

  ADCCON1 = HAL_ADC_STSEL | HAL_ADC_RAND_GEN | 0x03;
  ADCCON2 = HAL_ADC_REF_VOLT | HAL_ADC_DEC_RATE | HAL_ADC_SCHN;
  /*
  *  After reset, the first ADC reading of the extra conversion always reads GND level.
  *  We will do a few dummy conversions to bypass this bug.
  */
  tmp = ADCL;     /* read ADCL,ADCH to clear EOC */
  tmp = ADCH;
  ADCCON3 = HAL_ADC_REF_VOLT | HAL_ADC_DEC_RATE | HAL_ADC_ECHN;
  while ((ADCCON1 & HAL_ADC_EOC) != HAL_ADC_EOC);   /* Wait for conversion */
  tmp = ADCL;     /* read ADCL,ADCH to clear EOC */
  tmp = ADCH;
  ADCCON3 = HAL_ADC_REF_VOLT | HAL_ADC_DEC_RATE | HAL_ADC_ECHN;
  while ((ADCCON1 & HAL_ADC_EOC) != HAL_ADC_EOC);   /* Wait for conversion */
  tmp = ADCL;     /* read ADCL,ADCH to clear EOC */
  tmp = ADCH;

    /* use P0_7 as vdd measurement*/
 // P0SEL |= BV(7);
 // ADCCFG |= BV(7);
 // P0DIR &= ~BV(7);
 // P0INP |=BV(7);
#endif
}

/**************************************************************************************************
 * @fn      HalAdcRead
 *
 * @brief   Read the ADC based on given channel and resolution
 *
 * @param   channel - channel where ADC will be read
 * @param   resolution - the resolution of the value
 *
 * @return  16 bit value of the ADC in offset binary format.
 *          Note that the ADC is "bipolar", which means the GND (0V) level is mid-scale.
 **************************************************************************************************/
uint16 HalAdcRead (uint8 channel, uint8 resolution)
{
  int16  reading = 0;

#if (HAL_ADC == TRUE)

  uint8   i, resbits;
  uint8   adctemp;
  volatile  uint8 tmp;
  uint8  adcChannel = 1;

  /*
  * If Analog input channel is AIN0..AIN7, make sure corresponing P0 I/O pin is enabled.  The code
  * does NOT disable the pin at the end of this function.  I think it is better to leave the pin
  * enabled because the results will be more accurate.  Because of the inherent capacitance on the
  * pin, it takes time for the voltage on the pin to charge up to its steady-state level.  If
  * HalAdcRead() has to turn on the pin for every conversion, the results may show a lower voltage
  * than actuality because the pin did not have time to fully charge.
  */
  if (channel < 8)
  {
    for (i=0; i < channel; i++)
    {
      adcChannel <<= 1;
    }
  }

  /* Enable channel */
  ADCCFG |= adcChannel;

  /* Convert resolution to decimation rate */
  switch (resolution)
  {
    case HAL_ADC_RESOLUTION_8:
      resbits = HAL_ADC_DEC_064;
      break;
    case HAL_ADC_RESOLUTION_10:
      resbits = HAL_ADC_DEC_128;
      break;
    case HAL_ADC_RESOLUTION_12:
      resbits = HAL_ADC_DEC_256;
      break;
    case HAL_ADC_RESOLUTION_14:
    default:
      resbits = HAL_ADC_DEC_512;
      break;
  }

  /* read ADCL,ADCH to clear EOC */
  tmp = ADCL;
  tmp = ADCH;

  /* Setup Sample */
  adctemp = ADCCON3;
  adctemp &= ~(HAL_ADC_CHN_BITS | HAL_ADC_DEC_BITS | HAL_ADC_REF_BITS);
  adctemp |= channel | resbits | HAL_ADC_REF_VOLT;

  /* writing to this register starts the extra conversion */
  ADCCON3 = adctemp;

  /* Wait for the conversion to be done */
  while (!(ADCCON1 & HAL_ADC_EOC));

  /* Disable channel after done conversion */
  // ADCCFG &= ~adcChannel;

  /* Read the result */
  reading = (int16) (ADCL);
  reading |= (int16) (ADCH << 8);

  /* Treat small negative as 0 */
  if (reading < 0)
    reading = 0;

  switch (resolution)
  {
    case HAL_ADC_RESOLUTION_8:
      reading >>= 8;
      break;
    case HAL_ADC_RESOLUTION_10:
      reading >>= 6;
      break;
    case HAL_ADC_RESOLUTION_12:
      reading >>= 4;
      break;
    case HAL_ADC_RESOLUTION_14:
    default:
    break;
  }
#endif

  return ((uint16)reading);
}

/**************************************************************************************************
 * @fn      HalAdcCheckVdd
 *
 * @brief   Check the Vdd and return TRUE if it greater than or equal the limit
 *
 * @param   limit - limit that needs to be checked with the Vdd
 *
 * @return  TRUE if Vdd >= limit, FALSE otherwise
 *
 **************************************************************************************************/
 #if 0
bool HalAdcCheckVdd (uint8 limit)
{
  uint16 value;

  /* If rev C or earlier, dont check the voltage */
  if (CHVER < REV_D)
  {
    return TRUE;
  }

  /* Clear ADC interrupt flag */
  ADCIF = 0;

  /* Setup the new value for conversion */
  ADCCON3 = (HAL_ADC_REF_125V | HAL_ADC_DEC_064 | HAL_ADC_CHN_VDD3);

  /* Wait for the conversion to finish */
  while ( !ADCIF );

  /* Get the result */
  value = ADCL;
  value |= ((uint16) ADCH) << 8;

  /* Check the limit and return */
  return ( value >= HalAdcVddLimit[limit] );
}
#endif

/**************************************************************************************************
**************************************************************************************************/

uint8  HalAdcMeasureVdd ()
{
   uint16 value;
   uint8 level;
  /* P0_1 */
  value = HalAdcRead(HAL_ADC_CHANNEL_7, HAL_ADC_RESOLUTION_12);
  level = HalAdcVdd2Level(value);
  return level;
}

uint8 HalAdcCheckVdd(uint8 limit)
{
   uint16 value;  

    /* P0_1 */
  value = HalAdcRead(HAL_ADC_CHANNEL_7, HAL_ADC_RESOLUTION_12);
  return ( value >= HalAdcGasVddLimit[limit] );
}

#if 0
static void GetVddAdc(uint16 *pValue)
{
  /* If rev C or earlier, dont check the voltage */
  if (CHVER < REV_D)
  {
    *pValue = 0;
    return;
  }
  ADCCFG |= BV(7);
  
  /* Clear ADC interrupt flag */
  ADCIF = 0;

  /* Setup the new value for conversion */
  ADCCON3 = (HAL_ADC_REF_125V| HAL_ADC_DEC_256| HAL_ADC_CHN_AIN7);

  /* Wait for the conversion to finish */
  while ( !ADCIF );

  /* Get the result */
  *pValue = 0;
  *pValue = ADCL;
  *pValue |= ((uint16) ADCH) << 8;
}
#endif

static uint8 HalAdcVdd2Level(uint16 vdd)
{
	uint8 len = sizeof(HalAdcVccTable)/sizeof(HalAdcVccTable[0]);
	uint8 i;

	for(i=0;i<len;i++)
	{
		if(vdd < HalAdcVccTable[i])
		{
			return i;
		}
	}
	return len;
}


