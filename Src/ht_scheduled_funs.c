#include "ht_scheduled_funs.h"

static uint32_t ht_ticks[MAX_SCHEDULED_FUNS] = {0};
static uint32_t ht_tick_wait[MAX_SCHEDULED_FUNS] = {0}; // 0 means not waiting
static fnptr_t scheduled_funs[MAX_SCHEDULED_FUNS] = {0};

void HT_RunScheduledFuns()
{
  for (int i = 0; i < MAX_SCHEDULED_FUNS; ++i) {
      // if it is really function and its time has come
      if(scheduled_funs[i] && ++ht_ticks[i] >= ht_tick_wait[i]) {
          ht_ticks[i] = 0;
          scheduled_funs[i]();
      }
  }
}

bool HT_ScheduleFun(fnptr_t f, uint32_t ms_delay)
{
    if (ms_delay > 0) {
        // searching for the first 'free slot'
        for(int i = 0; i < MAX_SCHEDULED_FUNS; ++i) {
            if(!scheduled_funs[i]) {
                ht_tick_wait[i] = ms_delay;
                scheduled_funs[i] = f;
                return true;
            }
        }
        // error: too much scheduled funs
        return false;
    }
    // error: incorrect delay
    return false;
}

bool HT_UnScheduleFun(fnptr_t f)
{
    if (f) {
        // searching for the matching function
        for(int i = 0; i < MAX_SCHEDULED_FUNS ; ++i) {
            if(scheduled_funs[i] == f) {
                ht_tick_wait[i] = 0;
                scheduled_funs[i] = 0;
                return true;
            }
        }
        // this addr is not scheduled for execution (or just invalid)
        return false;
    }
    // incorrect funptr_t f
    return false;
}
