#include <jendefs.h>
#include <AppHardwareApi.h>

#include <mac_sap.h>
#include <mac_pib.h>
#include "JN5148_util.h"
#include "Utilities.h"
#include "spi.h"
#include "as3933.h"

#if 0
void config_As3933(void)
{
	//uint8 readvalue;
	//int i;
	//readvalue=0;

	PrintfUtil_vPrintf("startconfig\n");

	SpiReadWriteRegister(WRITE|AS3933_REG0,0x6e);
	SpiReadWriteRegister(WRITE|AS3933_REG1,0x2b);
	SpiReadWriteRegister(WRITE|AS3933_REG2,0x00);
	SpiReadWriteRegister(WRITE|AS3933_REG3,0x20);
	SpiReadWriteRegister(WRITE|AS3933_REG4,0xd0);
	SpiReadWriteRegister(WRITE|AS3933_REG5,0x69);
	SpiReadWriteRegister(WRITE|AS3933_REG6,0x96);
	SpiReadWriteRegister(WRITE|AS3933_REG7,0x2b);
	SpiReadWriteRegister(WRITE|AS3933_REG8,0x00);
	SpiReadWriteRegister(WRITE|AS3933_REG9,0x00);
	SpiReadWriteRegister(WRITE|AS3933_REG10,0x02);
	SpiReadWriteRegister(WRITE|AS3933_REG11,0x00);
	SpiReadWriteRegister(WRITE|AS3933_REG12,0x00);
	SpiReadWriteRegister(WRITE|AS3933_REG13,0xff);
	SpiReadWriteRegister(WRITE|AS3933_REG14,0x1d);
	SpiReadWriteRegister(WRITE|AS3933_REG15,0x0);
	SpiReadWriteRegister(WRITE|AS3933_REG16,0x0);
	SpiReadWriteRegister(WRITE|AS3933_REG17,0x0);
	SpiReadWriteRegister(WRITE|AS3933_REG18,0x0);
	SpiReadWriteRegister(WRITE|AS3933_REG19,0x0);
	//SpiCommandRegister(DIRECT_COMMAND);
	SpiReadWriteRegister(WRITE|0x0,0x4e);
	SpiReadWriteRegister(WRITE|0x1,0x2b);
	SpiReadWriteRegister(WRITE|0x2,0x00);
	SpiReadWriteRegister(WRITE|0x3,0x20);
	SpiReadWriteRegister(WRITE|0x4,0x10);
	SpiReadWriteRegister(WRITE|0x5,0x69);
	SpiReadWriteRegister(WRITE|0x6,0x96);
	SpiReadWriteRegister(WRITE|0x7,0x2b);
	SpiReadWriteRegister(WRITE|0x8,0x00);
	SpiReadWriteRegister(WRITE|0x9,0x00);
	//SpiReadWriteRegister(WRITE|0xa,0x02);
	//SpiReadWriteRegister(WRITE|0xb,0x00);
	//SpiReadWriteRegister(WRITE|0xc,0x00);
	//SpiReadWriteRegister(WRITE|0xd,0xff);
	//SpiReadWriteRegister(WRITE|0xe,0x1d);
	//SpiReadWriteRegister(WRITE|0xf,0x0);
	SpiReadWriteRegister(WRITE|0x10,0x0);
	SpiReadWriteRegister(WRITE|0x11,0x0);
	SpiReadWriteRegister(WRITE|0x12,0x0);
	SpiReadWriteRegister(WRITE|0x13,0x0);
	//SpiCommandRegister(DIRECT_COMMAND);
}
#endif
void config_As3933(void)
{

	SpiReadWriteRegister(WRITE|0x0,EN_A1|DAT_MASK|PAT32|ON_OFF);
    //SpiReadWriteRegister(WRITE|0x0,EN_A1|EN_A2|EN_A3|DAT_MASK|PAT32);
	SpiReadWriteRegister(WRITE|0x1,AGC_UD|EN_MANCH|EN_WPAT|EN_XTAL);
	SpiReadWriteRegister(WRITE|0x2,0x00);
	SpiReadWriteRegister(WRITE|0x3,Minimum_Preamble_Length_23MS);
	SpiReadWriteRegister(WRITE|0x4,Shunt_Resistor3K|T_OFF_0MS|((1<<7))|((1<<6)));
	//SpiReadWriteRegister(WRITE|0x4,Shunt_Resistor3K|T_OFF_0MS);
	SpiReadWriteRegister(WRITE|0x5,TS2);
	SpiReadWriteRegister(WRITE|0x6,TS1);
	SpiReadWriteRegister(WRITE|0x7,T_OUT_50ms|T_HBIT_8);
	SpiReadWriteRegister(WRITE|0x8,0x00);
	SpiReadWriteRegister(WRITE|0x9,0x00);
	//SpiReadWriteRegister(WRITE|0xa,0x02);
	//SpiReadWriteRegister(WRITE|0xb,0x00);
	//SpiReadWriteRegister(WRITE|0xc,0x00);
	//SpiReadWriteRegister(WRITE|0xd,0xff);
	//SpiReadWriteRegister(WRITE|0xe,0x1d);
	//SpiReadWriteRegister(WRITE|0xf,0x0);
	SpiReadWriteRegister(WRITE|0x10,0x0);
	SpiReadWriteRegister(WRITE|0x11,0x0);
	SpiReadWriteRegister(WRITE|0x12,0x0);
	SpiReadWriteRegister(WRITE|0x13,0x0);
	//SpiCommandRegister(DIRECT_COMMAND);
}


