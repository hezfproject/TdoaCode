#ifndef _LED_H_
#define _LED_H_

//-----------------------------------------------------------------------------
/*#define LED_Red_Off()          GPIO_SetBits(GPIOA, GPIO_Pin_12)
#define LED_Red_On()           GPIO_ResetBits(GPIOA, GPIO_Pin_12)
#define LED_Green_Off()          GPIO_SetBits(GPIOA, GPIO_Pin_15)
#define LED_Green_On()           GPIO_ResetBits(GPIOA, GPIO_Pin_15)*/

#define LED_Green_Off()          GPIO_SetBits(GPIOA, GPIO_Pin_12)
#define LED_Green_On()           GPIO_ResetBits(GPIOA, GPIO_Pin_12)
#define LED_Red_Off()          GPIO_SetBits(GPIOA, GPIO_Pin_15)
#define LED_Red_On()           GPIO_ResetBits(GPIOA, GPIO_Pin_15)
#define LED_Red_Off()          GPIO_SetBits(GPIOA, GPIO_Pin_0)
#define LED_Red_On()           GPIO_ResetBits(GPIOA, GPIO_Pin_0)

//void LED_RCC_Configuration(void);

void LED_Configuration(void);

void LED_Red_Flash(void);

void LED_Green_Flash(void);

void led_station_on(void);

void led_station_off(void);
#endif

