#ifndef __KEY__
#define __KEY__

#define KEY_UP_BIT          0
#define KEY_DOWN_BIT     1
#define KEY_POWER_BIT    4
#define KEY_OK_BIT          5
#define KEY_HELP_BIT       6
#define KEY_MODE_BIT      7

#define HAL_KEY_UP	(0x01<<KEY_UP_BIT)
#define HAL_KEY_DOWN 	(0x01<<KEY_DOWN_BIT)     
#define HAL_KEY_POWER 	(0x01<<KEY_POWER_BIT)    
#define HAL_KEY_OK 	(0x01<<KEY_OK_BIT)          
#define HAL_KEY_HELP 	(0x01<<KEY_HELP_BIT)       
#define HAL_KEY_MODE 	(0x01<<KEY_MODE_BIT)      


#define KEY_MASK      ((0x01<<KEY_UP_BIT)  \
| (0x01<<KEY_DOWN_BIT) \
| (0x01<<KEY_POWER_BIT) \
| (0x01<<KEY_OK_BIT) \
| (0x01<<KEY_HELP_BIT) \
| (0x01<<KEY_MODE_BIT))
// Initial Key
void InitialKey(void);

#endif  // __KEY__
