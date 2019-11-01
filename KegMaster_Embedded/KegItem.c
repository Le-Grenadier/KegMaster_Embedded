#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#include <applibs/log.h>
#include <applibs/i2c.h>
#include "azureiot/iothub_message.h"
#include <azureiot/iothub_device_client_ll.h>

#include "KegItem.h"
#include "KegMaster.h"
#include "KegMaster_Satellite.h"

#include "azure_iot_utilities.h"
#include "connection_strings.h"

extern int i2cFd;

static KegItem_obj* getSiblingByKey(KegItem_obj* self, char* key);
static bool dateTimeCompare(struct tm* base, struct tm* challenge);

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

/*-----------------------------------------------------------------------------
KegItem Object Value Setter functions
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
	
	if (self->value == NULL)
	{
		size_t sz = strlen(*(char**)value);
		self->value = malloc(sz);
		memset(self->value, 0, sz);
	}

	return(NULL != strcpy(self->value, *(char**)value));
};

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


int kegItem_setDateTimeFromString(KegItem_obj* self, void* value)
{
    // Db format example: "2016-05-20T07:00:00.0000000"
    char* start;
    char* end;
    struct tm time;
    //struct tm tm = *localtime(&t);

    if (self->value == NULL && value != NULL)
    {
        self->value = malloc(sizeof(struct tm));
        memset(self->value, 0, sizeof(struct tm));
    }
    /* Year */
    start = *(char**)value;
    end = strstr(start,"-");
    *end = 0;
    time.tm_year = atoi(start);

    /* Month */
    start = (char*)end+1;
    end = strstr(start, "-");
    *end = 0;
    time.tm_mon = atoi(start);

    /* Day */
    start = (char*)end+1;
    end = strstr(start, "T");
    *end = 0;
    time.tm_mday = atoi(start);

    /* Hour */
    start = (char*)end+1;
    end = strstr(start, ":");
    *end = 0;
    time.tm_hour = atoi(start);
    
    /* Minute */
    start = (char*)end+1;
    end = strstr(start, ":");
    *end = 0;
    time.tm_min = atoi(start);

    /* Second */
    start = (char*)end+1;
    //end = strstr(value, "."); // Omit Partial seconds
    //*end = 0;
    time.tm_sec = atoi(start);


    memcpy(self->value, &time, sizeof(struct tm));

    return(true);
};

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
	const char* FMT = "%s";
	char* c;
	char* s;
	size_t sz;

	assert(self->value);
	s = *(bool*)self->value ? "True" : "False";
	sz = strlen(s)+1; // +1 for null terminator
	c = (char*)malloc(sz);
	memset(c, 0, sz);
	snprintf(c, sz, FMT, s);
	return(c);
}

char* kegItem_formatDateTime(KegItem_obj* self)
{
    // Db format example: "2016-05-20T07:00:00.0000000"
    #define DT_STR_FMT "%d-%d-%dT%d:%d:%2.7f"
    #define DT_STR_SIZE 4+2+2+2+2+2+7+6+1 /* digits, seperators, and null terminator*/
    struct tm* time;
    char* str;

    time = (struct tm*)self->value;
    str = malloc(DT_STR_SIZE);
    memset(str, 0, DT_STR_SIZE);
    snprintf(str, DT_STR_SIZE, DT_STR_FMT, time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);

    return(str);
};

/*-----------------------------------------------------------------------------
KegItem Object Key:Value to JSON
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

/*=============================================================================
KegItem Hw and DB accessors
=============================================================================*/

