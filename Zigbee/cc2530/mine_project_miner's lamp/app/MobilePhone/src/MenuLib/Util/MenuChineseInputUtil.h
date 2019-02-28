#ifndef MENUCHINESEINPUTUTIL
#define MENUCHINESEINPUTUTIL


typedef enum
{
       LETTER_IN,
	NUMBER_IN,
	PUNCTUATION_IN,
	CHINESE_IN,
	CHINESE_BACK,
	OUTPUT_STATUS,
}Chinese_Input_ID;

uint8  menu_ChineseOutput_Length(void);
uint8* menu_ChineseOutput(void);
void    menu_ChineseOutputClear(void);
void    menu_inputchinese_display(void);
uint8   menu_inputchinese_onkey(uint8 keys, uint8 status);
void menu_ChineseContinuesPressTestEnd(void);

#endif
