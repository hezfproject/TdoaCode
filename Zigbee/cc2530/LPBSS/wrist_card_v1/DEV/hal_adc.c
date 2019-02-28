/*******************************************************************************
  Filename:     hal_adc.c
  Revised:        $Date: 17:22 2012年5月10日
  Revision:       $Revision: 1.0 $
  Description:  adc驱动

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <hal_mcu.h>

#define HAL_ADC_REF_VOLT    0x80

//static uint8 adcRef;


/*******************************************************************************
* GLOBAL FUNCTIONS
*/


void HalAdcInit(void)
{
    P2SEL &=~(0x01);    //AD采样电源控制口初始化
    P2DIR |= 0x01;
    P2_0 = 0x01;

    //#if (HAL_ADC == TRUE)
    APCFG |= (1 << 7); // P0_7 enable analog channel
    P0SEL |= (1 << 7);
    P0DIR &= ~(1 << 7);
    P0INP &= ~(1 << 7);

    /*P0SEL &= ~(1 << 7);
    P0DIR |= (1 << 7);
    P0_7 = 0;
    P0_7 = 1;
    P0_7 = 0;*/
    //P0INP |=(1 << 7);
    //ADCCON1 = HAL_ADC_STSEL | HAL_ADC_RAND_GEN | 0x03;
    //adcRef = HAL_ADC_REF_VOLT;
    //#endif
}

UINT16 HalAdcRead(void)
{
   UINT32  reading = 0;

  /*
   * If Analog input channel is AIN0..AIN7, make sure corresponing P0 I/O pin is enabled.  The code
   * does NOT disable the pin at the end of this function.  I think it is better to leave the pin
   * enabled because the results will be more accurate.  Because of the inherent capacitance on the
   * pin, it takes time for the voltage on the pin to charge up to its steady-state level.  If
   * HalAdcRead() has to turn on the pin for every conversion, the results may show a lower voltage
   * than actuality because the pin did not have time to fully charge.
   */

  /* Enable channel  p0_7 */
  ADCCFG |= 0x80;

  /* writing to this register starts the extra conversion */
  //ADCCON3 = channel | resbits | adcRef;
  ADCCON3 = 0x37;

  /* Wait for the conversion to be done */
  while (!(ADCCON1 & 0x80));

  /* Disable channel after done conversion */
  ADCCFG &= (0x80 ^ 0xFF);

  /* Read the result */
  reading = ADCL;
  reading |= (((unsigned int)ADCH) << 8);
  // Note that the conversion result always resides in MSB section of ADCH:ADCL adcValue >>= 4;
  // Shift 4 due to 12 bits resolution
  reading >>= 4;
  return ((UINT16)reading);
}



/*******************************************************************************
* @fn          HAL_ADC_GetVdd
*
* @brief       通过ADC检测，换算成电量
*
* @param       none
*
* @return      换算后的电量
*/
UINT16 HAL_ADC_GetVdd(VOID)
{
    UINT8 u8ADH;
    UINT8 u8ADL;
    UINT16 u16AD;

    // 内部1.15v参考电压，12位ENOB(最高位符号位)
    ADCCON3 = 0x3F;

    while (!(ADCCON1 & 0x80));

    u8ADL = ADCL;
    u8ADH = ADCH;
    u16AD = (UINT16)(u8ADH) << 8;
    u16AD |= u8ADL;
    u16AD >>= 4;

    return u16AD;
}

/*********************************************************************
 * @fn      HalAdcCheckVdd
 *
 * @brief   Check for minimum Vdd specified.
 *
 * @param   vdd - The board-specific Vdd reading to check for.
 *
 * @return  TRUE if the Vdd measured is greater than the 'vdd' minimum parameter;
 *          FALSE if not.
 *
 *********************************************************************/
BOOL HalAdcCheckVdd(uint8 vdd)
{
  ADCCON3 = 0x0F;
  while (!(ADCCON1 & 0x80));
  return (BOOL)(ADCH > vdd);
}

void HalIsAdcStart(BOOL IsGetAdc)
{
    if(IsGetAdc)
        P2_0 = 0x01;      //打开采样电压
    else
        P2_0 = 0x00;

}

