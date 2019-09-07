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
	const char* key,
	KegItem_Type type
	)
{
	static const struct
	{
		KegItem_funcSet* setVal;
		KegItem_funcChr* getStr;
	} valSet_funcs[KegItem_TypeCNT] =
	{
	{ NULL,	NULL},
	{ kegItem_setFloat, kegItem_formatFloat },
	{ kegItem_setInt, kegItem_formatInt },
	{ kegItem_setStr, kegItem_formatStr },
	{ kegItem_setStr, kegItem_formatStr },
	{ kegItem_setBool, kegItem_formatBool }
	};

	/* Create new elememt */
	KegItem_obj* e;
	e = malloc(sizeof(KegItem_obj));
	memset(e, 0, sizeof(KegItem_obj)); /* This memset is important for null pointer checks */

	/* Update key */
	e->key = malloc(strlen(key));
	strcpy(e->key, key);

	e->init = &KegItem_init;

	assert(type != KegItem_TypeNONE && type <= KegItem_TypeCNT );
	e->value_type = type;
	e->value_set = valSet_funcs[type].setVal;
	e->value_toString = valSet_funcs[type].getStr;
	e->toJson = KegItem_toJson;

	e->next = e;
	e->prev = e;

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
	//assert(((value_refresh != NULL)) || (value_proc != NULL));

	/* Copy functions into object */
	self->value_refresh = value_refresh;
	self->refresh_period = refresh_period;
	self->value_proc = value_proc;

	/* Perform initial refresh of data */
	if (self->value_refresh != NULL){
		self->value_dirty = self->value_refresh(self);
	}
	if (self->value_proc != NULL) {
		self->value_proc(self);
	}

	return(self);
}

/*=============================================================================
KegItem Object Value Setter functions
=============================================================================*/
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

int kegItem_setInt(KegItem_obj* self, void* value)
{
	if (self->value == NULL)
	{
		self->value = malloc(sizeof(int));
		memset(self->value, 0, sizeof(int));
	}
	*((int*)self->value) = *(int*)value;

	return(true);
};

int kegItem_setStr(KegItem_obj* self, void* value)
{
	assert(value);
	if (self->value != NULL && strlen(*(char**)value) > strlen(*(char**)value))
	{
		free(self->value);
		self->value = NULL;
	}
	
	if (self->value == NULL)
	{
		size_t sz = strlen(*(char**)value);
		self->value = malloc(sz);
		memset(self->value, 0, sz);
	}

	NULL != strcpy(self->value, *(char**)value);
	return(true);
};

int kegItem_setBool(KegItem_obj* self, void* value)
{
	if (self->value == NULL)
	{
		self->value = malloc(sizeof(bool));
		memset(self->value, 0, sizeof(bool));
	}
	*((bool*)self->value) = *(bool*)value;

	return(true);
};

/*=============================================================================
KegItem Object Value Formatting functions
 - char* must be free'd after use
=============================================================================*/
char* kegItem_formatFloat(KegItem_obj* self) {
	const char* FMT = "%3.3f";
	const size_t PRECISION = 3/*digits*/ + 3/*decimal*/ + 1/*space for decimal*/;
	char* c;

	c = (char*)malloc(PRECISION);
	memset(c, 0, sizeof(PRECISION));
	snprintf(c, PRECISION, FMT, *(float*)self->value);
	return(c);
}

char* kegItem_formatInt(KegItem_obj* self) {
	const char* FMT = "%d";
	const size_t PRECISION = (((1<32)/2)-1)/10;
	char* c;

	c = (char*)malloc(PRECISION);
	memset(c, 0, sizeof(PRECISION));
	snprintf(c, PRECISION, FMT, *(int*)self->value);
	return(c);
}

char* kegItem_formatStr(KegItem_obj* self) {
	const char* FMT = "%s";

	char* c;
	size_t sz = strlen(self->value)+1/* Incl Null Terminator */;

	assert(self->value);
	c = (char*)malloc(sz);
	memset(c, 0, sz);
	snprintf(c, sz, FMT, (char*)self->value);
	return(c);
}

char* kegItem_formatBool(KegItem_obj* self) {
	const char* FMT = "";
	char* c;
	char* s;
	size_t sz;

	assert(self->value);
	s = self->value ? "True" : "False";
	sz = strlen(s);
	c = (char*)malloc(sz);
	memset(c, 0, sz);
	snprintf(c, sz, FMT, s);
	return(c);
}

/*=============================================================================
KegItem Object Key:Value to JSON
 - char* must be free'd after use
=============================================================================*/
char* KegItem_toJson(KegItem_obj* self) {
	const char* JSON_ENTRY = "\"%s\":\"%s\"";
	char* c;
	size_t key_len = strlen(self->key);
	char* val_s = self->value_toString(self);
	size_t val_len = strlen(val_s);
	unsigned int mal_sz = strlen(JSON_ENTRY) + key_len + val_len;

	c = malloc(mal_sz);
	memset(c, 0, sizeof(mal_sz));
	snprintf(c, mal_sz, JSON_ENTRY, self->key, val_s);
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
int KegItem_ProcPressureCrnt(KegItem_obj* self)
{	
	//char* p = self->toJson(self);
	//AzureIoT_SendMessage(p);
	//free(p);
	return(true);
}