int KegItem_HwGetPressureCrnt(KegItem_obj* self){
	I2C_DeviceAddress address = 0x8; // Base address chosen at random-ish
	int* err;
    KegItem_obj* tapNo;
	KegMaster_SatelliteMsgType msg;
	bool result;
	float pressure;
	
    tapNo = getSiblingByKey(self, "TapNo");
    if (tapNo == NULL || tapNo->value == NULL) {
        return(false);
    }

	if (self->value == NULL ){
		float f;
		f = 0.0f;
		self->value_set(self, &f);
	}
    address += *(I2C_DeviceAddress*)tapNo->value;
    msg.id = KegMaster_SateliteMsgId_ADCRead;
	msg.data.adc.id = 0;
	msg.data.adc.value = 0;
	result = I2CMaster_WriteThenRead(i2cFd, address, (uint8_t*)&msg, sizeof(msg), (uint8_t*)&msg, sizeof(msg));  
	pressure = (float)msg.data.adc.value; // Do some conversion here later (need to adjust for STP)
	if (result != 0) {
		err = (int*)errno;
		Log_Debug("ERROR: TFMini Soft Reset Fail: errno=%d (%s)\n", err, strerror(err));
		pressure = 999999.9f;
	}

	return((int)pressure);
}


int KegItem_HwGetQtyAvail(KegItem_obj* self) {
    #define WEIGHT_UPDT_TOL 100 /* Grams */ / 1000.0f

    KegItem_obj* tapNo;
    int          tapNo_value;
	I2C_DeviceAddress address = 0x8; // Base address chosen at random-ish
	int* err;
    KegMaster_SatelliteMsgType msg_tx;
    KegMaster_SatelliteMsgType msg_rx;
    bool result;
	float weight;

	if (self->value == NULL) {
		float f;
		f = 0.0f;
		self->value_set(self, &f);
	}

    tapNo = getSiblingByKey(self, "TapNo");
    if (tapNo == NULL
     || tapNo->value == NULL) {
        return(0);
    }

    tapNo_value = *(int*)tapNo->value;
	address += tapNo_value;
    msg_tx.id = KegMaster_SateliteMsgId_ADCRead;
    msg_tx.data.adc.id = 3;
    msg_tx.data.adc.value = 0;
    msg_tx.msg_trm = 0x04FF;
	result = I2CMaster_WriteThenRead(i2cFd, address, (uint8_t*)&msg_tx, sizeof(msg_tx), (uint8_t*)&msg_rx, sizeof(msg_rx));
    weight = (float)msg_rx.data.adc.value; // Raw data
    weight = weight - 146400;     // Offset for zero
    weight = weight / 23.0f;      // convert to grams
    weight = weight / 1000.0f;    // convert to liters
    weight = weight * 2.11338f;   // Convert to pints

    /* Reasonableness checks */
    weight = fmaxf(weight, 0.0f);
    weight = fminf(weight, 9999.0f);

	if (result != 1) {
		err = errno;
		Log_Debug("ERROR: Failed to get Keg %d weight: errno=%d (%s)\n", tapNo_value, strerror(err));
        weight = INFINITY;
	}
    else {

        /* Reduce message Tx spam (and Azure bill) - only send update if change is notable */
        if ((weight < INFINITY) && (abs((*(float*)self->value) - weight) > WEIGHT_UPDT_TOL)) {
            self->value_dirty = true;
        }

        Log_Debug("INFO: Keg %d weighs %f\n", tapNo_value, weight);
        self->value_set(self, &weight);
    }

	return(0);
}

/*-----------------------------------------------------------------------------
KegItem Db access interface
-----------------------------------------------------------------------------*/
//static int KegItem_DbGetPressureCrnt(KegItem_obj * self)
//{
//	(*(float*)self->value)++;
//	self->value_set(self, ((float*)self->value));
//	return(true);
//}

