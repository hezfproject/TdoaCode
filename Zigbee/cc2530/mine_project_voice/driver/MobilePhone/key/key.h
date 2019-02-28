#ifndef __KEY__
#define __KEY__

#include "hal_defs.h"
#include "hal_types.h"

/* Interrupt option - Enable or disable */
#define HAL_KEY_INTERRUPT_DISABLE    0x00
#define HAL_KEY_INTERRUPT_ENABLE     0x01

/* Key state - ADC, nornal and so on.
*/
#define HAL_KEY_STATE_NORMAL          0x00
#define HAL_KEY_STATE_SHIFT           0x01

#define HAL_KEY_INVALID        (0xFF)
#define HAL_KEY_SELECT         (1)
#define HAL_KEY_UP             (2)
#define HAL_KEY_BACKSPACE      (3)
#define HAL_KEY_CALL           (4)
#define HAL_KEY_LEFT           (5)
#define HAL_KEY_RIGHT          (6)
#define HAL_KEY_POWER          (7)
#define HAL_KEY_CANCEL          HAL_KEY_POWER
#define HAL_KEY_DOWN           (8)
#define HAL_KEY_1              (9)
#define HAL_KEY_2              (10)
#define HAL_KEY_3              (11)
#define HAL_KEY_4              (12)
#define HAL_KEY_5              (13)
#define HAL_KEY_6              (14)
#define HAL_KEY_7              (15)
#define HAL_KEY_8              (16)
#define HAL_KEY_9              (17)
#define HAL_KEY_STAR           (18)
#define HAL_KEY_0              (19)
#define HAL_KEY_POUND          (20)


// Timer1 operation define

#define KEY_INT_ENABLE()   (P1IEN |= BV(5))
#define KEY_INT_DISABLE() (P1IEN &= ~BV(5))

// Initial Key
void InitialKey(void);

void InitialMisc(void);

// Get key by adc sampling
uint8 GetKey(void);


void WaitKeySleep(uint16 TimeOut);


 void KeyReset(void);

void KeyEnable(void);

void backlight_ctrl(bool enable);

void shake_ctrl(bool enable);

#endif  // __KEY__
