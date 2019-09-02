/*
 * Hardware Access interface description for Keg Master
 */

#pragma once

#include "epoll_timerfd_utilities.h"

#include "KegItem.h"


int KegMaster_initRemote(void);
int KegMaster_initLocal(void);
int KegMaster_initProcs(void);

int KegMaster_dbGetKegData(void);

/* Forward declare KegMaster_obj */
typedef struct KegMaster_obj KegMaster_obj;

typedef KegItem_obj* (KegMaster_funcItem( KegMaster_obj* self, char* key));
typedef KegMaster_obj* (KegMaster_create(char* guid));


struct KegMaster_obj
{
	/* Self-Data */
	char* guid; 

	/* Fields */
	struct
	{
		KegItem_obj* this; 
		KegItem_obj* next;
		KegItem_obj* prev;
	} items;

	/* Behavior */
	KegMaster_create* create;


	KegMaster_funcItem* fieldGetByKey;

};
