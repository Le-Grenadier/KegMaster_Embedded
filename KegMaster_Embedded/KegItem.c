#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "azureiot/iothub_message.h"
#include <azureiot/iothub_device_client_ll.h>

#include "KegItem.h"
#include "KegMaster.h"


#include "azure_iot_utilities.h"
#include "connection_strings.h"

/*=============================================================================
KegItem Object Constructors
=============================================================================*/

/*-----------------------------------------------------------------------------
 * Create new Keg Item and prepare for initialization
 -----------------------------------------------------------------------------*/
KegItem_obj* KegItem_Create(
	const char* guid,
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
	/* Create new elememt */
	KegItem_obj* e;
	e = malloc(sizeof(KegItem_obj));
	memset(e, 0, sizeof(KegItem_obj));

	/* Update key and Keg GUID */
	e->keg_guid = malloc(strlen(guid));
	e->key = malloc(strlen(key));
	strcpy(e->keg_guid, guid);
	strcpy(e->key, key);

	e->init = &KegItem_init;


	assert(type != KegItem_NONE && type <= KegItem_TYPECNT );
	e->value_type = type;
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
	KegItem_funcInt* value_refresh,
	unsigned int refresh_period,
	KegItem_funcInt* value_proc
	)
{
	assert(((value_refresh != NULL)) && (value_proc != NULL));

	/* Copy functions into object */
	self->value_refresh = value_refresh;
	self->refresh_period = refresh_period;
	self->value_proc = value_proc;

	/* Perform initial refresh of data */
	self->value_dirty = self->value_refresh(self);
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
		memset(self->value, 0, sizeof(float));
	}
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
	const char* JSON_FMT = "{\"Id\":\"%s\", \"%s\":\"%s\"}";
	char* c;
	size_t key_len = strlen(self->key);
	char* val_s = self->value_toString(self);
	size_t val_len = strlen(val_s);
	size_t guid_len = strlen(self->keg_guid);
	unsigned int mal_sz = strlen(JSON_FMT) /*- 4%s and all that*/ + key_len + val_len + guid_len;

	c = malloc(mal_sz);
	memset(c, 0, sizeof(mal_sz));
	snprintf(c, mal_sz, JSON_FMT, self->keg_guid, self->key, val_s);
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
int KegItem_ProcPressureCrnt(KegItem_obj* self, void* parent )
{	
	char* p = self->toJson(self);
	AzureIoT_SendMessage(p);
	free(p);
	return(true);
}