/*-----------------------------------------------------------------------------
KegItem data processing callback functions
 - This is where either data will be 'cleaned' or otherwise acted upon.
 - TODO: Reduct function complexity
-----------------------------------------------------------------------------*/
int KegItem_ProcPourEn(KegItem_obj* self) {
    #define POUR_DELAY 5 /* Seconds */ 
    #define POUR_INT_CNVT 1000 /* Interrupts per oz */

    static const bool disable = false;
    static struct timespec lockoutTimer = { 0, 0 };
    
    int ret = 0;
    KegItem_obj* avail;
    KegItem_obj* rsrv;
    KegItem_obj* szPour;
    KegItem_obj* szSmpl;
    KegItem_obj* tapNo;
    KegItem_obj* pourEn;

    bool pourTimeout;
    bool pourLevelOk;
    uint16_t pourQty; 
    struct timespec realTime;

    I2C_DeviceAddress address = 0x8; // Base address chosen at random-ish
    int* err;
    KegMaster_SatelliteMsgType qtyMsg;
    KegMaster_SatelliteMsgType pourMsg;

    bool result;

    avail = getSiblingByKey(self, "QtyAvailable");
    rsrv = getSiblingByKey(self, "QtyReserve");
    szPour = getSiblingByKey(self, "PourQtyGlass");
    szSmpl = getSiblingByKey(self, "PourQtySample");
    tapNo = getSiblingByKey(self, "TapNo");
    pourEn = getSiblingByKey(self, "PourEn"); 
    clock_gettime(CLOCK_REALTIME, &realTime);

    if (tapNo == NULL || avail == NULL || rsrv == NULL || szPour == NULL || szSmpl == NULL 
     || tapNo->value == NULL ) {
        return(0);
    }
    /* Update Satellite MCU address */
    address += *(I2C_DeviceAddress*)tapNo->value;

    /*=========================================================================
    Qty Poured Already
    =========================================================================*/
    
    /* Get Current qty poured */
    qtyMsg.id = KegMaster_SateliteMsgId_InterruptRead;
    qtyMsg.data.intrpt.id = 0;
    qtyMsg.msg_trm = 0x04FF;
    result = -1;
    result = I2CMaster_WriteThenRead(i2cFd, address, (uint8_t*)&qtyMsg, sizeof(qtyMsg), (uint8_t*)&qtyMsg, sizeof(qtyMsg));
    pourQty = qtyMsg.data.intrpt.count / POUR_INT_CNVT;

    /*-------------------------------------------------------------------------
    Disable pour temporarily if 
       - not presently locked-out
       - pouring enabled, glass has been dispensed
       - pouring disabled, sampling enabled, and sample has been served 
    -------------------------------------------------------------------------*/
    if( (realTime.tv_sec >= lockoutTimer.tv_sec )
     && ( (*(bool*)pourEn->value) && (*(float*)szPour->value) > 0.1 && pourQty >= (*(float*)szPour->value) )
       || (!(*(bool*)pourEn->value) && (*(float*)szSmpl->value) > 0.1 && pourQty >= (*(float*)szSmpl->value) ) ){
        lockoutTimer.tv_sec = realTime.tv_sec + POUR_DELAY;
    }

    /*=========================================================================
    Stop if below reserve value
    =========================================================================*/
    if ( (*(bool*)self->value) && (*(float*)avail->value <= *(float*)rsrv->value)) {
        self->value_set(self, &disable);
        //self->value_dirty = true;
    }

    /*=========================================================================
    Set ouput state
    =========================================================================*/
    /* Enable Pouring based on state, if not locked-out */
    pourMsg.id = KegMaster_SateliteMsgId_GpioSetDflt;
    pourMsg.data.gpio.id = 3;
    pourMsg.data.gpio.state = (*(bool*)pourEn->value) && (realTime.tv_sec >= lockoutTimer.tv_sec);
    pourMsg.data.gpio.holdTime = POUR_DELAY;
    pourMsg.msg_trm = 0x04FF;
    result = -1;
    result = I2CMaster_Write(i2cFd, address, (uint8_t*)&pourMsg, sizeof(pourMsg));

    return(ret=1);
}


