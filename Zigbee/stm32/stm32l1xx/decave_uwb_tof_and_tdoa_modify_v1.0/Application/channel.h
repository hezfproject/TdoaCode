
#include "printf_util.h"
//#include "stm32l1xx.h"
#define TOF_CHANNEL_CONFIG              0
#define BLINK_CHANNEL_CONFIG            1

/*
typedef struct
{
    uint8 channel ;
    uint8 prf ;
    uint8 datarate ;
    uint8 preambleCode ;
    uint8 preambleLength ;
    uint8 pacSize ;
    uint8 nsSFD ;
} chConfig_t ;
*/

void set_config(uint8 dr_mode);
