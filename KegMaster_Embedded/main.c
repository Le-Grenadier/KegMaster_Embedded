#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/gpio.h>

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


int main(void)
{
	KegMaster_obj* km;

    // This minimal Azure Sphere app repeatedly toggles GPIO 9, which is the green channel of RGB
    // LED 1 on the MT3620 RDB.
    // Use this app to test that device and SDK installation succeeded that you can build,
    // deploy, and debug an app with Visual Studio, and that you can deploy an app over the air,
    // per the instructions here: https://docs.microsoft.com/azure-sphere/quickstarts/qs-overview
    //
    // It is NOT recommended to use this as a starting point for developing apps; instead use
    // the extensible samples here: https://github.com/Azure/azure-sphere-samples
    Log_Debug(
        "\nVisit https://github.com/Azure/azure-sphere-samples for extensible samples to use as a "
        "starting point for full applications.\n");

	int fd0 = GPIO_OpenAsOutput(8, GPIO_OutputMode_PushPull, GPIO_Value_High);
    int fd1 = GPIO_OpenAsOutput(9, GPIO_OutputMode_PushPull, GPIO_Value_High);
	int fd2 = GPIO_OpenAsOutput(10, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (fd0 < 0 || fd1 < 0  || fd2 < 0 ) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

    const struct timespec sleepTime = {10, 0};
	AzureIoT_SetupClient();

	KegMaster_initRemote();
	KegMaster_initLocal();
	KegMaster_initProcs();
	km = KegMaster_createKeg(KEG_GUID_PRV);
	km->field_add(km, "PressureCrnt");
	
	/* Init polling handler */
	epollFd = CreateEpollFd();
	if (epollFd < 0) {
		return -1;
	}

	/* Polling timer to refresh KegItem data */
	struct timespec tperiod = { 5, 0 };
	epollFd = CreateEpollFd();
	TimerFd = CreateTimerFdAndAddToEpoll(epollFd, &tperiod, &TEventData, EPOLLIN);
	if (TimerFd < 0) {
		return -1;
	}


    while (true) {
		 int value; 
		
		 km->run(km);

		 value += 1;
		 value %= 8;

		//GPIO_GetValue(fd0, &value);
		GPIO_SetValue(fd0, (GPIO_Value)(value&1)==0);
		GPIO_SetValue(fd1, (GPIO_Value)(value&2)==0);
		GPIO_SetValue(fd2, (GPIO_Value)(value&4)==0);

        nanosleep(&sleepTime, NULL);
        //GPIO_SetValue(fd, GPIO_Value_High);
        //nanosleep(&sleepTime, NULL);


		//{
		//	extern int epollFd;
		//	WaitForEventAndCallHandler(epollFd);
		//}

		// AzureIoT_DoPeriodicTasks() needs to be called frequently in order to keep active
		// the flow of data with the Azure IoT Hub
		AzureIoT_DoPeriodicTasks();
		/*
		Get data from azure
		 - contains info about number of kegs
		Parse data into 'Kegs'
		 - each keg => thres
		Each keg item runs through fields and updates 
		*/
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