/*
 * Hardware Access interface description for Keg Master
 */

#pragma once
#include <semaphore.h>

#include "epoll_timerfd_utilities.h"

#include "KegItem.h"
#include "parson.h"

 /*=============================================================================
 Literal constants
 =============================================================================*/
#define cntOfArray(e) (sizeof(e)/sizeof(e[0]))

 /*=============================================================================
 Types
 =============================================================================*/
typedef enum {
	KegMaster_FieldId,
	KegMaster_FieldIdAlerts,
	KegMaster_FieldIdTapNo,
	KegMaster_FieldIdName,
	KegMaster_FieldIdStyle,
	KegMaster_FieldIdDescription,
	KegMaster_FieldIdDateKegged,
	KegMaster_FieldIdDateAvail,
	KegMaster_FieldIdPourEn,
	KegMaster_FieldIdPourNotify,
	KegMaster_FieldIdPourQtyGlass,
	KegMaster_FieldIdPourQtySample,
	KegMaster_FieldIdPressureCrnt,
	KegMaster_FieldIdPressureDsrd,
	KegMaster_FieldIdPressureDwellTime,
	KegMaster_FieldIdPressureEn,
	KegMaster_FieldIdQtyAvailable,
	KegMaster_FieldIdQtyReserve,
	KegMaster_FieldIdVersion,
	KegMaster_FieldIdCreatedAt,
	KegMaster_FieldIdUpdatedAt,
	KegMaster_FieldIdDeleted,
	KegMaster_FieldIdCount
} KegMaster_FieldIdType;

/* Forward declare structures */
typedef struct KegMaster_obj KegMaster_obj;
typedef struct KegMaster_FieldDefType KegMaster_FieldDefType;


typedef KegItem_obj* (KegMaster_funcKegItem(KegMaster_obj* self, char* c));
typedef KegMaster_obj* (KegMaster_funcKegMaster(char* guid));
typedef char* (KegMaster_funcChar(KegMaster_obj* self));
typedef int (KegMaster_funcInt(KegMaster_obj* self));
typedef void (KegMaster_funcQryDb(int tapNo, char* select));

struct KegMaster_obj
{
	/* Data */
	char* fields_json;

	/* Fields */
	KegItem_obj* fields; 

	/* Behavior */
	KegMaster_funcKegMaster* create;
	KegMaster_funcInt* run;

	KegMaster_funcChar* field_getJson;
	KegMaster_funcKegItem* field_add;
	KegMaster_funcKegItem* field_GetByKey;
    KegMaster_funcQryDb* queryDb;
    
    sem_t kegItem_semaphore;
};

struct KegMaster_FieldDefType
{
	char*            name;
	KegItem_Type     type;
	KegItem_funcInt* update;
	KegItem_funcInt* proc;
	float            update_rate; /* Seconds */
    float            queryRate;
};

int KegMaster_initRemote(void);
int KegMaster_initLocal(void);
int KegMaster_initProcs(void);

int KegMaster_dbGetKegData(void);
void KegMaster_procAzureIotMsg(const char* sqlRow);
/* Next two functions take a pointer to an array (pointer) of pointers */
KegMaster_obj* KegMaster_getKegPtr(KegMaster_obj*** keg_root, int idx);
KegMaster_obj* KegMaster_updateKeg(JSON_Value* jsonRoot, KegMaster_obj*** keg_root, int keg_cnt);
int KegMaster_execute(KegMaster_obj* self);
KegItem_obj* KegMaster_fieldAdd(KegMaster_obj* self, char* field);
KegItem_obj* KegMaster_getFieldByKey(KegMaster_obj* self, char* key);
char* KegMaster_getJson(KegMaster_obj* self);
void KegMaster_RequestKegData(int tapNo, char* select);
