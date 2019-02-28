/*
 * TCA6416A.h
 *
 *  Created on: 2011-4-12
 *      Author: Dong Biwen
 */

#ifndef TCA6416A_H_
#define TCA6416A_H_

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define TCA6416A_ADDRESS         (0x40)

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void TCA6416A_Write_Regester (uint8 command_regester,uint16 data_regester);
PUBLIC uint16 TCA6416A_Read_Regester (uint8 command_regester);
PUBLIC void TCA6416A_Init (void);
PUBLIC void TCA6416A_Write_Port (uint16 port_date);

#endif /* TCA6416A_H_ */
