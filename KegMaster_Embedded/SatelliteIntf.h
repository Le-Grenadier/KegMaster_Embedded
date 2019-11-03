#pragma once

#include "applibs\i2c.h"

/*
 * Contains definitions to support inter-controller communications
 */
#ifndef KEGMASTER_SATELLITE_H
#define KEGMASTER_SATELLITE_H

#pragma pack(1) 
typedef enum{
    Satellite_MsgId_GpioRead,
    Satellite_MsgId_GpioSet,
    Satellite_MsgId_GpioSetDflt,
    Satellite_MsgId_InterruptRead,
    Satellite_MsgId_InterruptReset,
    Satellite_MsgId_ADCRead
}Satellite_MsgId;

typedef enum {
    Satellite_InputId_undef0,
    Satellite_InputId_undef1,
    Satellite_InputId_undef2,
    Satellite_InputId_undef3
} Satellite_InputId_In;

typedef enum{
    Satellite_OutputId_HX711 = 0,
    Satellite_OutputId_UnDef = 1,
    Satellite_OutputId_GasEn = 2,
    Satellite_OutputId_TapEn = 3
} Satellite_OutputId_Out;


typedef enum {
    Satellite_IntrpId_QtyPoured = 0,
    Satellite_IntrpId_undef1,
    Satellite_IntrpId_undef2,
    Satellite_IntrpId_undef3
} Satellite_ItrptId_In;

typedef enum {
    Satellite_AdcId_undef0,
    Satellite_AdcId_undef1,
    Satellite_AdcId_Pressure = 2,
    Satellite_AdcId_Scale = 3       /* External ADC */
} Satellite_AdcId_Out;



#pragma pack(1) 
typedef struct {
    uint8_t id;
    union {
        struct {
            uint8_t id;
            uint8_t state;
            uint16_t holdTime;
        } gpio;

        struct {
            uint8_t id;
            uint16_t count;
        } intrpt; /* short name BC 'interrupt' is a keyword in MPLAB (apparently) and messes with syntax highlighting */

        struct {
            uint8_t id;
            uint32_t value;
        } adc;
    }data;
    uint16_t msg_trm;
} Satellite_MsgType;

#endif
/*-----------------------------------------------------------------------------
 * Functions and definitions to support I2C interface with satelite PIC modules
 *---------------------------------------------------------------------------*/ 

int Satellite_Init(void);
int Satellite_GpioRead(I2C_DeviceAddress address, uint8_t pinId, bool* data);
int Satellite_GpioSet(I2C_DeviceAddress address, uint8_t pinId, bool* data, uint16_t holdTime);
int Satellite_GpioSetDflt(I2C_DeviceAddress address, uint8_t pinId, bool* data);
int Satellite_InterruptRead(I2C_DeviceAddress address, uint8_t pinId, uint32_t* data);
int Satellite_InterruptReset(I2C_DeviceAddress address, uint8_t pinId);
int Satellite_ADCRead(I2C_DeviceAddress address, uint8_t pinId, uint32_t* data);
