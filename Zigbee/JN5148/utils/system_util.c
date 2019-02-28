#include "jendefs.h"
#include <AppHardwareApi.h>

// the data is from JN-UG-3068 TOF API
#define DBM_START -7
#define RSSI_START 108

PRIVATE volatile bool_t bRndNumGenerated = FALSE;

PUBLIC void SysUtil_vConvertEndian(void* pvData, uint8 u8Len)
{
    int i;
    uint8* pu8Data = (uint8*) pvData;
    for(i=0;i<u8Len/2;i++)
    {
        uint8 tmp;
        tmp= pu8Data[i];
        pu8Data[i] = pu8Data[u8Len-i-1];
        pu8Data[u8Len-i-1] = tmp;
    }
}

PUBLIC int8 SysUtil_vConvertLQI2Dbm(uint8 u8Lqi)
{
    int rssi;

    /*
    The IEEE 802.15.4 standard Link Quality Indication (LQI) is derived from the RSSI 
    using the following pseudo-code: 
    
    LQI = (RSSI ¨C 20) x 3; 
     
    if (LQI < 0) 
      LQI = 0; 
    else if (LQI > 255) 
      LQI = 255;  
    */ 
    rssi = (u8Lqi/3)+20;

    //the RSSI->Dbm is almost a y=x+a curve from JN-UG-3068
    return DBM_START - (RSSI_START-rssi);
}

PUBLIC uint32 SysUtil_u32GetExtVoltage()
{
	vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
	              E_AHI_AP_INT_DISABLE,
	              E_AHI_AP_SAMPLE_8,
	              E_AHI_AP_CLOCKDIV_2MHZ,
	              E_AHI_AP_INTREF);

	 while(!bAHI_APRegulatorEnabled());  

	vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT, E_AHI_AP_INPUT_RANGE_2, E_AHI_ADC_SRC_ADC_1);
	vAHI_AdcStartSample();

	while(bAHI_AdcPoll());

	vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT, E_AHI_AP_INPUT_RANGE_2, E_AHI_ADC_SRC_ADC_1);
       vAHI_AdcStartSample();
       while(bAHI_AdcPoll() != 0x00); 
			  
	uint16 u16TmpV = u16AHI_AdcRead();

	return ((uint32)((uint32)(u16TmpV * 586) + ((uint32)(u16TmpV * 586) >> 1))) / 1000;
}

PUBLIC uint32 SysUtil_u32GetIntVoltage()
{
	vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
	              E_AHI_AP_INT_DISABLE,
	              E_AHI_AP_SAMPLE_2,
	              E_AHI_AP_CLOCKDIV_2MHZ,
	              E_AHI_AP_INTREF);

	 while(!bAHI_APRegulatorEnabled());  

	vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT, E_AHI_AP_INPUT_RANGE_1, E_AHI_ADC_SRC_VOLT);
	vAHI_AdcStartSample();

	while(bAHI_AdcPoll());

	vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT, E_AHI_AP_INPUT_RANGE_2, E_AHI_ADC_SRC_VOLT);
       vAHI_AdcStartSample();
       while(bAHI_AdcPoll() != 0x00); 
			
	uint16 u16TmpV = u16AHI_AdcRead();

	return ((uint32)((uint32)(u16TmpV * 586) + ((uint32)(u16TmpV * 586) >> 1))) / 1000;
}


PUBLIC uint16 SysUtil_u16GenRndNum()
{
	bRndNumGenerated = FALSE;
	vAHI_StartRandomNumberGenerator(E_AHI_RND_SINGLE_SHOT, E_AHI_INTS_ENABLED);
	while(bRndNumGenerated == FALSE); // wait for random generator available
	return u16AHI_ReadRandomNumber();
}

PUBLIC void vSysCtrlRndCallback()
{
	bRndNumGenerated = TRUE;
}

PUBLIC uint16 SysUtil_u16atou(const char *str)
{
	uint16 ret = 0;
	
	ret = *str - '0'; 
	
	++str;
	while (*str <= '9' && *str >= '0') 
	{
		ret = ret * 10 + (*str - '0');
		++str;
	}

	return (ret);
}


