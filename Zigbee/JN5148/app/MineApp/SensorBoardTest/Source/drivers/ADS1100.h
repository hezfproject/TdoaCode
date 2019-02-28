/*
 * IIC.h
 *
 *  Created on: 2011-4-2
 *      Author: Dong Biwen
 */

#ifndef ADS1100_H_
#define ADS1100_H_

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define MULTIPLEXERS_ONE   (0)
#define MULTIPLEXERS_TWO   (1)
#define MULTIPLEXERS_THREE (2)
#define MULTIPLEXERS_FOUR  (3)
#define MULTIPLEXERS_FIVE  (4)
#define MULTIPLEXERS_SIX   (5)
#define MULTIPLEXERS_SEVEN (6)
#define MULTIPLEXERS_EIGHT (7)

#define ADS1100_ADDRESS (0x90)
#define ADS1100_MODIFY  (0x9C)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
union  ADReg32
{
    uint8   Reg[3];
    struct  R
    {
        uint16  AD_Value;
        uint8   AD_State;
    }C;
};

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void ADS1100_Init(void);
PUBLIC uint16 ADS1100_ReadSingle (uint8 u8channel);
PUBLIC void ADS1100_ReadBlock (void);

#endif /* ADS1100_H_ */
