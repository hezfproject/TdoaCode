#include "cc_def.h"
#include "stm32l1xx.h"
#include "CommonTypes.h"
#include "adc.h"

#define MAX_AVD_VALUE   1165

/*******************************************************************************
* GLOBAL FUNCTIONS
*/
void ADC_Configuration(void)
{
    ADC_InitTypeDef ADC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	//设置PA1为ADC电压检测输入,PB4为使能。
	/*PA1,输入*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*PB4,输出*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* ADC1 configuration */
    ADC_StructInit(&ADC_InitStructure);
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* ADC1 regular channel18 configuration */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_4Cycles);

    ADC_DelaySelectionConfig(ADC1, ADC_DelayLength_Freeze);

    /* Enable ADC1 Power Down during Delay */
    ADC_PowerDownCmd(ADC1, ADC_PowerDown_Idle_Delay, ENABLE);
	ADC_Cmd(ADC1, DISABLE);
}

void ADC_Start(void)
{
    //PB4使能
    GPIO_SetBits(GPIOB, GPIO_Pin_4);

	/* Enable the HSI oscillator */
	RCC_HSICmd(ENABLE);
	/* Check that HSI oscillator is ready */
	while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);

    /* Enable ADC1 */
    ADC_Cmd(ADC1, ENABLE);

    /* Wait until the ADC1 is ready */
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_ADONS) == RESET);

    /* Start ADC1 Software Conversion */
    ADC_SoftwareStartConv(ADC1);

	/* Wait until ADC Channel 5 or 1 end of conversion */
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
    {
    }
}

void ADC_Stop(void)
{
	ADC_Cmd(ADC1, DISABLE);

    RCC_HSICmd(DISABLE);

    //GPIO_ResetBits(GPIOB, GPIO_Pin_4);
}

uint_16 ADC_Get_ADCValue(void)
{
    uint_16 adc_measure;

    ADC_Start();

    adc_measure= ADC_GetConversionValue(ADC1);

    ADC_Stop();

    return adc_measure;
}

/*******************************************************************************
* @fn          ADC_Get_Vdd
*
* @brief       通过ADC检测，换算成电压
*
* @param       none
*
* @return      换算后的电压。测量范围3.3~4.2V。小于3.3，返回3.3,大于4.2，返回4.3
*/
uint_8 ADC_Get_Vdd(uint_16 adc_measure)
{
    uint_8 vdd = 0;
    uint_8 i;

    adc_measure = (uint_16)((float)adc_measure * 2500 / 4095);

    for(i=0; i<20; i++)
    {
        if(adc_measure >= (MAX_AVD_VALUE - 15 * i))
        {
            vdd = MAX_ADC_VALUE - i;
            break;
        }
    }
    //超过表中最大值，取最大可测值。
    if(vdd == 0)
    {
        vdd = MIN_ADC_VALUE;
    }

	return vdd;
}