int KegItem_ProcPressureCrnt(KegItem_obj* self)
{
    #define DWELL_MIN 0.02f

    KegItem_obj* pressDsrd;
    KegItem_obj* tapNo;
    I2C_DeviceAddress address = 0x8; // Base address chosen at random-ish
    float pressure_crnt;
    float pressure_dsrd;
    float dwell_time;
    int* err;
    KegItem_obj* keg_item;
    KegMaster_SatelliteMsgType msg;
    bool result;

    if (self->value == NULL) {
        return(false);
    }

    tapNo = getSiblingByKey(self, "TapNo");
    pressDsrd = getSiblingByKey(self, "PressureDsrd");
    if (tapNo == NULL || pressDsrd == NULL
     || tapNo->value == NULL) {
            return(0);
    }
    pressure_crnt = *(float*)self->value;
    pressure_dsrd = *(float*)pressDsrd->value;
    
    // [insert fancy control algorithm here] 
    /* For now, just proportional control
        - Minimum dwell time = 20 ms (mostly chosen at random, based on other, similar valves.)
        - Max dwell time 1 second (Again chosen mostly at random, based on how scary I'd find it going off while disconnected.) */
    dwell_time = (pressure_dsrd - pressure_crnt) / pressure_dsrd * 1000;
    dwell_time = fmaxf(dwell_time, DWELL_MIN);

    /* Send message */
    address += *(I2C_DeviceAddress*)tapNo->value;
    msg.id = KegMaster_SateliteMsgId_GpioSet;
    msg.data.gpio.id = 2;
    msg.data.gpio.state = 1;
    msg.data.gpio.holdTime = (uint16_t) dwell_time;
    msg.msg_trm = 0x04FF;
    result = -1;
    result = I2CMaster_Write(i2cFd, address, (uint8_t*)&msg, sizeof(msg));

    if (result != 0) {
        err = errno;
        Log_Debug("ERROR: TFMini Soft Reset Fail: errno=%d (%s)\n", err, strerror(err));
    }

    /* Update statistics */
    keg_item = getSiblingByKey(self, "PressureDwellTime");
    if (keg_item->value != NULL) {
        dwell_time += *(float*)keg_item->value;
    }
    keg_item->value_set(keg_item, &dwell_time);

    return(result == 0);
}


int KegItem_ProcDateAvail(KegItem_obj* self) {
    bool b;
    KegItem_obj* pourEn;
    time_t t = time(NULL);
    struct tm time = *localtime(&t);
    if(self->value == NULL) {
        return(0);
    }

    //KegMaster_SatelliteMsgType msg;
    pourEn = getSiblingByKey(self, "PourEn");

    if( (b = dateTimeCompare(self->value, &time)) && pourEn != NULL && !*(bool*)pourEn->value){
        pourEn->value_set(pourEn, &b);
    }

    return(0);
}


/*=============================================================================
Local functions
=============================================================================*/

static KegItem_obj* getSiblingByKey(KegItem_obj* self, char* key)
{
	KegItem_obj* this = NULL;
	KegItem_obj* item = NULL;
	assert(key != NULL);

	this = self;
	do {
		if (0 == memcmp(key, this->key, strlen(key))) {
			item = this;
			break;
		}
		this = this->next;
	} while (this != self);

	return(item);
}

static bool dateTimeCompare(struct tm* base, struct tm* challenge) {
    bool gt;

    if (base == NULL || challenge == NULL )
    {
        return(0);
    }
    /* Only compare date and time */
    gt = 0;
    gt |= challenge->tm_year > base->tm_year ? true : false;
    gt |= challenge->tm_mon  > base->tm_mon  ? true : false;
    gt |= challenge->tm_mday > base->tm_mday ? true : false;
    gt |= challenge->tm_hour > base->tm_hour ? true : false;
    gt |= challenge->tm_min  > base->tm_min  ? true : false;
    gt |= challenge->tm_sec  > base->tm_sec  ? true : false;

    return(gt);
}