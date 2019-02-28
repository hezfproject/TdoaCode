#ifndef _TYPES_H_
#define _TYPES_H_

#include "CommonTypes.h"
#include "CommonMacro.h"
#include <string>
#include <vector>
#include <map>
using	namespace	std;

#define PanAddr_t	uint_16

#define PAN_ADDR_DISABLE	PanAddr_t(3)
#define PAN_ADDR_MODBUS		PanAddr_t(4)

typedef	std::string		termID_t;
#define TERM_ID_ALL		"11111111"
#define TERM_ID_CENTER		"00000000"
#define TERM_ID_INVALID		"00000001"

#define MAX_PACK_SIZE		1024
 
typedef enum 
{
	CONN_STATE_IDLE = 0x1,
	CONN_STATE_REQUEST,
	CONN_STATE_DIAL,
	CONN_STATE_FOUND,
	CONN_STATE_WORK,
} connstate_t;

#define CONN_IO_IN      0
#define CONN_IO_OUT 	1
#define CONN_IO_NOT		2
typedef struct
{   
    PanAddr_t PanAddr;
    uint_16     dist;
}valid_neibor_item_t;


#define PHONE_IDLE 0x0
#define PHONE_DIAL 0x1  
#define PHONE_WORK 0x2
#define PHONE_LEAVE 0x3
typedef unsigned char PhoneStatus_t;

typedef struct
{
	char cardName[NMBRDIGIT];
	PanAddr_t mainSite;
	PanAddr_t secondSite;
	time_t timestamp;
	float distancePercent;
} LocateInfo;

typedef struct 
{
	char priority;
	char grouptype;
	short total;
	bool sended;
	bool deleted;
	//time_t timestamp;
	short revtotal;
	char reqtimes;
	char datetime[8];
	vector<string> vgroupitemstr;
	vector<int> vsendtimes;
	vector<int> vrevseq;
} Groups_t;

#endif
