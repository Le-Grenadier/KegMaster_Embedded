#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "DateTime.h"

#include "KegItem.h"
#include "KegItem_Utl.h"

/*=====================================================================================
Keg Item Parsing Functions - From string
=====================================================================================*/

/*-----------------------------------------------------------------------------
Intake Float
-----------------------------------------------------------------------------*/
int kegItem_setFloat(KegItem_obj* self, void* value)
{
    if (value == NULL) {
        return(false);
    }

    if (self->value == NULL)
    {
        self->value = malloc(sizeof(float));
        memset(self->value, 0, sizeof(float));
    }
    *((float*)self->value) = *(float*)value;

    return(true);
};

/*-----------------------------------------------------------------------------
Intake Int
-----------------------------------------------------------------------------*/
int kegItem_setInt(KegItem_obj* self, void* value)
{
    if (value == NULL) {
        return(false);
    }

    if (self->value == NULL)
    {
        self->value = malloc(sizeof(int));
        memset(self->value, 0, sizeof(int));
    }
    *((int*)self->value) = *(int*)value;

    return(true);
};

/*-----------------------------------------------------------------------------
Intake String
-----------------------------------------------------------------------------*/
int kegItem_setStr(KegItem_obj* self, void* value)
{
    assert(value);
    if (value == NULL || *(char**)value == NULL) {
        return(false);
    }

    if (self->value != NULL && strlen(*(char**)value) > strlen(*(char**)value))
    {
        free(self->value);
        self->value = NULL;
    }

    // TODO: use realloc
    if (self->value == NULL)
    {
        size_t sz = strlen(*(char**)value);
        self->value = malloc(sz);
        memset(self->value, 0, sz);
    }

    return(NULL != strcpy(self->value, *(char**)value));
};

/*-----------------------------------------------------------------------------
Intake Boolean
-----------------------------------------------------------------------------*/
int kegItem_setBool(KegItem_obj* self, void* value)
{
    if (self->value == NULL && value != NULL)
    {
        self->value = malloc(sizeof(bool));
        memset(self->value, 0, sizeof(bool));
    }
    *((bool*)self->value) = *(bool*)value;

    return(true);
};

/*-----------------------------------------------------------------------------
Intake SQL DateTime Type
-----------------------------------------------------------------------------*/
int kegItem_setDateTimeFromString(KegItem_obj* self, void* value)
{
    /* Db format example: "2016-05-20T07:00:00.0000000" */
    char* start;
    char* end;
    struct tm time;

    if (self->value == NULL && value != NULL)
    {
        self->value = malloc(sizeof(struct tm));
        memset(self->value, 0, sizeof(struct tm));
    }
    /* Year - (Years since 1900) */
    start = *(char**)value;
    end = strstr(start, "-");
    *end = 0;
    time.tm_year = atoi(start);
    time.tm_year -= 1900;

    /* Month - (base zero) */
    start = (char*)end + 1;
    end = strstr(start, "-");
    *end = 0;
    time.tm_mon = atoi(start);
    time.tm_mon = time.tm_mon - 1;

    /* Day */
    start = (char*)end + 1;
    end = strstr(start, "T");
    *end = 0;
    time.tm_mday = atoi(start);

    /* Hour */
    start = (char*)end + 1;
    end = strstr(start, ":");
    *end = 0;
    time.tm_hour = atoi(start);

    /* Minute */
    start = (char*)end + 1;
    end = strstr(start, ":");
    *end = 0;
    time.tm_min = atoi(start);

    /* Second */
    start = (char*)end + 1;
    time.tm_sec = atoi(start);


    memcpy(self->value, &time, sizeof(struct tm));

    return(true);
};


/*=====================================================================================
Keg Item Formatting Functions - to string
=====================================================================================*/

/*-----------------------------------------------------------------------------
KegItem Object Value Formatting functions
 - char* must be free'd after use
-----------------------------------------------------------------------------*/
char* kegItem_formatFloat(KegItem_obj* self) {
    const char* FMT = "%3.3f";
    const size_t PRECISION = 3/*digits*/ + 3/*decimal*/ + 1/*space for decimal*/;
    char* c;

    c = (char*)malloc(PRECISION);
    memset(c, 0, sizeof(PRECISION));
    snprintf(c, PRECISION, FMT, *(float*)self->value);
    return(c);
}

/*-----------------------------------------------------------------------------
Format Integer KegItem to Integer string for sending to SQL Db
 - char* must be free'd after use
-----------------------------------------------------------------------------*/
char* kegItem_formatInt(KegItem_obj* self) {
    const char* FMT = "%d";
    const size_t PRECISION = (((1 < 32) / 2) - 1) / 10;
    char* c;

    c = (char*)malloc(PRECISION);
    memset(c, 0, sizeof(PRECISION));
    snprintf(c, PRECISION, FMT, *(int*)self->value);
    return(c);
}

