#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <pthread.h>

#include "ConnectionStringPrv.h"
#include "KegItem.h"
#include "KegMaster.h"
#include "epoll_timerfd_utilities.h"
#include "azure_iot_utilities.h"


// Temp
void TestPeriodic(EventData* e);
int epollFd = -1;
int TimerFd = -1;


KegMaster_obj* e;
EventData TEventData = { .eventHandler = &TestPeriodic };
extern KegMaster_obj* km[4];

static int pt_AzureIotPeriodic(void);

int main(void)
{
    pthread_t pt_AzureIot;
    int pt_AzureRet;

    int fd1 = GPIO_OpenAsOutput(9, GPIO_OutputMode_PushPull, GPIO_Value_High);
	int fd2 = GPIO_OpenAsOutput(10, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if ( fd1 < 0  || fd2 < 0 ) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

	AzureIoT_SetupClient();
    pt_AzureRet = pthread_create(&pt_AzureIot, NULL, pt_AzureIotPeriodic, NULL);

	KegMaster_initRemote();
	KegMaster_initLocal();
	KegMaster_initProcs();
	KegMaster_RequestKegData(0, NULL);
	
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
         if (km[0] != NULL) {
             km[0]->run(km[0]);
         }
         else {
             /* Slow down the processing to try and help avoid racking up a huge bill if I make a mistake */
             const struct timespec sleepTime = { 30, 0 };
             nanosleep(&sleepTime, NULL);

             // TODO - re-request all data periodically
             KegMaster_RequestKegData(0, NULL);
         }
         // TODO: These should be threads
         if (km[1] != NULL) {
             km[1]->run(km[1]);
         }
         // TODO: These should be threads
         if (km[2] != NULL) {
             km[2]->run(km[2]);
         }
         // TODO: These should be threads
         if (km[3] != NULL) {
             km[3]->run(km[3]);
         }

		 value += 1;
		 value %= 8;

		//GPIO_GetValue(fd0, &value);
		GPIO_SetValue(fd1, (GPIO_Value)(value&2)==0);
		GPIO_SetValue(fd2, (GPIO_Value)(value&4)==0);

        {
        
        /* Slow down the processing to try and help avoid racking up a huge bill if I make a mistake */
        const struct timespec sleepTime = { 1, 0 };
        nanosleep(&sleepTime, NULL);
        }
    }
}

static int pt_AzureIotPeriodic(void) {
    int fd0 = GPIO_OpenAsOutput(8, GPIO_OutputMode_PushPull, GPIO_Value_High);
    const struct timespec sleepTime = { 10, 0 };
    int value;

    if (fd0 < 0) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
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