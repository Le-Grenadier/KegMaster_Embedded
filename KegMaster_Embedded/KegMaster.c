/*
 * Hardware Access interface for Keg Master
 */

#include <errno.h>
#include <assert.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <applibs/gpio.h>
#include <applibs/log.h>
#include "azure_iot_utilities.h"
#include "azureiot/iothub_device_client_ll.h"
#include "epoll_timerfd_utilities.h"
#include "parson.h"
#include "unistd.h"

#include "ConnectionStringPrv.h"
#include "KegMaster.h"
#include "KegItem.h"
#include "KegItem_Proc.h"
#include "KegItem_Updt.h"
#include "KegItem_Utl.h"

#define MIN 60.0f
#define SEC 1.0f
#define NO_UPDT 0.0f

KegMaster_obj** km;
int            km_cnt = 0;

/* Available Data  */
 static KegMaster_FieldDefType KegMaster_FieldDef[] =
	{
	/*------------------------------------------------------------------------------*/
	/* Data definitions; update and processing callbacks and rates	    			*/		
    /*                                                                              */
    /* Proc and update rates cannot be less than one second                         */      
	/*------------------------------------------------------------------------------*/
	/*  This field			KegItem_ValueType,		update-cb					processing-cb				update Rate         re-query Rate */	
	{ "Id",				    KegItem_TypeSTR,		NULL,						NULL,		        		   NO_UPDT,		       NO_UPDT      }, /* KegMaster_FieldId	*/
	{ "Alerts",				KegItem_TypeSTR,		NULL,						NULL,           			15*SEC,		           NO_UPDT      }, /* KegMaster_FieldIdAlerts*/
	{ "TapNo",				KegItem_TypeINT,		NULL,						NULL,						15*MIN,		           NO_UPDT      }, /* KegMaster_FieldIdTapNo */
	{ "Name",				KegItem_TypeSTR,		NULL,						NULL,						15*MIN,		           NO_UPDT      }, /* KegMaster_FieldIdName */
	{ "Style",				KegItem_TypeSTR,		NULL,						NULL,						15*MIN,		           NO_UPDT      }, /* KegMaster_FieldIdStyle */
	{ "Description",		KegItem_TypeSTR,		NULL,						NULL,						15*MIN,		           NO_UPDT      }, /* KegMaster_FieldIdDescription */
	{ "DateKegged",			KegItem_TypeDATE,		NULL,						NULL,						15*MIN,		           NO_UPDT      }, /* KegMaster_FieldIdDateKegged */
	{ "DateAvail",			KegItem_TypeDATE,		NULL,						KegItem_ProcDateAvail,       1*MIN,	           120*MIN          }, /* KegMaster_FieldIdDateAvail */
	{ "PourEn",				KegItem_TypeBOOL,		NULL,						KegItem_ProcPourEn,          1*SEC,		       120*MIN          }, /* KegMaster_FieldIdPourEn */
	{ "PourNotification",	KegItem_TypeBOOL,		NULL,						NULL,						15*MIN,		           NO_UPDT      }, /* KegMaster_FieldIdPourNotify */
	{ "PourQtyGlass",		KegItem_TypeFLOAT,		NULL,						NULL,						15*MIN,		       120*MIN          }, /* KegMaster_FieldIdPourQtyGlass */
	{ "PourQtySample",		KegItem_TypeFLOAT,		NULL,						NULL,						15*MIN,		       120*MIN          }, /* KegMaster_FieldIdPourQtySample */
	{ "PressureCrnt",		KegItem_TypeFLOAT,		KegItem_HwGetPressureCrnt,	NULL,                        1*SEC,		           NO_UPDT      }, /* KegMaster_FieldIdPressureCrnt */
	{ "PressureDsrd",		KegItem_TypeFLOAT,		NULL,						KegItem_ProcPressureDsrd,    1*SEC,		       120*MIN          }, /* KegMaster_FieldIdPressureDsrd */
    { "PressureDwellTime",	KegItem_TypeFLOAT,		NULL,						NULL,						15*MIN,		           NO_UPDT      }, /* KegMaster_FieldIdPressureDwellTime */
	{ "PressureEn",			KegItem_TypeBOOL,		NULL,						NULL,						15*MIN,	           120*MIN          }, /* KegMaster_FieldIdPressureEn */
	{ "QtyAvailable",		KegItem_TypeFLOAT,		KegItem_HwGetQtyAvail,	    NULL,                       30*SEC,		           NO_UPDT      }, /* KegMaster_FieldIdQtyAvailable */
	{ "QtyReserve",			KegItem_TypeFLOAT,		NULL,						NULL,						15*MIN,	           120*MIN          }, /* KegMaster_FieldIdQtyReserve */
	{ "Version",			KegItem_TypeSTR,		NULL,						NULL,						15*MIN,		           NO_UPDT      }, /* KegMaster_FieldIdVersion */
	{ "CreatedAt",			KegItem_TypeDATE,		NULL,						NULL,						15*MIN,		           NO_UPDT      }, /* KegMaster_FieldIdCreatedAt */
	{ "UpdatedAt",			KegItem_TypeDATE,		NULL,						NULL,						15*MIN,		           NO_UPDT      }, /* KegMaster_FieldIdUpdatedAt */
	{ "Deleted",			KegItem_TypeBOOL,		NULL,						NULL,					    15*MIN,		           NO_UPDT      }  /* KegMaster_FieldIdDeleted */
	};

 int fd1;

