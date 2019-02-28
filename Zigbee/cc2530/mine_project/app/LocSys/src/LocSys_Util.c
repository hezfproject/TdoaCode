#include "LocSys_Util.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"

uint32 Util_Add320us(uint32 a, uint32 b)
{
  a += b;
  if(a >= MAX_BACKOFF_TIMER_COUNT) a -= MAX_BACKOFF_TIMER_COUNT;
  return a;
}
uint32 Util_Sub320us(uint32 a, uint32 b)
{
  a += MAX_BACKOFF_TIMER_COUNT - b;
  if(a >= MAX_BACKOFF_TIMER_COUNT) a -= MAX_BACKOFF_TIMER_COUNT;
  return a;
}

static uint16 sleep_bitmap[16] = {0};
void Util_DisableSleep(uint8 taskid, uint8 holdid)
{
  sleep_bitmap[taskid] |= (uint16)1 << holdid;
  osal_pwrmgr_task_state(taskid, PWRMGR_HOLD);
}

void Util_EnableSleep(uint8 taskid, uint8 holdid)
{
  sleep_bitmap[taskid] &= ~((uint16)1 << holdid);
  if(sleep_bitmap[taskid] == 0){
    osal_pwrmgr_task_state(taskid, PWRMGR_CONSERVE);
  }
}

