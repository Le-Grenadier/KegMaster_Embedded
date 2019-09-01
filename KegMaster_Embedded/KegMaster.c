/*
 * Hardware Access interface for Keg Master
 */

#include <signal.h>
#include <string.h>

#include "KegMaster.h"
#include "KegItem.h"

#include "epoll_timerfd_utilities.h"

/* Available Data  */
//const char* KegItem_Fields[] =
//	{
//	{ KM_SW, "Alerts",				},
//	{ KM_HW, "TapNo",				},
//	{ KM_SW, "Name",				},
//	{ KM_SW, "Style",				},
//	{ KM_SW, "Description",			},
//	{ KM_SW, "DateKegged",			},
//	{ KM_SW, "DateAvail",			},
//	{ KM_SW, "PourEn",				},
//	{ KM_SW, "PourNotification",	},
//	{ KM_SW, "PourQtyGlass",		},
//	{ KM_SW, "PourQtySample",		},
//	{ KM_HW, "PressureCrnt",		},
//	{ KM_SW, "PressureDsrd",		},
//	{ KM_HW, "PressureDwellTime",	},
//	{ KM_HW, "PressureEn",			},
//	{ KM_HW, "QtyAvailable",		},
//	{ KM_SW, "QtyReserve",			},
//	{ KM_SW, "Version",				},
//	{ KM_SW, "CreatedAt",			},
//	{ KM_SW, "UpdatedAt",			},
//	{ KM_SW, "Deleted"				}
//	};

int epollFd = -1;
int TimerFd = -1;
static const char* test = "PressureCrnt";

KegItem_obj* e;
EventData TEventData = { .eventHandler = &TestPeriodic };


int KegMaster_initRemote()
{



	// Tell the system about the callback function that gets called when we receive a device twin update message from Azure
	//AzureIoT_SetDeviceTwinUpdateCallback(&deviceTwinChangedHandler);

	return 0;
}

int KegMaster_initLocal()
{
	e = KegItem_CreateNew(test, KegItem_FLOAT);

	KegItem_init(e,
		KegItem_HwGetPressureCrnt,//KegItem_DbGetPressureCrnt, //self->refresh_fromDb = refresh_fromDb;
		NULL, //self->refresh_fromDb = refresh_fromDb;
		15,/*refresh every second(s)*/
		KegItem_CleanDbPressureCrnt //self->value_clean = value_clean;
	);

	//if (initI2c() == -1) {
	//	return -1;
	//}
	return(0);
}

int KegMaster_initProcs()
{
	/* Custom error handling */
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	//action.sa_handler = TerminationHandler;
	//sigaction(SIGTERM, &action, NULL);

	/* Init polling handler */
	epollFd = CreateEpollFd();
	if (epollFd < 0) {
		return -1;
	}

	/* Polling timer to refresh KegItem data */
	struct timespec tperiod = { 15, 0 };
	epollFd = CreateEpollFd();
	TimerFd = CreateTimerFdAndAddToEpoll(epollFd, &tperiod, &TEventData, EPOLLIN);
	if (TimerFd < 0) {
		return -1;
	}
	return(0);
}







void TestPeriodic(EventData* eventData)
{

	if (ConsumeTimerFdEvent(TimerFd) != 0) {
		//terminationRequired = true;
		return;
	}
	e->refresh_fromHw(e);
	e->value_clean(e);
}