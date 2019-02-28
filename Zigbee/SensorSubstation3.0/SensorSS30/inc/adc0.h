#ifndef _ADC0_H_
#define _ADC0_H_

#include "type.h"

#define DEFAULT_CONVER_COUNT 1
#define DEFAULT_CHANNELS ((AIN0_0 << 4) | AIN0_1)
#define CURRENT_CHANNELS (DEFAULT_CHANNELS)
#define FEED_CHANNELS    ((AIN0_2 << 4) | AGND)
#define CIRCUIT          0
enum{
    AIN0_0  = 0x00,
    AIN0_1  = 0x01,
    AIN0_2  = 0x02,
    AIN0_3  = 0x03,
    AIN0_4  = 0x04,
    AIN0_5  = 0x05,
    AIN0_6  = 0x06,
    AIN0_7  = 0x07,
    AGND    = 0x08,
    TEMP    = 0x0F
};

/*
*   u8Channels : AD0NSEL | AD0PSEL
*/
 extern long xdata sample_array[2];
 void ADC_GetValue();
 void ADC0_Init();

#endif