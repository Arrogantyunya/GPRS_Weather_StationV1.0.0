#ifndef _PRIVATE_RTC_H
#define _PRIVATE_RTC_H

#include <RTClock.h>
#include "User_Clock.h"

extern UTCTimeStruct RtcTime;
extern RTClock InRtc;

void Init_RTC(unsigned int init_time);
void Update_RTC(void);
void Get_RTC(unsigned char *buffer);
void RTC_Interrupt(void);

#endif