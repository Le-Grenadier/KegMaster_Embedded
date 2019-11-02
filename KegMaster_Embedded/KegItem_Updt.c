#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include <applibs/i2c.h>
#include <applibs/log.h>

#include "KegItem.h"
#include "KegItem_Updt.h"
#include "KegItem_Utl.h"
//#include "KegMaster.h"
#include "SateliteIntf.h"


extern int i2cFd;

/*=============================================================================
KegItem Hw and DB accessors
=============================================================================*/

int KegItem_HwGetPressureCrnt(KegItem_obj* self) {
    I2C_DeviceAddress address = 0x8; // Base address chosen at random-ish
    int err;
    KegItem_obj* tapNo;
    Satellite_MsgType msg;
    bool result;
    float pressure;

    tapNo = KegItem_getSiblingByKey(self, "TapNo");
    if (tapNo == NULL || tapNo->value == NULL) {
        return(false);
    }

    if (self->value == NULL) {
        float f;
        f = 0.0f;
        self->value_set(self, &f);
    }
    address += *(I2C_DeviceAddress*)tapNo->value;
    msg.id = Satellite_MsgId_ADCRead;
    msg.data.adc.id = 0;
    msg.data.adc.value = 0;
    result = I2CMaster_WriteThenRead(i2cFd, address, (uint8_t*)&msg, sizeof(msg), (uint8_t*)&msg, sizeof(msg));
    pressure = (float)msg.data.adc.value; // Do some conversion here later (need to adjust for STP)
    if (result != 0) {
        err = errno;
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
    int err;
    Satellite_MsgType msg_tx;
    Satellite_MsgType msg_rx;
    bool result;
    float weight;

    if (self->value == NULL) {
        float f;
        f = 0.0f;
        self->value_set(self, &f);
    }

    tapNo = KegItem_getSiblingByKey(self, "TapNo");
    if (tapNo == NULL
        || tapNo->value == NULL) {
        return(0);
    }

    tapNo_value = *(int*)tapNo->value;
    address += (I2C_DeviceAddress)tapNo_value;
    msg_tx.id = Satellite_MsgId_ADCRead;
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
    if ((result != true)
        || (weight != fmaxf(weight, 0.0f))    /* Going into beverage-debt with kegs not supported     */
        || (weight != fminf(weight, 800.0f))) { /* Kegs greater than 100 gallons not supported          */
        err = errno;
        Log_Debug("ERROR: Failed to get Keg %d weight: errno=%d (%s)\n", tapNo_value, strerror(err));
        weight = INFINITY;
    }
    else {

        /* Reduce message Tx spam (and Azure bill) - only send update if change is notable */
        if ((weight < INFINITY) && (fabsf((*(float*)self->value) - weight) > WEIGHT_UPDT_TOL)) {
            self->value_dirty = true;
        }

        Log_Debug("INFO: Keg %d weighs %f\n", tapNo_value, weight);
        self->value_set(self, &weight);
    }

    return(0);
}
