#include <errno.h> 
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <applibs/log.h>
#include <applibs/i2c.h>

#include "DateTime.h"
#include "KegItem.h"
#include "KegItem_Proc.h"
#include "KegItem_Utl.h"
#include "SatelliteIntf.h"


/*-----------------------------------------------------------------------------
Process PourEn data field
 - Enable Pouring based on KegItem value
 - Enable Pouring if QtySample is set (ignoring KegItem value)
 - Temporarily disable pouring after dispensing QtyPour or QtySample as appropriate
 - Disable poring if QtyAvail drops into QtyReserve (Will Update Database. TODO: Send alert)
 
 - TODO: Reduce function complexity
 - TODO: Make this more robust to behave well in the absence of related data
-----------------------------------------------------------------------------*/
int KegItem_ProcPourEn(KegItem_obj* self) {
#define POUR_DELAY 5 /* Seconds */ 
#define POUR_INT_CNVT 1000 /* Interrupts per oz */

    static bool disable = false;
    static struct timespec lockoutTimer = { 0, 0 };

    int ret = 0;
    bool enablePour;
    bool enablePourLimit;
    bool enableSample;
    bool tempLockout;
    KegItem_obj* avail;
    KegItem_obj* rsrv;
    KegItem_obj* szPour;
    KegItem_obj* szSmpl;
    KegItem_obj* pourEn;

    uint16_t pourQty;
    uint32_t holdTime;
    struct timespec realTime;

    I2C_DeviceAddress address = 0x8; // Base address chosen at random-ish
    int err;
    Satellite_MsgType qtyMsg;
    Satellite_MsgType pourMsg;

    bool result;

    /* Get related data */
    avail = KegItem_getSiblingByKey(self, "QtyAvailable");
    rsrv = KegItem_getSiblingByKey(self, "QtyReserve");
    szPour = KegItem_getSiblingByKey(self, "PourQtyGlass");
    szSmpl = KegItem_getSiblingByKey(self, "PourQtySample");
    pourEn = self;
    clock_gettime(CLOCK_REALTIME, &realTime);

    if(avail == NULL || rsrv == NULL || szPour == NULL || szSmpl == NULL) {
        return(0);
    }
    /* Update Satellite MCU address */
    address += (I2C_DeviceAddress)KegItem_getTapNo(self);
    enablePour = *(bool*)pourEn->value;
    enablePourLimit = (*(float*)szPour->value > 0.1);
    enableSample = *(float*)szSmpl->value > 0.1;

    /*-----------------------------------------------------
    Qty Poured Already
    -----------------------------------------------------*/
    Satellite_InterruptRead(address, Satellite_IntrpId_QtyPoured, &pourQty);
    pourQty = pourQty / POUR_INT_CNVT;

    /*-----------------------------------------------------
    Disable pour temporarily if
       - not presently locked-out
       - Pour enabled, glass has been dispensed
       - Pour disabled, sampling enabled, sample dispensed
    ----------------------------------------------------*/
    if ((realTime.tv_sec >= lockoutTimer.tv_sec)
        && ( ( enablePour && enablePourLimit && (pourQty > *(float*)szPour->value))
          || (!enablePour && enableSample    && (pourQty > *(float*)szSmpl->value)))) {
        lockoutTimer.tv_sec = realTime.tv_sec + POUR_DELAY;
        Satellite_InterruptReset(address, Satellite_IntrpId_QtyPoured);
    }

    /*-----------------------------------------------------
    Stop if below reserve value
    -----------------------------------------------------*/
    if ((*(bool*)self->value)
        && (*(float*)avail->value <= *(float*)rsrv->value)
        && (*(float*)avail->value > *(float*)rsrv->value - pourQty ) ) {
        self->value_set(self, &disable);
        self->value_dirty = true;
    }

    /*-----------------------------------------------------
    Set ouput state
    -----------------------------------------------------*/
    /* Enable Pouring based on state, if not locked-out                                 */
    /*  - Enable for 2x refresh period so an error won't leave it unlocked indefinitely */
    holdTime = (uint32_t)self->refreshPeriod * 2 * 1000;
    holdTime = holdTime > UINT16_MAX ? UINT16_MAX : (uint16_t)holdTime;

    tempLockout = realTime.tv_sec <= lockoutTimer.tv_sec;

    enablePour = (enablePour || enableSample) && !tempLockout;
    Satellite_GpioSet(address, Satellite_OutputId_TapEn, &enablePour, (uint16_t)holdTime);

    return(ret = 1);
}