/*-----------------------------------------------------------------------------
Format String KegItem to string for sending to SQL Db
 - char* must be free'd after use
-----------------------------------------------------------------------------*/
char* kegItem_formatStr(KegItem_obj* self) {
    const char* FMT = "%s";

    char* c;
    size_t sz = strlen(self->value) + 1/* Incl Null Terminator */;

    assert(self->value);
    c = (char*)malloc(sz);
    memset(c, 0, sz);
    snprintf(c, sz, FMT, (char*)self->value);
    return(c);
}
/*-----------------------------------------------------------------------------
Format Boolean KegItem to boolean string for sending to SQL Db
 - char* must be free'd after use
-----------------------------------------------------------------------------*/
char* kegItem_formatBool(KegItem_obj* self) {
    const char* FMT = "%s";
    char* c;
    char* s;
    size_t sz;

    assert(self->value);
    s = *(bool*)self->value ? "True" : "False";
    sz = strlen(s) + 1; // +1 for null terminator
    c = (char*)malloc(sz);
    memset(c, 0, sz);
    snprintf(c, sz, FMT, s);
    return(c);
}

/*-----------------------------------------------------------------------------
Format DateTime KegItem to DateTime string for sending to SQL Db
 - char* must be free'd after use
-----------------------------------------------------------------------------*/
char* kegItem_formatDateTime(KegItem_obj* self)
{
    /* Db format example: "2016-05-20T07:00:00.0000000" */
    #define DT_STR_FMT "%d-%d-%dT%d:%d:%2.7f"
    #define DT_STR_SIZE 4+2+2+2+2+2+7+6+1 /* digits, seperators, and null terminator*/
    int year_absolute;
    int month_1base;
    struct tm* time;
    char* str;

    time = (struct tm*)self->value;
    year_absolute = time->tm_year + 1900;   /* tm struct stores year relative to 1900 -- Sql DateTime struct is absolute */
    month_1base = time->tm_mon + 1;         /* tm struct stores zero based month */
    str = malloc(DT_STR_SIZE);
    memset(str, 0, DT_STR_SIZE);
    snprintf(str, DT_STR_SIZE, DT_STR_FMT, year_absolute, month_1base, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);

    return(str);
};


/*=====================================================================================
Generic Utilities
=====================================================================================*/

/*-----------------------------------------------------------------------------
Return Key:Value pair (string:string) for given object
 - char* must be free'd after use
-----------------------------------------------------------------------------*/
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

/*-----------------------------------------------------------------------------
Find sibling with given name
 - Must be 'direct' sibling (linked into same list as self)
-----------------------------------------------------------------------------*/
KegItem_obj* KegItem_getSiblingByKey(KegItem_obj* self, char* key)
{
    KegItem_obj* this = NULL;
    KegItem_obj* item = NULL;
    assert(key != NULL);


    if (self != NULL) {

        this = self;
        do {
            if (0 == memcmp(key, this->key, strlen(key))) {
                item = this;
                break;
            }
            this = this->next;
        } while (this != self);
    }

    return(item);
}

/*-----------------------------------------------------------------------------
Get Tap Number for provided object
 - 'TapNo' must have been already linked as sibling. This is importnant since 
    latent messages are queued on the server and can be recieved before KegData
    as a whole. (Processing will otherwise occur for singular data.)
-----------------------------------------------------------------------------*/
int KegItem_getTapNo(KegItem_obj* self)
{
    KegItem_obj* tapNo_obj = NULL;
    int tapNo = -1;

    if (self != NULL) {
        tapNo_obj = KegItem_getSiblingByKey(self, "TapNo");
        tapNo = tapNo_obj != NULL ? *(int*)tapNo_obj->value : -1;
    }

    return(tapNo);

}

/*-----------------------------------------------------------------------------
Process DateAvailable field
 - Enable tap if appropriate
 - TODO: Somehow modify only enable beer 'once-ish'
-----------------------------------------------------------------------------*/
int KegItem_SetAlert(KegItem_obj* self, char* alert) {
    bool b;
    KegItem_obj* alerts_obj;
    time_t t = time(NULL);
    struct tm time = *localtime(&t);

    alerts_obj = KegItem_getSiblingByKey(self, "Alerts");

    if (alerts_obj == NULL) {
        return(0);
    }

    alerts_obj->value_set(alerts_obj, &alert);
    alerts_obj->value_dirty;

    return(0);
}