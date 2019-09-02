#pragma once
#include <stdbool.h>

extern const char* KegItem_Fields[];

/*=============================================================================
Types 
=============================================================================*/
typedef enum {
	KegItem_TypeNONE = 0,
	KegItem_TypeFLOAT,
	KegItem_TypeINT,
	KegItem_TypeDATE,
	KegItem_TypeSTR,
	KegItem_TypeBOOL,
	KegItem_TypeCNT
} KegItem_Type;

/* Forward declare structure type to allow self-reference */
typedef struct KegItem_obj KegItem_obj;

typedef int (KegItem_funcInt(KegItem_obj* self));
typedef char* (KegItem_funcChr(KegItem_obj* self));
typedef int (KegItem_funcSet(KegItem_obj* self, void* value));
typedef int (KegItem_funcProc(KegItem_obj* self));

typedef KegItem_obj* (KegItem_funcInit(KegItem_obj* self,
	KegItem_funcInt* value_refresh,
	unsigned int refresh_period,
	KegItem_funcProc* value_proc ) );

struct KegItem_obj
{
	/* Data */
	char* key;
	void* value;
	KegItem_Type value_type;
	
	/* State */
	bool value_sync;  /* Ready for sync with Sql Db */	
	bool value_dirty; /* Udpated from HW, not yet processed */

	/* Data accessors - Direct value access not supported */
	KegItem_funcSet* value_set;

	/* Transformations */
	KegItem_funcChr* value_toString;
	KegItem_funcChr* toJson;

	/* Behavior */
	int refresh_timeNext;
	int refresh_timePrev; // Could be used for connectiviy issues
	unsigned int refresh_period;

	KegItem_funcInt* value_refresh; /* Either Hw or Db assumed */
	KegItem_funcProc* value_proc;	
	KegItem_funcInit* init;

	/* Lined List */
	KegItem_obj* next;
	KegItem_obj* prev;
};


/*=============================================================================
KegItem Object Constructors
=============================================================================*/
KegItem_obj* KegItem_Create(
	const char* key,
	KegItem_Type type
);


KegItem_obj* KegItem_init(
	KegItem_obj* self,
	KegItem_funcInt* value_refresh,
	unsigned int refresh_period,
	KegItem_funcInt* value_proc
);


/*=============================================================================
KegItem Object Value Setter functions
=============================================================================*/
//__inline KegItem_funcSet kegItem_setInt;
KegItem_funcSet kegItem_setFloat;
//__inline KegItem_funcSet kegItem_setDateTime;
//__inline KegItem_funcSet kegItem_setBool;


/*=============================================================================
KegItem Object Value Formatting functions
 - char* must be free'd after use
=============================================================================*/
//__inline KegItem_funcChr kegItem_formatInt;
KegItem_funcChr kegItem_formatFloat;
//__inline KegItem_funcChr kegItem_formatDateTime;
//__inline KegItem_funcChr kegItem_formatBool;

/*=============================================================================
KegItem Object Key:Value to JSON
 - char* must be free'd after use
=============================================================================*/
char* KegItem_toJson(KegItem_obj* self);

/*=============================================================================
KegItem Hw access interface
=============================================================================*/
int KegItem_HwGetPressureCrnt(KegItem_obj* self);

/*=============================================================================
KegItem Db access interface
=============================================================================*/
//static int KegItem_DbGetPressureCrnt(KegItem_obj* self);

/*=============================================================================
KegItem data processing callback 'cleaner' functions
=============================================================================*/
int KegItem_ProcPressureCrnt(KegItem_obj* self);

/*=============================================================================
Utilities
=============================================================================*/

static __inline KegItem_obj* KegItem_ListGetLast(KegItem_obj* self)
{
	return( (self->prev == NULL || self->prev == self) ? self : self->prev);
}

static __inline KegItem_obj* KegItem_ListInsertAfter(KegItem_obj* pivot, KegItem_obj* next)
{
	next->next = pivot->next;
	next->prev = pivot;

	pivot->next->prev = next;
	pivot->next = next;

	return(next);
}

