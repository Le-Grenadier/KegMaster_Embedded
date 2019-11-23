#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
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
#include "SatelliteIntf.h"

/*-----------------------------------------------------------------------------
Uncomment and set this to debug one keg at a time (at the specified index). 
 - Else the debugger gets confused when running multiple threads 
 -----------------------------------------------------------------------------*/
//#define DEBUG_KEG_NO 0


// Temp
void TestPeriodic(EventData* e);
int epollFd = -1;
int TimerFd = -1;


KegMaster_obj* e;
EventData TEventData = { .eventHandler = &TestPeriodic };
extern KegMaster_obj** km;
extern int             km_cnt;

static void* pt_AzureIotPeriodic(void* args);
static void* pt_KegRequest(void* args);

int main(void)
{
    pthread_t pt_AzureIot;
    bool requestComplete = true;
    int numKegs = 0;
    pthread_t* kegThreads;

	int fd2 = GPIO_OpenAsOutput(10, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if ( fd2 < 0 ) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

    /*-----------------------------------------------------
    Init Azure Services 
    -----------------------------------------------------*/
    /* Azure IoT Client */
	AzureIoT_SetupClient();
    pthread_create(&pt_AzureIot, NULL, pt_AzureIotPeriodic, NULL);
    
    /* NTP Time Sync */
    Networking_TimeSync_SetEnabled(true);
    tzset();

    /*-----------------------------------------------------
    Init Keg Master and request first data 
        - TODO: Find more robust way to request keg data
    -----------------------------------------------------*/
	KegMaster_initRemote();
	KegMaster_initLocal();
	KegMaster_initProcs();
	
    /*-----------------------------------------------------
    Init I2C Communication with Satellite PIC MCUs
    -----------------------------------------------------*/
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

    kegThreads = malloc(sizeof(pthread_t));
    while (true) {
        struct {
            int idx;
            KegMaster_obj** e;
            bool* requestComplete;
              } args;
        int value;
        KegMaster_obj* keg;

        // NOTE: This pattern will fail spectacularly if we run out of RAM.
        // Also Note: This is likely to be upwards of 1,000 kegs.
        // TODO: Handle the above better.
        // Also TODO: Don't run out of RAM.
        if (requestComplete) {
            requestComplete = false;

            /* If current keg is populated, move to next */

            #if defined(DEBUG_KEG_NO)
            numKegs = DEBUG_KEG_NO;
            #endif
            keg = KegMaster_getKegPtr(&km, numKegs);
            numKegs = numKegs + (keg != NULL && keg->fields != NULL);
            keg = KegMaster_getKegPtr(&km, numKegs);

            kegThreads = realloc(kegThreads, (size_t)(sizeof(pthread_t) * (numKegs + 1)));
            args.idx = numKegs;
            
            args.e = &km[args.idx];
            args.requestComplete = &requestComplete;

            pthread_create(&kegThreads[args.idx], NULL, pt_KegRequest, &args);
        }
        sleep(30);

		value += 1;
		value %= 8;
		GPIO_SetValue(fd2, (GPIO_Value)(value&4)==0);
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

void *pt_KegRequest(void* args) {
    struct {
        int idx;
        KegMaster_obj** e;
        bool* requestComplete;
    } params;
    KegMaster_obj* self;
    int seconds;

    memcpy( &params, args, sizeof(params));

    /*-----------------------------------------------------
    Request data and block for up to 90 minutes (To help
    reduce IoT message usage).
    -----------------------------------------------------*/
    KegMaster_RequestKegData(params.idx, NULL);

    seconds = 90 * 60;
    /* Visual studio defaults data to 0xFF in debug builds */
    while (seconds > 0 && (params.e == NULL || params.e == 0xFFFFFFFF || (*params.e)->fields == NULL)) {
        seconds--;
        sleep(1);
    }

    /* Signal to main that we're done waiting - Don't request more kegs if we're debugging */
    #if !defined(DEBUG_KEG_NO)
    *params.requestComplete = true;
    #endif

    self = *params.e;
    while (params.e != NULL && self->run != NULL) {
        self->run(self);
    }

    pthread_exit(0);
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