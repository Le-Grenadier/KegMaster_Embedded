/*
 * Hardware Access interface for Keg Master
 */

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <applibs/log.h>
#include "azure_iot_utilities.h"
#include "azureiot/iothub_device_client_ll.h"
#include "epoll_timerfd_utilities.h"
#include "parson.h"

#include "ConnectionStringPrv.h"
#include "KegMaster.h"
#include "KegItem.h"


/* Available Data  */
 KegMaster_FieldDefType KegMaster_FieldDef[] =
	{
	/*------------------------------------------------------------------------------*/
	/* Order dependent for ease of indexing, I'll probably remove it in the morning */
	/* Only one dependency allowed per field atm									*/		
	/*------------------------------------------------------------------------------*/
	/*  This field			KegItem_ValueType,		update-cb					processing-cb				Update Rate */	
	{ "Id",				    KegItem_TypeSTR,		NULL,						NULL,		        		0.0f			}, /* KegMaster_FieldId	*/
		{ "Alerts",				KegItem_TypeSTR,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdAlerts*/
	{ "TapNo",				KegItem_TypeINT,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdTapNo */
	{ "Name",				KegItem_TypeSTR,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdName */
	{ "Style",				KegItem_TypeSTR,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdStyle */
	{ "Description",		KegItem_TypeSTR,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdDescription */
	{ "DateKegged",			KegItem_TypeDATE,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdDateKegged */
	{ "DateAvail",			KegItem_TypeDATE,		NULL,						KegItem_ProcDateAvail,      60.0f*60.0f			}, /* KegMaster_FieldIdDateAvail */
	{ "PourEn",				KegItem_TypeBOOL,		NULL,						KegItem_ProcPourEn,         5.0f			}, /* KegMaster_FieldIdPourEn */
		{ "PourNotification",	KegItem_TypeBOOL,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdPourNotify */
	{ "PourQtyGlass",		KegItem_TypeFLOAT,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdPourQtyGlass */
		{ "PourQtySample",		KegItem_TypeFLOAT,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdPourQtySample */
	{ "PressureCrnt",		KegItem_TypeFLOAT,		KegItem_HwGetPressureCrnt,	KegItem_ProcPressureCrnt,	15.0f			}, /* KegMaster_FieldIdPressureCrnt */
	{ "PressureDsrd",		KegItem_TypeFLOAT,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdPressureDsrd */
    { "PressureDwellTime",	KegItem_TypeFLOAT,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdPressureDwellTime */
	{ "PressureEn",			KegItem_TypeBOOL,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdPressureEn */
	{ "QtyAvailable",		KegItem_TypeFLOAT,		KegItem_HwGetQtyAvail,	    NULL,                       15.0f			}, /* KegMaster_FieldIdQtyAvailable */
	{ "QtyReserve",			KegItem_TypeFLOAT,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdQtyReserve */
	{ "Version",			KegItem_TypeSTR,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdVersion */
	{ "CreatedAt",			KegItem_TypeDATE,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdCreatedAt */
	{ "UpdatedAt",			KegItem_TypeDATE,		NULL,						NULL,						15.0f			}, /* KegMaster_FieldIdUpdatedAt */
	{ "Deleted",			KegItem_TypeBOOL,		NULL,						NULL,					    15.0f			}  /* KegMaster_FieldIdDeleted */
	};

 extern KegMaster_obj* km;

int KegMaster_initRemote()
{
	// Tell the system about the callback function that gets called when we receive a device twin update message from Azure
	//AzureIoT_SetDeviceTwinUpdateCallback(&deviceTwinChangedHandler);

	return 0;
}

int KegMaster_initLocal()
{


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
	AzureIoT_SetMessageReceivedCallback(KegMaster_createKeg);

	return(0);
}

int KegMaster_dbGetKegData(void)
{
	return(true);
}

void KegMaster_createKeg(const char* sqlRow)
{
	KegMaster_obj* keg;
	KegItem_obj* ki;
	char* jsonKey;
	JSON_Value* jsonRoot = NULL;
	JSON_Array* jsonArray = NULL;
	JSON_Object* jsonObj = NULL;
	JSON_Value* jsonElem = NULL;
	int i; 

	/* Build Keg in memory and initialize fields */
	keg = malloc(sizeof(KegMaster_obj));
	memset(keg, 0, sizeof(KegMaster_obj)); /* Important for null comparisons */

	keg->field_add = KegMaster_fieldAdd;
	keg->field_GetByKey = KegMaster_getFieldByKey;
	keg->field_getJson = KegMaster_getJson;
	keg->run = KegMaster_execute;

	/* Build Keg Definition from provided JSON */
	jsonRoot = json_parse_string(sqlRow);
	if (jsonRoot == NULL) {
		Log_Debug("WARNING: Cannot parse the string as JSON content.\n");
	}
	jsonArray = json_value_get_array(jsonRoot);
	jsonObj = json_array_get_object(jsonArray, 0); /* Azure function returns 0-??? number of rows */
	for (i = 0; i < cntOfArray(KegMaster_FieldDef); i++)
	{
		jsonKey = KegMaster_FieldDef[i].name;
		jsonElem = json_object_get_value(jsonObj, jsonKey);
		if (jsonElem == NULL)
		{
			continue;
		}

		ki = keg->field_add(keg, KegMaster_FieldDef[i].name);
		switch (ki->value_type)
		{
			//bool type_bool;
			float type_float;
			int type_int;
			const char* type_str;
            bool type_bool;

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


	km = keg;
}

int KegMaster_execute(KegMaster_obj* self)
{
	KegItem_obj* e;
	char* c; 

	e = self->fields;
	if (e == NULL)
	{
		return( false );
	}

	do
	{
        if (e->value_refresh != NULL) {
            e->value_refresh(e);
        }

		if(e->value_proc != NULL)
		{
			char* j;
			char* s;
			size_t sz;
			e->value_proc(e);

            if(e->value_dirty){
                /* gather JSON and for later SQL db update */
                j = e->toJson(e);
                sz = self->fields_json == NULL ? 0 : strlen(self->fields_json);
                sz += +strlen(j) + 1;
                s = malloc(sz);
                memset(s, 0, sz);
                if (self->fields_json != NULL) { /* Add a comma for multiple fields */
                    s = strcat(self->fields_json, ",");
                }
                s = strcat(s, j);
                free(self->fields_json);
                free(j);
                self->fields_json = s;
                }
		}

		e = e->next;
	} while (e != self->fields);

	c = self->field_getJson(self);
    if(c) {
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
    newItem = KegItem_init(newItem, fieldDef->update, fieldDef->update_rate, fieldDef->proc );

    self->fields = (self->fields == NULL) ? newItem : KegItem_ListInsertBefore(self->fields, newItem);

	return(newItem);
}


KegItem_obj* KegMaster_getFieldByKey(KegMaster_obj* self, char* key)
{
	KegItem_obj* this = NULL;
	KegItem_obj* item;
	assert(key != NULL);

	this = self->fields;
	do {
		if (0 == memcmp(key, this->key, strlen(key))) {
			item = this;
			break;
		}
		this = this->next;
	} while (this != self->fields);

	return(item);
}

/* Calling this function destroys cached data cache after passing it back */
char* KegMaster_getJson(KegMaster_obj* self)
{
	static const char* JSON_GRP = "{%s, %s}";
	size_t s;
	char* c;
	KegItem_obj* item;
	item = self->field_GetByKey(self, "Id");
	char* id = item->toJson(item);

    if (self->fields_json == NULL) {
        return(NULL);
    }

	s = strlen(self->fields_json) + strlen(id) + strlen(JSON_GRP);
	c = malloc(s);
	snprintf(c, s, JSON_GRP, id, self->fields_json);
	free(id);
	free(self->fields_json);
	self->fields_json = NULL;
	//char *json_serialize_to_string(const JSON_Value *value);

	return(c);
}


void KegMaster_RequestKegData(int tapNo)
{
	static const char* JSON_GRP = "{\"ReqId\": \"none\", \"TapNo\":\"%d\"}";
	size_t s;
	char* c;
	s = strlen(JSON_GRP) + 4 /* up to 9999 Taps? ^_^ */;
	c = malloc(s);
	snprintf(c, s, JSON_GRP, tapNo);
	AzureIoT_SendMessage(c);
}