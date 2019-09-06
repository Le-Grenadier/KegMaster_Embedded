/*
 * Hardware Access interface for Keg Master
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ConnectionStringPrv.h"
#include "KegMaster.h"
#include "KegItem.h"

#include "azure_iot_utilities.h"
#include "azureiot/iothub_device_client_ll.h"
#include "epoll_timerfd_utilities.h"

/* Available Data  */
 KegMaster_FieldDefType KegMaster_FieldDef[] =
	{
	/*------------------------------------------------------------------------------*/
	/* Order dependent for ease of indexing, I'll probably remove it in the morning */
	/* Only one dependency allowed per field atm									*/		
	/*------------------------------------------------------------------------------*/
	/*  This field			KegItem_ValueType,		update-cb					processing-cb				Update Rate */	
	{ "Alerts",				KegItem_TypeSTR,		NULL,						NULL,						15			}, /* KegMaster_FieldIdAlerts*/
	{ "TapNo",				KegItem_TypeINT,		NULL,						NULL,						15			}, /* KegMaster_FieldIdTapNo */
	{ "Name",				KegItem_TypeSTR,		NULL,						NULL,						15			}, /* KegMaster_FieldIdName */
	{ "Style",				KegItem_TypeSTR,		NULL,						NULL,						15			}, /* KegMaster_FieldIdStyle */
	{ "Description",		KegItem_TypeSTR,		NULL,						NULL,						15			}, /* KegMaster_FieldIdDescription */
	{ "DateKegged",			KegItem_TypeDATE,		NULL,						NULL,						15			}, /* KegMaster_FieldIdDateKegged */
	{ "DateAvail",			KegItem_TypeDATE,		NULL,						NULL,						15			}, /* KegMaster_FieldIdDateAvail */
	{ "PourEn",				KegItem_TypeBOOL,		NULL,						NULL,						15			}, /* KegMaster_FieldIdPourEn */
	{ "PourNotification",	KegItem_TypeBOOL,		NULL,						NULL,						15			}, /* KegMaster_FieldIdPourNotify */
	{ "PourQtyGlass",		KegItem_TypeFLOAT,		NULL,						NULL,						15			}, /* KegMaster_FieldIdPourQtyGlass */
	{ "PourQtySample",		KegItem_TypeFLOAT,		NULL,						NULL,						15			}, /* KegMaster_FieldIdPourQtySample */
	{ "PressureCrnt",		KegItem_TypeFLOAT,		KegItem_HwGetPressureCrnt,	KegItem_ProcPressureCrnt,	15			}, /* KegMaster_FieldIdPressureCrnt */
	{ "PressureDsrd",		KegItem_TypeFLOAT,		NULL,						NULL,						15			}, /* KegMaster_FieldIdPressureDsrd */
	{ "PressureDwellTime",	KegItem_TypeFLOAT,		NULL,						NULL,						15			}, /* KegMaster_FieldIdPressureDwellTime */
	{ "PressureEn",			KegItem_TypeBOOL,		NULL,						NULL,						15			}, /* KegMaster_FieldIdPressureEn */
	{ "QtyAvailable",		KegItem_TypeFLOAT,		NULL,						NULL,						15			}, /* KegMaster_FieldIdQtyAvailable */
	{ "QtyReserve",			KegItem_TypeFLOAT,		NULL,						NULL,						15			}, /* KegMaster_FieldIdQtyReserve */
	{ "Version",			KegItem_TypeSTR,		NULL,						NULL,						15			}, /* KegMaster_FieldIdVersion */
	{ "CreatedAt",			KegItem_TypeDATE,		NULL,						NULL,						15			}, /* KegMaster_FieldIdCreatedAt */
	{ "UpdatedAt",			KegItem_TypeDATE,		NULL,						NULL,						15			}, /* KegMaster_FieldIdUpdatedAt */
	{ "Deleted",			KegItem_TypeBOOL,		NULL,						NULL,					    15			}  /* KegMaster_FieldIdDeleted */
	};


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


	return(0);
}

int KegMaster_dbGetKegData(void)
{
	return(true);
}

KegMaster_obj* KegMaster_createKeg(char* guid)
{
	KegMaster_obj* km;
	km = malloc(sizeof(KegMaster_obj));
	memset(km, 0, sizeof(KegMaster_obj)); /* Important for null comparisons */

	km->guid = malloc(strlen(guid));
	strcpy(km->guid, guid);

	km->field_add = KegMaster_fieldAdd;
	km->field_GetByKey = NULL;
	km->field_getJson = KegMaster_getJson;


	km->run = KegMaster_execute;

	return(km);
}

int KegMaster_execute(KegMaster_obj* self)
{
	KegItem_obj* e;
	char* c; 

	e = self->fields;
	do
	{
		if (e->value_refresh == NULL) {
			e = e->next;
			continue;
		}

		e->value_refresh(e);
		if (e->value_dirty)
		{
			char* j;
			char* s;
			int sz;
			e->value_proc(e);

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

		e = e->next;
	} while (e != self->fields);

	c = self->field_getJson(self);
	AzureIoT_SendMessage(c);
	free(c);
	return(true);
}


KegItem_obj* KegMaster_fieldAdd(KegMaster_obj* self, char* field)
{
	KegItem_obj* n;
	KegMaster_FieldDefType* f;

	for (int i = 0; i < sizeof(KegMaster_FieldDef) / sizeof(KegMaster_FieldDef[0]); i++) {
		f = &KegMaster_FieldDef[i];
		if (0 == strcmp(f->name, field)) {
			break;
		}
	}
	n = KegItem_Create(f->name, f->type);
	n = KegItem_init(n, f->update, f->update_rate, f->proc );

	if (self->fields == NULL) {
		self->fields = n;
	} else { 
		KegItem_ListInsertAfter(self->fields, n);
	}

	return(n);
}

char* KegMaster_getJson(KegMaster_obj* self)
{
	static const char* JSON_GRP = "{\"ReqId\": \"none\", \"TapNo\":\"0\", \"Id\":\"%s\", %s}";
	size_t s;
	char* c;
	s = strlen(self->fields_json) + strlen(self->guid) + strlen(JSON_GRP);
	c = malloc(s);
	snprintf(c, s, JSON_GRP, self->guid, self->fields_json);
	free(self->fields_json);
	self->fields_json = NULL;
	return(c);
}