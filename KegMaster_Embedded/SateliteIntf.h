#pragma once

#pragma once

/*
 * Contains definitions to support inter-controller communications
 */
#ifndef KEGMASTER_SATELLITE_H
#define KEGMASTER_SATELLITE_H

#pragma pack(1) 
typedef enum uint8_t {
    Satellite_MsgId_GpioRead,
    Satellite_MsgId_GpioSet,
    Satellite_MsgId_GpioSetDflt,
    Satellite_MsgId_InterruptRead,
    Satellite_MsgId_InterruptReset,
    Satellite_MsgId_ADCRead
}KegMaster_SatelliteMsgId;

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