#ifndef	_GROUND_CARD_DATA_TYPE_
#define	_GROUND_CARD_DATA_TYPE_

#include "lpbssnwk.h"

// one byte alignment in Win32 and Wince app
#if (defined(WIN32) || defined(WINCE))
#pragma pack(push)
#pragma pack(1)
#endif

#define NUM_LEN 		12
#define NAME_LEN		20
#define TAG_LEN			10
#define DEPART_LEN		1
#define REGTIME_LEN		20
#define	BASEINFO_IN_LEN	(NUM_LEN + NAME_LEN + TAG_LEN + DEPART_LEN + REGTIME_LEN)

typedef struct
{    
    unsigned char   worktype;
	unsigned short   len;
    unsigned char   number[NUM_LEN];   
    unsigned char   name[NAME_LEN]; 
    unsigned char   tag[TAG_LEN]; 
    unsigned char   department;
    unsigned char   regtime[REGTIME_LEN];    
}app_Card_BaseInfo_t;

typedef struct
{     
   unsigned char maxBuff[MAX_EDEV_DATA_LEN];
}app_Card_RemarkInfo_t;

#if (defined(WIN32) || defined(WINCE))
#pragma pack(pop)
#endif

#endif	// _GROUND_CARD_DATA_TYPE_