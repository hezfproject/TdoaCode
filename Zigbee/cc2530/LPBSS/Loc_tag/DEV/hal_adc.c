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

/*******************************************************************************
* GLOBAL FUNCTIONS
*/

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

