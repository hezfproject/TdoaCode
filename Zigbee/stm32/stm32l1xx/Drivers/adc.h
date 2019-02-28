/*******************************************************************************
  Filename:     hal_adc.h
  Revised:        $Date: 18:21 2012��5��7��
  Revision:       $Revision: 1.0 $
  Description:  adc driver

*******************************************************************************/
#ifndef _HAL_ADC_H_
#define _HAL_ADC_H_

#define MAX_ADC_VALUE	84
#define MIN_ADC_VALUE   66

void ADC_Configuration(void);

void ADC_Start(void);

void ADC_Stop(void);

uint_16 ADC_Get_ADCValue(void);

/*******************************************************************************
* @fn          ADC_Get_Vdd
*
* @brief       ͨ��ADC��⣬����ɵ�ѹ
*
* @param       none
*
* @return      �����ĵ�ѹ��������Χ3.3~4.2V��С��3.3������3.3,����4.2������4.3
*/
uint_8 ADC_Get_Vdd(uint_16 adc_measure);

/*********************************************************************
 * @fn      AdcCheckVdd
 *
 * @brief   Check for minimum Vdd specified.
 *
 * @param   vdd - The board-specific Vdd reading to check for.
 *
 * @return  TRUE if the Vdd measured is greater than the 'vdd' minimum parameter;
 *          FALSE if not.
 *
 *********************************************************************/
Bool AdcCheckVdd(uint_8 vdd);


#endif
