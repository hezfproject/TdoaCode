#ifndef VOICE_DQUEUE
#define  VOICE_DQUEUE
#include "ZComdef.h"

typedef	struct
{
	uint8*  pbuf;
	uint8 itemsize;
	uint8 maxitem;
	uint8 idx;
}voice_dqueue_ctrl_t;

bool voicedqueue_init(voice_dqueue_ctrl_t *h, uint8 maxitem, uint8 itemsize);
bool voicedqueue_destory( voice_dqueue_ctrl_t *h);
bool  voicedqueue_getitem(voice_dqueue_ctrl_t *h, uint8 *p);
bool  voicedqueue_additem(voice_dqueue_ctrl_t *h, uint8 *p);
uint16   voicedqueue_adddata(voice_dqueue_ctrl_t *h, uint8 *p, uint16 len);
uint16   voicedqueue_getalldata(voice_dqueue_ctrl_t *h, uint8 *p);
uint16  voicedqueue_getsize (const voice_dqueue_ctrl_t *h);
uint16  voicedqueue_getmaxsize (const voice_dqueue_ctrl_t *h);
bool  voicedqueue_isfull (const voice_dqueue_ctrl_t *h);
bool  voicedqueue_isempty (const voice_dqueue_ctrl_t *h);
bool   voicedqueue_flush(voice_dqueue_ctrl_t *h);

#endif
