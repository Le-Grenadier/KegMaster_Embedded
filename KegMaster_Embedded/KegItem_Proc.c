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
#include "SateliteIntf.h"

extern int i2cFd;

/*-----------------------------------------------------------------------------
KegItem data processing callback functions
 - This is where either data will be 'cleaned' or otherwise acted upon.
 - TODO: Reduce function complexity
-----------------------------------------------------------------------------*/
int KegItem_ProcPourEn(KegItem_obj* self) {
#define POUR_DELAY 5 /* Seconds */ 
#define POUR_INT_CNVT 1000 /* Interrupts per oz */

    static bool disable = false;
    static struct timespec lockoutTimer = { 0, 0 };

    int ret = 0;
    KegItem_obj* avail;
    KegItem_obj* rsrv;
    KegItem_obj* szPour;
    KegItem_obj* szSmpl;
    KegItem_obj* tapNo;
    KegItem_obj* pourEn;

    uint16_t pourQty;
    uint32_t hold_time;
    struct timespec realTime;

    I2C_DeviceAddress address = 0x8; // Base address chosen at random-ish
    int err;
    Satellite_MsgType qtyMsg;
    Satellite_MsgType pourMsg;

    bool result;

    avail = KegItem_getSiblingByKey(self, "QtyAvailable");
    rsrv = KegItem_getSiblingByKey(self, "QtyReserve");
    szPour = KegItem_getSiblingByKey(self, "PourQtyGlass");
    szSmpl = KegItem_getSiblingByKey(self, "PourQtySample");
    tapNo = KegItem_getSiblingByKey(self, "TapNo");
    pourEn = self;
    clock_gettime(CLOCK_REALTIME, &realTime);

    if (tapNo == NULL || avail == NULL || rsrv == NULL || szPour == NULL || szSmpl == NULL
        || tapNo->value == NULL) {
        return(0);
    }
    /* Update Satellite MCU address */
    address += *(I2C_DeviceAddress*)tapNo->value;

    /*=========================================================================
    Qty Poured Already
    =========================================================================*/

    /* Get Current qty poured */
    qtyMsg.id = Satellite_MsgId_InterruptRead;
    qtyMsg.data.intrpt.id = 0;
    qtyMsg.msg_trm = 0x04FF;
    result = -1;
    result = I2CMaster_WriteThenRead(i2cFd, address, (uint8_t*)&qtyMsg, sizeof(qtyMsg), (uint8_t*)&qtyMsg, sizeof(qtyMsg));
    if (result != 0) {
        err = errno;
        Log_Debug("ERROR: TFMini Soft Reset Fail: errno=%d (%s)\n", err, strerror(err));
    }
    pourQty = qtyMsg.data.intrpt.count / POUR_INT_CNVT;

    /*-------------------------------------------------------------------------
    Disable pour temporarily if
       - not presently locked-out
       - pouring enabled, glass has been dispensed
       - pouring disabled, sampling enabled, and sample has been served
    -------------------------------------------------------------------------*/
    if ((realTime.tv_sec >= lockoutTimer.tv_sec)
        && ( ( *(bool*)pourEn->value && (*(float*)szPour->value > 0.1) && (pourQty >= *(float*)szPour->value))
          || (!*(bool*)pourEn->value && (*(float*)szSmpl->value > 0.1) && (pourQty >= *(float*)szSmpl->value)))) {
        lockoutTimer.tv_sec = realTime.tv_sec + POUR_DELAY;
    }

    /*=========================================================================
    Stop if below reserve value
    =========================================================================*/
    if ((*(bool*)self->value)
        && (*(float*)avail->value <= *(float*)rsrv->value)
        && (*(float*)avail->value + pourQty > * (float*)rsrv->value)) {
        self->value_set(self, &disable);
        //self->value_dirty = true;
    }

    /*=========================================================================
    Set ouput state
    =========================================================================*/
    /* Enable Pouring based on state, if not locked-out                                 */
    /*  - Enable for 2x refresh period so an error won't leave it unlocked indefinitely */
    hold_time = (uint32_t)self->refreshPeriod * 2 * 1000;

    pourMsg.id = Satellite_MsgId_GpioSet;
    pourMsg.data.gpio.id = 3;
    pourMsg.data.gpio.state = (*(bool*)pourEn->value) && (realTime.tv_sec >= lockoutTimer.tv_sec);
    pourMsg.data.gpio.holdTime = hold_time > UINT16_MAX ? UINT16_MAX : (uint16_t)hold_time;
    pourMsg.msg_trm = 0x04FF;
    result = -1;
    result = I2CMaster_Write(i2cFd, address, (uint8_t*)&pourMsg, sizeof(pourMsg));
    if (result != 0) {
        err = errno;
        Log_Debug("ERROR: TFMini Soft Reset Fail: errno=%d (%s)\n", err, strerror(err));
    }

    return(ret = 1);
}


int KegItem_ProcPressureDsrd(KegItem_obj* self)
{
#define DWELL_MIN 0.02f

    KegItem_obj* pressCrnt;
    KegItem_obj* tapNo;
    I2C_DeviceAddress address = 0x8; // Base address chosen at random-ish
    float pressure_crnt;
    float pressure_dsrd;
    float dwell_time;
    int err;
    KegItem_obj* keg_item;
    Satellite_MsgType msg;
    bool result;

    if (self->value == NULL) {
        return(false);
    }

    tapNo = KegItem_getSiblingByKey(self, "TapNo");
    pressCrnt = KegItem_getSiblingByKey(self, "PressureCrnt");
    if (tapNo == NULL || pressCrnt == NULL
        || tapNo->value == NULL) {
        return(0);
    }
    pressure_crnt = *(float*)pressCrnt->value;
    pressure_dsrd = *(float*)self->value;

    // [insert fancy control algorithm here] 
    /* For now, just proportional control
        - Minimum dwell time = 20 ms (mostly chosen at random, based on other, similar valves.)
        - Calculate dwell time as percentage of interval */
    dwell_time = (pressure_dsrd - pressure_crnt) / pressure_dsrd;
    dwell_time = dwell_time * 0.5f; /* Max dwell of 1/2 inverval */
    dwell_time = dwell_time * (float)self->refreshPeriod;
    dwell_time = dwell_time > 0.0f ? fmaxf(dwell_time, DWELL_MIN) : 0;

    /* Send message */
    address += *(I2C_DeviceAddress*)tapNo->value;
    msg.id = Satellite_MsgId_GpioSet;
    msg.data.gpio.id = 2;
    msg.data.gpio.state = 1;
    msg.data.gpio.holdTime = (uint16_t)(dwell_time * 1000);
    msg.msg_trm = 0x04FF;
    result = -1;
    result = I2CMaster_Write(i2cFd, address, (uint8_t*)&msg, sizeof(msg));

    if (result != 0) {
        err = errno;
        Log_Debug("ERROR: TFMini Soft Reset Fail: errno=%d (%s)\n", err, strerror(err));
    }

    /* Update statistics */
    keg_item = KegItem_getSiblingByKey(self, "PressureDwellTime");
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

