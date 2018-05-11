#include "stdbool.h"
#include "stdint.h"

#define MAX_SCHEDULED_FUNS 1

typedef void(*fnptr_t)();

void HT_RunScheduledFuns();

bool HT_ScheduleFun(fnptr_t f, uint32_t ms_delay);

bool HT_UnScheduleFun(fnptr_t f);