/*-----------------------------------------------------------------------------
Process PressureDsrd field
 - Control Valve to bring keg up to pressure.
-----------------------------------------------------------------------------*/
int KegItem_ProcPressureDsrd(KegItem_obj* self)
{
    #define DWELL_MIN 0.02f

    KegItem_obj* pressCrnt_obj;
    I2C_DeviceAddress address = 0x8; // Base address chosen at random-ish
    float pressureCrnt;
    float pressureDsrd;
    float dwellTime;
    int err;
    KegItem_obj* kegItem;
    Satellite_MsgType msg;
    bool gasEn;
    uint16_t holdTime;
    bool result;

    if (self->value == NULL) {
        return(false);
    }

    pressCrnt_obj = KegItem_getSiblingByKey(self, "PressureCrnt");
    if(pressCrnt_obj == NULL) {
        return(0);
    }
    pressureCrnt = *(float*)pressCrnt_obj->value;
    pressureDsrd = *(float*)self->value;

    // [insert fancy control algorithm here] 
    /* For now, just proportional control
        - Minimum dwell time = 20 ms (mostly chosen at random, based on other, similar valves.)
        - Calculate dwell time as percentage of interval */
    dwellTime = (pressureDsrd - pressureCrnt) / pressureDsrd;
    dwellTime = dwellTime * 0.5f; /* Max dwell of 1/2 inverval */
    dwellTime = dwellTime * (float)self->refreshPeriod;
    dwellTime = dwellTime > 0.0f ? fmaxf(dwellTime, DWELL_MIN) : 0;

    /* Send message */
    address += (I2C_DeviceAddress)KegItem_getTapNo(self);
    holdTime = (uint16_t)(dwellTime * 1000);
    Satellite_GpioSet(address, Satellite_OutputId_GasEn, &gasEn, holdTime);

    /* Update statistics */
    kegItem = KegItem_getSiblingByKey(self, "PressureDwellTime");
    if((result!=0) && (kegItem->value != NULL)) {
        dwellTime += *(float*)kegItem->value;
        kegItem->value_set(kegItem, &dwellTime);
        //TODO: Push to Db (Mark value dirty) -- save on message count for now.
    }

    return(result == 0);
}

/*-----------------------------------------------------------------------------
Process DateAvailable field
 - Enable tap if appropriate
 - TODO: Somehow modify only enable beer 'once-ish'
-----------------------------------------------------------------------------*/
int KegItem_ProcDateAvail(KegItem_obj* self) {
    bool b;
    KegItem_obj* pourEn;
    time_t t = time(NULL);
    struct tm time = *localtime(&t);
    if (self->value == NULL) {
        return(0);
    }

    //KegMaster_SatelliteMsgType msg;
    pourEn = KegItem_getSiblingByKey(self, "PourEn");

    if ((b = DateTime_Compare(self->value, &time)) && pourEn != NULL && !*(bool*)pourEn->value) {
        pourEn->value_set(pourEn, &b);
    }

    return(0);
}


/*-----------------------------------------------------------------------------
Process DateAvailable field
 - Enable tap if appropriate
 - TODO: Somehow modify only enable beer 'once-ish'
-----------------------------------------------------------------------------*/
int KegItem_ProcAlerts(KegItem_obj* self) {
    #define MSG "Test Alert. Please Ignore"
    char* s = MSG; 

    self->value_set(self, &s);
    //self->value_dirty = true;

    return(0);
}
