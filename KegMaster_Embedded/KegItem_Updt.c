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
    #define THRESH 0.3f
    #define PIC_ADC_REF_V 3.3f
    #define SNSR_MAX_V 4.5f
    #define PCNT_FULL_RNG (PIC_ADC_REF_V / SNSR_MAX_V)
    #define OUR_PRES_RNG (100.0f * PCNT_FULL_RNG)
    #define OFFST_0v5 (PCNT_FULL_RNG * 0.5f * 1024 / 4.5f) /* 0.5v : 0 Psi reading is from seller's spec, but I'm skeptical. TODO: Confirm */

    I2C_DeviceAddress address = 0x8; 
    uint32_t pressure_raw;
    float percent;
    float pressure;

    if (self->value == NULL) {
        float f;
        f = 0.0f;
        self->value_set(self, &f);
    }
    address += (I2C_DeviceAddress)KegItem_getTapNo(self);
    Satellite_ADCRead(address, Satellite_AdcId_Pressure, &pressure_raw);
    /*-----------------------------------------------------
    Full scale pressure is 0-100psig @ 0.5-4.5 volts on a 10 bit ADC
     - We don't need full scale and PIC I/O is 5v tollerant, so this is okay.
    -----------------------------------------------------*/
    pressure_raw = pressure_raw >> 6; /* PIC has 10 bit ADC built in and is left aligned by default */
    percent = ((pressure_raw - OFFST_0v5) / 1024.0f);
    pressure = OUR_PRES_RNG * percent;
    pressure = fmaxf(pressure, 0.0f);
    pressure = fminf(pressure, 99.9f);
    if (THRESH < abs(pressure - *(float*)self->value)) {
        self->value_set(self, &pressure);
        self->value_dirty = 1;
    }
    Log_Debug("INFO: Keg %d PSI: %f\n", KegItem_getTapNo(self), pressure);

    return((int)pressure);
}

/*-----------------------------------------------------------------------------
Get Quantity available 
 - from HX711 interface, via I2C to PIC 
-----------------------------------------------------------------------------*/
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

    /*-----------------------------------------------------
    Convert to pints and adjust for keg weight
     Scalars (To pints, from ""liters"" as seen below):
      - up to 12 pints == 2.11338
      - In between == may need more work if more accuracy desired.
      - up to 50 pints == 2.11338 / 2.1402
    -----------------------------------------------------*/
    weight = (float)weight_raw;   // Raw data
    weight = weight - 146400;     // Tare
    weight = weight / 23.0f;      // convert to grams
    weight = weight / 1000.0f;    // convert to liters
    weight = weight * 2.11338f;   // Convert to pints
    weight = fminf(weight, 12.0f) + ((fmaxf(weight, 12.0f) - 12.0f) / 2.1402f);

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
            Log_Debug("INFO: Keg %d weighs %f\n", KegItem_getTapNo(self), weight);
        }
    }

    return(result == 0);
}
