#include "channel.h"

#include "instance.h"

/*
chConfig_t chConfig[9] ={
	//mode 1 - S1: 7 off, 6 off, 5 off
	{
		1,              // channel
		DWT_PRF_16M,    // prf
		DWT_BR_110K,    // datarate
        3,             // preambleCode
        DWT_PLEN_128,	// preambleLength
        DWT_PAC32,		// pacSize
        1		// non-standard SFD
    },
    //mode 2
	{
		2,              // channel
		DWT_PRF_16M,    // prf
		DWT_BR_110K,    // datarate
        3,             // preambleCode
        DWT_PLEN_128,	// preambleLength
        DWT_PAC32,		// pacSize
        0		// non-standard SFD
    },
    //mode 3
	{
		2,              // channel
		DWT_PRF_64M,    // prf
		DWT_BR_110K,    // datarate
        9,             // preambleCode
        DWT_PLEN_1024,	// preambleLength
        DWT_PAC32,		// pacSize
        1		// non-standard SFD
    },
    //mode 4
	{
		2,              // channel
		DWT_PRF_64M,    // prf
		DWT_BR_6M8,    // datarate
        9,             // preambleCode
        DWT_PLEN_128,	// preambleLength
        DWT_PAC8,		// pacSize
        0		// non-standard SFD
    },
    //mode 5
	{
		5,              // channel
		DWT_PRF_16M,    // prf
		DWT_BR_110K,    // datarate
		3,             // preambleCode
		DWT_PLEN_1024,	// preambleLength
		DWT_PAC32,		// pacSize
		1		// non-standard SFD
	},
	{
		5,              // channel
		DWT_PRF_16M,    // prf
		DWT_BR_110K,    // datarate
		3,             // preambleCode
		DWT_PLEN_64,	// preambleLength
		DWT_PAC32,		// pacSize
		1		// non-standard SFD
	},
	{
		5,              // channel
		DWT_PRF_16M,    // prf
		DWT_BR_6M8,    // datarate
		3,             // preambleCode
		DWT_PLEN_128,	// preambleLength
		DWT_PAC8,		// pacSize
		0		// non-standard SFD
	},
	//mode 7
	{
		5,              // channel
		DWT_PRF_64M,    // prf
		DWT_BR_110K,    // datarate
		9,             // preambleCode
		DWT_PLEN_1024,	// preambleLength
		DWT_PAC32,		// pacSize
		1		// non-standard SFD
	},
	//mode 8
	{
		5,              // channel
		DWT_PRF_64M,    // prf
		DWT_BR_6M8,    // datarate
		9,             // preambleCode
		DWT_PLEN_128,	// preambleLength
		DWT_PAC8,		// pacSize
		0		// non-standard SFD
	}
};
*/

void set_config(uint8 dr_mode)
{
	instanceConfig_t instConfig;
    instConfig.channelNumber = chConfig[dr_mode].channel ;
    instConfig.preambleCode = chConfig[dr_mode].preambleCode ;
    instConfig.pulseRepFreq = chConfig[dr_mode].prf ;
    instConfig.pacSize = chConfig[dr_mode].pacSize ;
    instConfig.nsSFD = chConfig[dr_mode].nsSFD ;

    instConfig.dataRate = chConfig[dr_mode].datarate ;
    instConfig.preambleLen = chConfig[dr_mode].preambleLength ;

    instance_config(&instConfig) ;                  // Set operating channel etc
}

void change_channel(uint8 channel)
{
	
}

