#include "audio_interface.h"

#ifdef AUDIO_SERIAL
#include "audio_ambe_serial.c"
#else
#include "audio_ambe.c"
#endif
