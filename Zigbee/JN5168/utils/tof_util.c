#include <Math.h>
#include "tof_util.h"
#include "printf_util.h"
#include "app_protocol.h"

#define ABS(x)     ((x) >= 0 ? (x) : (-1)*(x))
#define TMP_MAX_READINGS    10

PRIVATE double grubbs_lookup[] = {1.15, 1.46, 1.67, 1.82, 1.94, 2.03, 2.11, 2.18, 2.23, 2.28, 2.33, 2.37, 2.41, 2.44, 2.48, 2.5, \
                                2.53, 2.56, 2.58, 2.6, 2.62, 2.64, 2.66, 2.74, 2.81, 2.87, 2.96, 3.17};

// return meter
// need u8MaxReadings to define i32RecFlag & i32GrubFlag, wait for the malloc util
PUBLIC int16 i16GetTofDistance(tsAppApiTof_Data* asTofData, const uint8 u8MaxReadings) 
{
    int32 i32SuccessNum = 0;
    int32 n;
    int32 i32RecFlag[TMP_MAX_READINGS];         // receive flag: 1: sucess, 0: failed
    int32 i32GrubFlag[TMP_MAX_READINGS];    
    int32 i32RecMin = 10000, i32RecMax = 0, i32MaxIndex = 0, i32MinIndex = 0;

    int32 rtVal=0;


    if(u8MaxReadings <=3 )
    {
        for(n = 0; n < u8MaxReadings; n++) 
        {
            if(asTofData[n].u8Status == MAC_TOF_STATUS_SUCCESS)
            {
                rtVal += (int32)(asTofData[n].s32Tof*0.003);     // set negative data to be 0 for all success data, and set data to be decimeter
                i32SuccessNum++;
            }
        }

        if(i32SuccessNum > 0) return (int16)(rtVal/i32SuccessNum/10);
        else return INVALID_TOF_DISTANCE;
    }
    

    // filter success tof data
    for(n = 0; n < u8MaxReadings; n++) 
    {
        if(asTofData[n].u8Status == MAC_TOF_STATUS_SUCCESS)
        {
            i32RecFlag[n] = 1;
            asTofData[n].s32Tof = (int32)(asTofData[n].s32Tof*0.003);     // set negative data to be 0 for all success data, and set data to be decimeter
            i32SuccessNum++;
        }
        else
        {
            i32RecFlag[n] = 0;
        }
    }

//PrintfUtil_vPrintf("Errors: %i, ", u8MaxReadings - i32SuccessNum);

    // data analyse: remove abnormal data
    for(;;)
    {
        i32RecMin = 1000000;
        i32RecMax = -1000000;
//PrintfUtil_vPrintf("--------------\n");
        // find max & min data, and record their indexs
        for(n = 0; n < u8MaxReadings; n++)
        {
            if(i32RecFlag[n] == 1)
            {
//PrintfUtil_vPrintf("%d\n", asTofData[n].s32Tof);
                if(asTofData[n].s32Tof > i32RecMax)
                {
                    i32RecMax = asTofData[n].s32Tof;
                    i32MaxIndex = n;
                }
                if(asTofData[n].s32Tof < i32RecMin)
                {
                    i32RecMin = asTofData[n].s32Tof;
                    i32MinIndex = n;
                }
            }
//            else
//PrintfUtil_vPrintf("%d -\n", asTofData[n].s32Tof);
        }

        // remove biggest gap max & min data
        if((((i32RecMax+i32RecMin)/(MAX(1,i32RecMax-i32RecMin)) <=3) || (i32RecMax-i32RecMin > 66666))  // 66666: 20 meters
            && (i32SuccessNum > 2))
        {
            i32SuccessNum -= 2;
            i32RecFlag[i32MaxIndex] = 0;
            i32RecFlag[i32MinIndex] = 0;
        }
        else
            break;
    }

    //...........................................................................................................................
    // Grubbs process begin

    // init grubbs data
    for(n = 0; n < u8MaxReadings; n++)
        i32GrubFlag[n] = i32RecFlag[n];

    int32 maxIndex; 
    int32 sx = 0, sxx = 0;
    double  average, sigma, v, maxV; 
    for(;;)
    {
//PrintfUtil_vPrintf("==============\n");
        i32SuccessNum = 0;
        sx = 0;
        sxx = 0;
        for (n = 0; n < u8MaxReadings; ++n)
        {
            if(i32GrubFlag[n] == 1)
            {
//PrintfUtil_vPrintf("%d\n", asTofData[n].s32Tof);
                ++i32SuccessNum;
                sx += asTofData[n].s32Tof;
                sxx += asTofData[n].s32Tof*asTofData[n].s32Tof;
            }
//            else
//PrintfUtil_vPrintf("%d -\n", asTofData[n].s32Tof);
        }

        average = ((double)sx) / MAX(1, i32SuccessNum);

        if(i32SuccessNum <= 2)
            break;

        //sigma = sqrt((sxx - i32SuccessNum*average*average)/(i32SuccessNum -1));
        sigma = 0;
        for (n = 0; n < u8MaxReadings; ++n)
        {
            if(i32GrubFlag[n] == 1)
            {
                sigma += ((double)asTofData[n].s32Tof - average) * ((double)asTofData[n].s32Tof - average);
            }
        }
        sigma = sqrt(sigma/(i32SuccessNum-1));

        if (i32SuccessNum > 100)
            n = 30;
        else if (i32SuccessNum > 50)
            n = 29;
        else if(i32SuccessNum > 40)
            n = 28;
        else if(i32SuccessNum > 30)
            n= 27;
        else if(i32SuccessNum > 25)
            n = 26;

        v = sigma * grubbs_lookup[n-3];

        maxV = 0;
        maxIndex = 0;
        for (n = 0; n < u8MaxReadings; ++n)
        {
            if((i32GrubFlag[n] == 1) && (ABS(asTofData[n].s32Tof - (int32)average) > maxV))
            {
                maxV = ABS(asTofData[n].s32Tof - (int32)average);
                maxIndex = n;
            }
        }

        if(maxV > v) // this data is abnormal
            i32GrubFlag[maxIndex] = 0;
        else
            break;                    
    }
    // end of Grub process
    //.......................................................................

//PrintfUtil_vPrintf("valid: %i, ", i32SuccessNum);
    if (i32SuccessNum > 0)
    {
//PrintfUtil_vPrintf("distance: %i\n", (int16)(average/10));
        return (int16)(average/10);
    }
    else
    {
//PrintfUtil_vPrintf("distance: -1\n");
        return INVALID_TOF_DISTANCE;
    }
}

// end of file

