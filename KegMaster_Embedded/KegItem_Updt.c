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
#include "SatelliteIntf.h"


/*=============================================================================
KegItem Hw and DB accessors
=============================================================================*/

int KegItem_HwGetPressureCrnt(KegItem_obj* self) {
    I2C_DeviceAddress address = 0x8; // Base address chosen at random-ish
    uint32_t pressure_raw;
    float pressure;

    if (self->value == NULL) {
        float f;
        f = 0.0f;
        self->value_set(self, &f);
    }
    address += (I2C_DeviceAddress)KegItem_getTapNo(self);
    Satellite_ADCRead(address, Satellite_AdcId_Pressure, &pressure_raw);
    pressure = (float)pressure_raw; 
    pressure = pressure; // Do some conversion here later (need to adjust for STP and all that)
    pressure = fmaxf(pressure, 0.0f);
    pressure = fminf(pressure, 99.9f);

    return((int)pressure);
}


int KegItem_HwGetQtyAvail(KegItem_obj* self) {
    #define WEIGHT_UPDT_TOL 100 /* Grams */ / 1000.0f

    I2C_DeviceAddress address = 0x8; // Base address chosen at random-ish
    bool result;
    float weight;
    uint32_t weight_raw;

    if (self->value == NULL) {
        float f;
        f = 0.0f;
        self->value_set(self, &f);
    }

    address += (I2C_DeviceAddress)KegItem_getTapNo(self);
    result = Satellite_ADCRead(address, Satellite_AdcId_Scale, &weight_raw);

    /* Convert to pints and adjust for keg weight */
    weight = (float)weight_raw;   // Raw data
    weight = weight - 146400;     // Offset for zero
    weight = weight / 23.0f;      // convert to grams
    weight = weight / 1000.0f;    // convert to liters
    weight = weight * 2.11338f;   // Convert to pints
    weight = weight < 7 ? weight : weight - 7.0f;       // Offset for Keg weight (about 7 pints) -- TODO: Impliment better tare functionality.

    /* Reasonableness checks */
    weight = fmaxf(weight, 0.0f);
    weight = fminf(weight, 999.9f);

    if(result != 0) {
        bool dfltOff = false;

        /* HX711 Scale Clk needs to be default low - PIC should default to low but put this here just in case */
        Satellite_GpioSetDflt(address, Satellite_OutputId_HX711, &dfltOff);
    }
    else{

        /* Reduce message Tx spam (and Azure bill) - only send update if change is notable */
        if ((fabsf((*(float*)self->value) - weight) > WEIGHT_UPDT_TOL)) {
            self->value_dirty = true;
            self->value_set(self, &weight);
        }
        Log_Debug("INFO: Keg %d weighs %f - Value Dirty %d\n", KegItem_getTapNo(self), weight, self->value_dirty);

    }

    return(result == 0);
}
