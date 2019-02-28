#include "lcd_interface.h"

#ifdef AUDIO_SERIAL
#include "lcd_serial1.c"
#else
#include "lcd.c"
#endif