#ifndef __TOF_UTIL_H__
#define __TOF_UTIL_H__

#include <jendefs.h>
#include <AppApiTof.h>


PUBLIC int16 i16GetTofDistance(tsAppApiTof_Data* asTofData, const uint8 u8MaxReadings);

#endif
