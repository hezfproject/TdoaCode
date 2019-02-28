#ifndef _APP_GROUP_MSG_H_
#define _APP_GROUP_MSG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define GROUP_NUMBER_LENGTH 4
#define GROUP_NUMBER "1000"
#define GROUP_NUMBER_TEMP_SECOND "7000"
#define GROUP_NUMBER_SECOND "7001"

#define GROUP_ITEM_NUMBER_LENGTH 2
#define GROUP_ITEM_NUMBER_STR_LENGTH 4
#define GROUP_ITEM_PACKET_MAX 50
enum
{
	GROUP_CMD_DIAL,
	GROUP_CMD_FOUND,
	GROUP_CMD_BUSY,
	GROUP_CMD_ACCEPT,
	GROUP_CMD_CLOSE,
	GROUP_CMD_AUDIO,//5
	GROUP_CMD_ITEM_MSG,
	GROUP_CMD_START_WARN,
	GROUP_CMD_INSERT,
	GROUP_CMD_AUDIO_INSERT,
	GROUP_CMD_INSERT_DIAL,
	GROUP_CMD_INSERT_WARN_CANCE,
	GROUP_CMD_INSERT_CANCE,
	GROUP_CMD_REQUEST_ITEMS,
	GROUP_CMD_REQUEST_CLOSE,
	GROUP_CMD_SINGLE_CALL,
	GROUP_CMD_DELETE,
	GROUP_CMD_CHECK,
	GROUP_CMD_GROUPNUM,
};
enum {
	GROUPTYPE_ALL,
	GROUPTYPE_DEPARTMENT,
	GROUPTYPE_CLASS,
	GROUPTYPE_PERSON,
	GROUPTYPE_INSERT,
	GROUPTYPE_TEMP,
	GROUPTYPE_SINGLE_DIAL,
};
typedef struct 
{
	int msgtype;
	short total;
	short data;//seq /req item times
	char groupnumber[GROUP_NUMBER_LENGTH];
	char dstnumber[GROUP_NUMBER_LENGTH];
	char datetime[8];
} app_group_msg_head_t;
enum {
	APP_GROUP_TYPE_ALL,
	APP_GROUP_TYPE_DEPARTMENT,
	APP_GROUP_TYPE_CLASS,
	APP_GROUP_TYPE_PERSON,
	APP_GROUP_TYPE_INSERT,
	APP_GROUP_TYPE_TEMP,
	APP_GROUP_TYPE_SINGLE_DIAL,
};

#define GROUP_ITEM_REQ_TIMES 3
typedef struct 
{
	char priority;
	char grouptype;
	short total;
	char callernumber[GROUP_ITEM_NUMBER_STR_LENGTH];
	char state;
	int coutforclose;
	char datetime[8];
} Group_t;
#ifdef __cplusplus
}
#endif

#endif

