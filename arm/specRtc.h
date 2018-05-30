#ifndef SPECRTC_H_INCLUDED
#define SPECRTC_H_INCLUDED

#include "system.h"

void RTC_Init();
bool RTC_GetTime(tm *newTime);
bool RTC_SetTime(tm *newTime);
void RTC_Update();

#endif