int KegMaster_initRemote()
{
	// Tell the system about the callback function that gets called when we receive a device twin update message from Azure
	//AzureIoT_SetDeviceTwinUpdateCallback(&deviceTwinChangedHandler);

	return 0;
}

int KegMaster_initLocal()
{
    fd1 = GPIO_OpenAsOutput(9, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (fd1 < 0 ) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

    km = NULL;
 
	return(0);
}

int KegMaster_initProcs()
{
	/* Custom error handling */
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	//action.sa_handler = TerminationHandler;
	//sigaction(SIGTERM, &action, NULL);
	AzureIoT_SetMessageReceivedCallback(KegMaster_procAzureIotMsg);

	return(0);
}

int KegMaster_dbGetKegData(void)
{
	return(true);
}


void KegMaster_procAzureIotMsg(const char* sqlRow) {
    JSON_Value* jsonRoot = NULL;
    GPIO_Value_Type io;

    /* Build Keg Definition from provided JSON */
    jsonRoot = json_parse_string(sqlRow);
    if (jsonRoot == NULL) {
        Log_Debug("WARNING: Cannot parse the string as JSON content.\n");
    }
    else {
        KegMaster_updateKeg(jsonRoot, &km, km_cnt);
    }
    
    /* Green LED */
    GPIO_GetValue(fd1, &io);
    GPIO_SetValue(fd1, !io);

}

KegMaster_obj* KegMaster_getKegPtr(KegMaster_obj*** keg_root, int idx) {
    KegMaster_obj* keg;

    if (keg_root == NULL || idx >= km_cnt) {
        /* Build Keg in memory and initialize fields */
        keg = malloc(sizeof(KegMaster_obj));
        memset(keg, 0, sizeof(KegMaster_obj)); /* Important for null comparisons */

        keg->field_add = KegMaster_fieldAdd;
        keg->field_GetByKey = KegMaster_getFieldByKey;
        keg->field_getJson = KegMaster_getJson;
        keg->run = KegMaster_execute;
        keg->queryDb = KegMaster_RequestKegData;

        sem_init(&keg->kegItem_semaphore, 0, 1);

        km_cnt++;
        *keg_root = realloc(*keg_root, sizeof(KegMaster_obj*) * (uint32_t)km_cnt);
        (*keg_root)[idx] = keg;
    }
    return((*keg_root)[idx]);
}

KegMaster_obj* KegMaster_updateKeg(JSON_Value* jsonRoot, KegMaster_obj*** keg_root, int keg_cnt){
    bool result;
    int err;
    KegMaster_obj* keg = NULL;
	KegItem_obj* ki;
	char* jsonKey;
	JSON_Array* jsonArray = NULL;
	JSON_Object* jsonObj = NULL;
	JSON_Value* jsonElem = NULL;
	int i; 

    /* Get Json data */
    jsonArray = json_value_get_array(jsonRoot);
    jsonObj = json_array_get_object(jsonArray, 0);

    /* Get pointer to keg object */
    jsonElem = json_object_get_value(jsonObj, "TapNo");

    keg = KegMaster_getKegPtr(keg_root, (int)json_value_get_number(jsonElem));
    result = sem_wait(&keg->kegItem_semaphore);
    if (result != 0) {
        err = errno;
        Log_Debug("ERROR: Failed to acquire semaphore. KegMaster_execute(): errno=%d (%s)\n", err, strerror(err));
        return (0);
    }

    /* Populate / Update Keg */
	for (i = 0; i < cntOfArray(KegMaster_FieldDef); i++)
	{
		jsonKey = KegMaster_FieldDef[i].name;
		jsonElem = json_object_get_value(jsonObj, jsonKey);
		if (jsonElem == NULL){
			continue;
		}

        /* Update existing data else create new */
        ki = keg->field_GetByKey(keg, jsonKey);
		ki = (ki == NULL) ? keg->field_add(keg, jsonKey) : ki;

		switch (ki->value_type){
			bool type_bool;
			float type_float;
			int type_int;
			const char* type_str;

			case KegItem_TypeFLOAT:
				type_float = (float)json_value_get_number(jsonElem);
				ki->value_set(ki, &type_float);
				break;

			case KegItem_TypeINT:
				type_int = (int)json_value_get_number(jsonElem);
				ki->value_set( ki, &type_int );
				break;

			case KegItem_TypeDATE:
				type_str = json_value_get_string(jsonElem);
				ki->value_set(ki, &type_str);
				break;

			case KegItem_TypeSTR:
				type_str = json_value_get_string(jsonElem);
				ki->value_set(ki, &type_str);
				break;

			case KegItem_TypeBOOL:
				type_bool = json_value_get_boolean(jsonElem);
				ki->value_set(ki, &type_bool);
				break;

			case KegItem_TypeNONE:
			default:
				assert(false);
				break;
		}
	}
    sem_post(&keg->kegItem_semaphore);

    return(keg);
}

int KegMaster_execute(KegMaster_obj* self)
{
    bool result;
    int err;
    KegItem_obj* e;
    KegItem_obj* TapNo;
    char* c;
    bool doQuery;
    bool doProc;
    struct timespec ts;

	e = self->fields;
	if (e == NULL){
		return( false );
	}

    clock_gettime(CLOCK_REALTIME, &ts);

    do {
        doQuery = (e->query_timeNext.tv_sec < ts.tv_sec) && (e->queryPeriod != 0);
        doProc = (e->refresh_timeNext.tv_sec < ts.tv_sec) && (e->refreshPeriod != 0);
        TapNo = self->field_GetByKey(self, "TapNo");

        result = sem_wait(&self->kegItem_semaphore);
        if (result != 0) {
            err = errno;
            Log_Debug("ERROR: Failed to acquire semaphore. KegMaster_execute(): errno=%d (%s)\n", err, strerror(err));
            return (0);
        }

        if (doQuery && TapNo != NULL) {
            e->query_timeNext.tv_sec = ts.tv_sec + e->queryPeriod;
            self->queryDb((*(int*)TapNo->value), e->key);
        }

        if (doProc && e->value_refresh != NULL) {
            e->refresh_timeNext.tv_sec = ts.tv_sec + e->refreshPeriod;
            e->value_refresh(e);
        }

		if(doProc && e->value_proc != NULL){
            e->refresh_timeNext.tv_sec = ts.tv_sec + e->refreshPeriod;
			e->value_proc(e);
		}

        if (e->value_dirty) {
            char* j;
            char* s;
            size_t sz;

            /* gather JSON and for later SQL db update */
            j = e->toJson(e);
            sz = self->fields_json == NULL ? 0 : strlen(self->fields_json);
            sz += +strlen(j) + 1;
            s = malloc(sz);
            memset(s, 0, sz);
            if (self->fields_json != NULL) { /* Add a comma for multiple fields */
                s = strncpy(s, self->fields_json, strlen(self->fields_json));
                s = strcat(s, ",");
            }
            s = strcat(s, j);
            if( self->fields_json != NULL )
                free(self->fields_json);
            free(j);
            self->fields_json = s;

            e->value_dirty = false;
        }
		e = e->next;
        sem_post(&self->kegItem_semaphore);
	} while (e != self->fields);

	c = self->field_getJson(self);
    if(c) {
        Log_Debug("INFO: IoT Hub Message - %s\n", c);
        AzureIoT_SendMessage(c);
        free(c);
    }
	return(true);
}


KegItem_obj* KegMaster_fieldAdd(KegMaster_obj* self, char* field)
{
	KegItem_obj* newItem;
	KegMaster_FieldDefType* fieldDef;

	for (int i = 0; i < sizeof(KegMaster_FieldDef) / sizeof(KegMaster_FieldDef[0]); i++) {
        fieldDef = &KegMaster_FieldDef[i];
		if (0 == strcmp(fieldDef->name, field)) {
			break;
		}
	}
    newItem = KegItem_Create(fieldDef->name, fieldDef->type);
    newItem = KegItem_init(newItem, fieldDef->update, fieldDef->update_rate, fieldDef->proc, fieldDef->queryRate );

    self->fields = (self->fields == NULL) ? newItem : KegItem_ListInsertBefore(self->fields, newItem);

	return(newItem);
}


KegItem_obj* KegMaster_getFieldByKey(KegMaster_obj* self, char* key)
{
	assert(key != NULL);
    return(KegItem_getSiblingByKey(self->fields, key));
}

/* Calling this function destroys cached data cache after passing it back */
char* KegMaster_getJson(KegMaster_obj* self){
	static const char* JSON_GRP = "{%s, %s}";
	size_t s;
	char* c;
	KegItem_obj* item;
	item = self->field_GetByKey(self, "Id");
    if (item == NULL || self->fields_json == NULL) {
        return(NULL);
    }
	char* id = item->toJson(item);

	s = strlen(self->fields_json) + strlen(id) + strlen(JSON_GRP);
	c = malloc(s);
	snprintf(c, s, JSON_GRP, id, self->fields_json);
	free(id);
	free(self->fields_json);
	self->fields_json = NULL;

	return(c);
}


void KegMaster_RequestKegData(int tapNo, char* select){
    #define SELECT_ALL "*"

	static const char* JSON_GRP = "{\"ReqId\": \"none\", \"TapNo\":\"%d\", \"qrySelect\":\"%s\"}";
	size_t s;
	char* c;
    if (select == NULL) {
        select = SELECT_ALL;
    }

	s = strlen(JSON_GRP) + strlen(select) + 4 /* up to 9999 Taps? ^_^ */;
	c = malloc(s);
	snprintf(c, s, JSON_GRP, tapNo, select);

    Log_Debug("INFO: IoT Hub Message - %s\n", c);

    AzureIoT_SendMessage(c);
    free(c);
}