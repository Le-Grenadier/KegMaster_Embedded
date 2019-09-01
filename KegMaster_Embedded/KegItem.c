#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "azureiot/iothub_message.h"
#include <azureiot/iothub_device_client_ll.h>
#include "epoll_timerfd_utilities.h"
#include "KegItem.h"


#include "azure_iot_utilities.h"
#include "connection_strings.h"


int epollFd = -1;
static int TimerFd = -1;
static const char* test = "PressureCrnt";

static void TestPeriodic(EventData* e);
KegItem_obj* e;
EventData TEventData = { .eventHandler = &TestPeriodic };

int testrun(void)
{
	e = KegItem_CreateNew(test, KegItem_FLOAT);
	KegItem_init(e,
		KegItem_HwGetPressureCrnt,//KegItem_DbGetPressureCrnt, //self->refresh_fromDb = refresh_fromDb;
		NULL, //self->refresh_fromDb = refresh_fromDb;
		15,/*refresh every second(s)*/
		KegItem_CleanDbPressureCrnt //self->value_clean = value_clean;
		);
	// Set up a timer to poll the buttons
	struct timespec tperiod = { 15, 0 };
	epollFd = CreateEpollFd();
	TimerFd = CreateTimerFdAndAddToEpoll(epollFd, &tperiod, &TEventData, EPOLLIN);
	return(true);
}

static void TestPeriodic(EventData* eventData)
{

	if (ConsumeTimerFdEvent(TimerFd) != 0) {
		//terminationRequired = true;
		return;
	}
	e->refresh_fromHw(e);
	e->value_clean(e);
}











/*=============================================================================
KegItem Object Constructors
=============================================================================*/

/*-----------------------------------------------------------------------------
 * Create new Keg Item and prepare for initialization
 -----------------------------------------------------------------------------*/
KegItem_obj* KegItem_CreateNew(
	const char* key,
	KegItem_ValueType type
	)
{
	static const struct
	{
		KegItem_funcSet* setVal;
		KegItem_funcChr* getStr;
	} valSet_funcs[KegItem_TYPECNT] =
	{
	{ NULL,	NULL},
	{ kegItem_setFloat, kegItem_formatFloat },
	//{ kegItem_setInt, kegItem_formatInt },
	//{ kegItem_setDateTime, kegItem_formatDateTime },
	//{ kegItem_setBool, kegItem_formatBool }
	};

	KegItem_obj* e;
	e = malloc(sizeof(KegItem_obj));
	memset(e, 0, sizeof(KegItem_obj));

	e->init = &KegItem_init;
	e->key = malloc(sizeof(key));
	strcpy(e->key, key);

	assert(type != KegItem_NONE && type <= KegItem_TYPECNT );
	e->value_set = valSet_funcs[type].setVal;
	e->value_toString = valSet_funcs[type].getStr;
	e->toJson = KegItem_toJson;

	return(e);
}

/*-----------------------------------------------------------------------------
 * (re)Init KegItem
 * 
 * Refresh from either Hw or Db, simultaneous use of both not currently supported.
 -----------------------------------------------------------------------------*/
KegItem_obj* KegItem_init(
	KegItem_obj* self,
	KegItem_funcInt* refresh_fromHw,
	KegItem_funcInt* refresh_fromDb,
	unsigned int refresh_period,
	//KegItem_funcInt* update_Db,
	KegItem_funcInt* value_clean
	)
{
	assert(((refresh_fromDb != NULL) ^
		(refresh_fromHw != NULL)) &&
		(value_clean != NULL) );

	/* Copy functions into object */
	self->refresh_fromDb = refresh_fromDb;
	self->refresh_fromHw = refresh_fromHw;
	self->refresh_period = refresh_period;
	self->value_clean = value_clean;

	/* Perform initial refresh of data */
	self->value_dirty = self->refresh_fromDb != NULL ? self->refresh_fromDb(self) : self->refresh_fromHw(self);
	self->value_clean(self);
	return(self);
}

/*=============================================================================
KegItem Object Value Setter functions
=============================================================================*/
//__inline KegItem_funcSet kegItem_setInt;
int kegItem_setFloat(KegItem_obj* self, void* value)
{
	if (self->value == NULL)
	{
		self->value = malloc(sizeof(float));
	}
	memset(self->value, 0, sizeof(float));
	*((float*)self->value) = *(float*)value;
	
	return(true);
};
//__inline KegItem_funcSet kegItem_setDateTime;
//__inline KegItem_funcSet kegItem_setBool;

/*=============================================================================
KegItem Object Value Formatting functions
 - char* must be free'd after use
=============================================================================*/

//__inline KegItem_funcChr kegItem_formatInt;
char* kegItem_formatFloat(KegItem_obj* self) {
	const char* FMT = "%3.3f";
	const size_t PRECISION = 3/*digits*/ + 3/*decimal*/ + 1/*space for decimal*/;
	char* c;

	c = (char*)malloc(PRECISION);
	memset(c, 0, sizeof(PRECISION));
	snprintf(c, PRECISION, FMT, *(float*)self->value);
	return(c);
}
//__inline KegItem_funcChr kegItem_formatDateTime;
//__inline KegItem_funcChr kegItem_formatBool;

/*=============================================================================
KegItem Object Key:Value to JSON
 - char* must be free'd after use
=============================================================================*/
char* KegItem_toJson(KegItem_obj* self) {
	const char* JSON_FMT = "{\"%s\":\"%s\"}";
	char* c;
	size_t key_len = strlen(self->key);
	char* val_s = self->value_toString(self);
	size_t val_len = strlen(val_s);
	unsigned int mal_sz = strlen(JSON_FMT) /*- 4%s and all that*/ + key_len + val_len;

	c = malloc(mal_sz);
	memset(c, 0, sizeof(mal_sz));
	snprintf(c, mal_sz, JSON_FMT, self->key, val_s);
	free(val_s);
	return(c);
}

/*=============================================================================
KegItem Hw access interface
=============================================================================*/
int KegItem_HwGetPressureCrnt(KegItem_obj* self)
{
	if (self->value == NULL)
	{
		float f;
		f = 0.0f;
		self->value_set(self, &f);
	}
	(*(float*)self->value)++;
	return(self->value_set(self, ((float*)self->value)));
}

/*=============================================================================
KegItem Db access interface
=============================================================================*/
//static int KegItem_DbGetPressureCrnt(KegItem_obj * self)
//{
//	(*(float*)self->value)++;
//	self->value_set(self, ((float*)self->value));
//	return(true);
//}

/*=============================================================================
KegItem data processing callback 'cleaner' functions
=============================================================================*/
int KegItem_CleanDbPressureCrnt(KegItem_obj* self)
{	
	char* p = self->toJson(self);
	//AzureIoT_SendMessage(p);
	free(p);
	return(true);
}
