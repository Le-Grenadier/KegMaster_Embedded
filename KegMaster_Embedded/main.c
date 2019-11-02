#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <pthread.h>

#include "azure_iot_utilities.h"
#include "ConnectionStringPrv.h"
#include "epoll_timerfd_utilities.h"
#include "KegItem.h"
#include "KegMaster.h"
#include "SateliteIntf.h"


// Temp
void TestPeriodic(EventData* e);
int epollFd = -1;
int TimerFd = -1;


KegMaster_obj* e;
EventData TEventData = { .eventHandler = &TestPeriodic };
extern KegMaster_obj** km;
extern int             km_cnt;

static void *pt_AzureIotPeriodic(void* args);

int main(void)
{
    pthread_t pt_AzureIot;

	int fd2 = GPIO_OpenAsOutput(10, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if ( fd2 < 0 ) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

	AzureIoT_SetupClient();
    pthread_create(&pt_AzureIot, NULL, pt_AzureIotPeriodic, NULL);
    
	KegMaster_initRemote();
	KegMaster_initLocal();
	KegMaster_initProcs();
	KegMaster_RequestKegData(0, NULL);
	
    Satellite_Init();


	/* Init polling handler */
	epollFd = CreateEpollFd();
	if (epollFd < 0) {
		return -1;
	}

	/* Polling timer to refresh KegItem data */
	struct timespec tperiod = { 1, 0 };
	epollFd = CreateEpollFd();
	TimerFd = CreateTimerFdAndAddToEpoll(epollFd, &tperiod, &TEventData, EPOLLIN);
	if (TimerFd < 0) {
		return -1;
	}


    while (true) {
        int value;

         // TODO: These should be threads
        for (int i = 0; i < km_cnt; i++) {
            if (km[i] != NULL && km[i]->run != NULL) {
                km[0]->run(km[0]);
            }
            else {
                /* For now, Tap numbers may not be skipped -- but the data may be recieved out of order */
                KegMaster_RequestKegData(i, NULL);
            }
        }
		 value += 1;
		 value %= 8;

		//GPIO_GetValue(fd0, &value);
		GPIO_SetValue(fd2, (GPIO_Value)(value&4)==0);

        {
        
        /* Slow down the processing to try and help avoid racking up a huge bill if I make a mistake */
        const struct timespec sleepTime = { 1, 0 };
        nanosleep(&sleepTime, NULL);
        }
    }
}

static void *pt_AzureIotPeriodic(void* args) {
    int fd0 = GPIO_OpenAsOutput(8, GPIO_OutputMode_PushPull, GPIO_Value_High);
    const struct timespec sleepTime = { 10, 0 };
    int value;

    if (fd0 < 0) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return NULL;
    }

    while (1) {
        GPIO_SetValue(fd0, (GPIO_Value)(value++ % 2));
        AzureIoT_DoPeriodicTasks();
        nanosleep(&sleepTime, NULL);
    }
}




void TestPeriodic(EventData* eventData)
{

	if (ConsumeTimerFdEvent(TimerFd) != 0) {
		//terminationRequired = true;
		return;
	}

	//e->value_refresh(e);
	//e->value_proc(e, NULL);
}