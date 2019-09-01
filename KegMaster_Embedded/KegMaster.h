/*
 * Hardware Access interface description for Keg Master
 */

#pragma once

#include "epoll_timerfd_utilities.h"

typedef enum
{
	KM_HW,
	KM_SW,
	KM_SWHW // SW drives HW feedback avail
}KegMaster_DataSource;

int KegMaster_initRemote(void);
int KegMaster_initLocal(void);
int KegMaster_initProcs(void);


// Temp
void TestPeriodic(EventData* e);