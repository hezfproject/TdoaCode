/*******************************************************************************
  Filename:     hal_adc.h
  Revised:        $Date: 18:21 2012年5月7日
  Revision:       $Revision: 1.0 $
  Description:  adc driver

*******************************************************************************/
#ifndef _HAL_ADC_H_
#define _HAL_ADC_H_


/* Vdd Limits */
#define HAL_ADC_VDD_LIMIT_0        0x00
#define HAL_ADC_VDD_LIMIT_1        0x01
#define HAL_ADC_VDD_LIMIT_2        0x02
#define HAL_ADC_VDD_LIMIT_3        0x03
#define HAL_ADC_VDD_LIMIT_4        0x04
#define HAL_ADC_VDD_LIMIT_5        0x05
#define HAL_ADC_VDD_LIMIT_6        0x06
#define HAL_ADC_VDD_LIMIT_7        0x07

/*******************************************************************************
* @fn          HAL_ADC_GetVdd
*
* @brief       通过ADC检测，换算成电量
*
* @param       none
*
* @return      换算后的电量
*/
UINT16 HAL_ADC_GetVdd(VOID);


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
BOOL HalAdcCheckVdd(uint8 vdd);
void HalAdcInit(void);
UINT16 HalAdcRead(void);
void HalIsAdcStart(BOOL IsGetAdc);



#endif
