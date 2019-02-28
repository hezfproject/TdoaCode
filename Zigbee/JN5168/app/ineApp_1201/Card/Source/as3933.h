#define READ	0x40
#define WRITE	0x0
#define DIRECT_COMMAND 0xc0
void config_As3933(void);






#define PAT32					(1<<7)
#define DAT_MASK				(1<<6)
#define ON_OFF					(1<<5)
#define MUX_123					(1<<4)
#define EN_A2					(1<<3)
#define EN_A3					(1<<2)
#define EN_A1					(1<<1)

#define ABS_HY					(1<<7)
#define AGC_TLIM				(1<<6)
#define AGC_UD					(1<<5)
#define ATT_ON					(1<<4)
#define EN_MANCH  				(1<<3)
#define EN_PAT2 				(1<<2)
#define EN_WPAT					(1<<1)
#define EN_XTAL 				(1<<0)

#define	Minimum_Preamble_Length_23MS (4<<3)

#define Shunt_Resistor3K		(1<<4)
#define T_OFF_0MS					0

#define TS2						0x96
#define TS1						0x96

#define T_OUT_50ms				(1<<5)
#define T_HBIT_8				(0xb)

#define	AS3933_REG0				0
#define	AS3933_REG1				1
#define	AS3933_REG2				2
#define	AS3933_REG3				3
#define	AS3933_REG4				4
#define	AS3933_REG5				5
#define	AS3933_REG6				6
#define	AS3933_REG7				7
#define	AS3933_REG8				8
#define	AS3933_REG9				9
#define	AS3933_REG10			10
#define	AS3933_REG11			11
#define	AS3933_REG12			12
#define	AS3933_REG13			13
#define	AS3933_REG14			14
#define	AS3933_REG15			15
#define	AS3933_REG16			16
#define	AS3933_REG17			17
#define	AS3933_REG18			18
#define	AS3933_REG19			19
