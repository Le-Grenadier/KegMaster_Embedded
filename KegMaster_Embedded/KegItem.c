#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#include "KegItem.h"
#include "KegItem_Utl.h"

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
	{ NULL,	                            NULL},
	{ kegItem_setFloat,                 kegItem_formatFloat },      /* Float */
	{ kegItem_setInt,                   kegItem_formatInt },        /* Int */
	{ kegItem_setDateTimeFromString,    kegItem_formatDateTime },   /* Date */
	{ kegItem_setStr,                   kegItem_formatStr },        /* Str */
	{ kegItem_setBool,                  kegItem_formatBool }        /* Bool */
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
	float refresh_period,
	KegItem_funcInt* value_proc,
    float queryRate
)
{
	//assert(((value_refresh != NULL)) || (value_proc != NULL));

	/* Copy functions into object */
	self->value_refresh = value_refresh;
	self->refreshPeriod = (time_t)refresh_period;
	self->value_proc = value_proc;
    self->queryPeriod = (time_t)queryRate;

    clock_gettime(CLOCK_REALTIME, &self->refresh_timeNext);
    self->refresh_timeNext.tv_sec += self->refreshPeriod;

    clock_gettime(CLOCK_REALTIME, &self->query_timeNext);
    self->query_timeNext.tv_sec += self->queryPeriod;

	return(self);
}
