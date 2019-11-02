#pragma once
#include <stdbool.h>
#include <time.h>

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
	float refresh_period,
	KegItem_funcProc* value_proc,
    float queryRate
    ) );

struct KegItem_obj
{
	/* Data */
	char* key;
	void* value;
	KegItem_Type value_type;
	void* state; /* Item specific state data */

	/* State */
	bool value_sync;  /* Ready for sync with Sql Db */	
	bool value_dirty; /* Udpated from HW, not yet processed */

	/* Data accessors - Direct value access not supported */
	KegItem_funcSet* value_set;

	/* Transformations */
	KegItem_funcChr* value_toString;
	KegItem_funcChr* toJson;

	/* Behavior */
	struct timespec refresh_timeNext;
	time_t refreshPeriod;
    struct timespec query_timeNext;
    time_t queryPeriod;


	KegItem_funcInt* value_refresh; /* Either Hw or Db assumed */
	KegItem_funcProc* value_proc;	
	KegItem_funcInit* init;

	/* Linked List */
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
	float refresh_period,
	KegItem_funcInt* value_proc,
    float queryRate
);

/*=============================================================================
Utilities
=============================================================================*/

static __inline KegItem_obj* KegItem_ListGetLast(KegItem_obj* self)
{
	return( (self->prev == NULL || self->prev == self) ? self : self->prev);
}

static __inline KegItem_obj* KegItem_ListInsertBefore(KegItem_obj* pivot, KegItem_obj* prev)
{
    KegItem_obj* old_prev;
    old_prev = pivot->prev;

    /* old_prev <--> [------] <--> pivot <--> next */

    /* old_prev <--> new_prev x--x pivot <--> next */
    prev->prev = old_prev;
    old_prev->next = prev;

    /* old_prev <--> new_prev <--> pivot <--> next */
    prev->next = pivot;
    pivot->prev = prev;

	return(pivot);
}

