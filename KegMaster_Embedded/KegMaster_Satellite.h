#pragma once

/*
 * Contains definitions to support inter-controller communications
 */
#ifndef KEGMASTER_SATELLITE_H
#define KEGMASTER_SATELLITE_H

#pragma pack(1) 
typedef enum uint8_t {
	KegMaster_SateliteMsgId_GpioRead,
	KegMaster_SateliteMsgId_GpioSet,
	KegMaster_SateliteMsgId_GpioSetDflt,
	KegMaster_SateliteMsgId_InterruptRead,
	KegMaster_SateliteMsgId_InterruptReset,
	KegMaster_SateliteMsgId_ADCRead
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
} KegMaster_SatelliteMsgType;

#endif