//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <C8051F350.h>                 // SFR declarations
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "c51f350.h"

//-----------------------------------------------------------------------------
// variable defined
//-----------------------------------------------------------------------------
volatile UINT32 g_u8SensorMode = IO_INVALID_MODE;
volatile UINT32 g_u32SensorData = 0;
volatile UINT32 g_u32SensorSeqNum = 0;
volatile UINT8 u8NewMode;
//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
VOID Oscillator_Init(VOID);
VOID Device_Init(VOID);
VOID SensorCheck(VOID);
VOID ModeChange(VOID);
VOID SysUpdate(VOID);
//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------
VOID main (VOID)
{
    Device_Init();                          // Initializes hardware peripherals

    while(1)
    {
        SensorCheck();
        WATCHDOG_FEED();                    // 喂狗
        SysUpdate();                        // SPI数据刷新，0.5秒一次
    }
}

VOID SensorCheck(VOID)
{
    ModeChange();                                       // 传感器模式查询
	ADC_GetValue();                                     // check circuit
    
	switch (g_u8SensorMode)                             // sample_array[0]传感器数据采集
    {                                                   // sample_array[1]传感器回路检测
    case IO_CURRENT_MODE:
        g_u32SensorData = sample_array[0];
        g_u32SensorSeqNum++;// 最好放入模块中累加
        
        if (abs(sample_array[0]) < 10) // 10mA
        {
            g_u8SensorMode = IO_INVALID_MODE;
            g_u32SensorData = 0;
        }
        
        break;
        
    case IO_FREQ_MODE:
        g_u32SensorData = PCA0_GetCounter();
        g_u32SensorSeqNum++;//最好放入模块中累加
        if (g_u32SensorData < 180)    // check circuit
        {
            g_u8SensorMode = IO_INVALID_MODE;
            g_u32SensorData = 0;
        }
        break;
        
    case IO_STATE_MODE:
        g_u32SensorData = IO_GetStateMode();
        g_u32SensorSeqNum++;//最好放入模块中累加
        break;
        
    case IO_UART485_MODE:
        g_u32SensorData = 0x12345678;
		g_u32SensorSeqNum++;
        
        if( sample_array[1] < 3)
        {
            g_u8SensorMode = IO_INVALID_MODE;
            g_u32SensorData = 0;
        }
        
        break;
    }
}

/******************************************************************************
**  获取传感器的类型
*/
VOID ModeChange(VOID)
{
    u8NewMode = IO_GetSwitchState();

    if (u8NewMode != g_u8SensorMode)
    {
        g_u32SensorSeqNum = 0;  // 新的传感器seq清零
        
        switch (g_u8SensorMode)
        {
        case IO_CURRENT_MODE:
            break;
            
        case IO_FREQ_MODE:
            PCA0_DeInit();
            break;
            
        case IO_STATE_MODE:
            break;
            
        case IO_UART485_MODE:
            break;
        }
        
        g_u8SensorMode = u8NewMode;

        if (IO_FREQ_MODE == u8NewMode)
            PCA0_Init();
    }
}

/******************************************************************************
** 根据TIMER0的定时周期标志来刷新SPI的数据BUFFER
*/
VOID SysUpdate(VOID)
{
    volatile SPI_DATA g_stSpiData;

    WATCHDOG_FEED();

    if (SysUpdateState() != false)
    {
        g_stSpiData.s32Type = g_u8SensorMode;
        g_stSpiData.s32Value = g_u32SensorData;
        g_stSpiData.u32SeqNum = g_u32SensorSeqNum;
        
        SPI0_SetSpiData((UINT8 *)(&g_stSpiData));

        if (SPI0_GetSpiData((UINT8 *)(&g_stSpiData)))
        {
            if (SPI0_ARM_COMMAND_TYPE == g_stSpiData.s32Type)
            {
                if (SPI0_ARM_COMMAND_SHT == g_stSpiData.s32Value)
                {
                    IO_Shutting(true);
                }
                else if (SPI0_ARM_COMMAND_RESU == g_stSpiData.s32Value)
                {
                    IO_Shutting(false);
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------
// Oscillator_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function initializes the system clock to use the internal oscillator
// at 24.5 MHz.
//
//-----------------------------------------------------------------------------
VOID Oscillator_Init(VOID)
{
#if SYSTEMCLOCK == 24500000
    OSCICN = 0xC3;                      // Set the internal oscillator to
#elif SYSTEMCLOCK == 12250000
    OSCICN = 0xC2;
#elif SYSTEMCLOCK == 6125000
    OSCICN = 0xC1;
#elif SYSTEMCLOCK == 3062500
    OSCICN = 0xC0;
#else
    #error "SYSTEMCLOCK error"
    // 24.5 MHz
#endif
}

//-----------------------------------------------------------------------------
// Device_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Calls all device initialization functions.
//
//-----------------------------------------------------------------------------
VOID Device_Init (VOID)
{
    WATCHDOG_OFF();
    Oscillator_Init();

    IO_Init();
    UART0_Init();
    SPI0_Init();
    Timer0_Init(1);                         // Every 1sec update a system
	ADC0_Init();
    OPEN_INT();                             // Enable global interrupts
    WATCHDOG_ON();
    WATCHDOG_FEED();
}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
