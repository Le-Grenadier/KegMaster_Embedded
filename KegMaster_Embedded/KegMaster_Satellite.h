#pragma once

/*
 * Contains definitions to support inter-controller communications
 */
#ifndef KEGMASTER_SATELLITE_H
#define KEGMASTER_SATELLITE_H

typedef enum {
	KegMaster_SateliteMsgId_GpioRead,
	KegMaster_SateliteMsgId_GpioSet,
	KegMaster_SateliteMsgId_GpioSetDflt,
	KegMaster_SateliteMsgId_InterruptRead,
	KegMaster_SateliteMsgId_InterruptReset,
	KegMaster_SateliteMsgId_ADCRead
}KegMaster_SatelliteMsgId;


typedef struct {
	KegMaster_SatelliteMsgId id;
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
} KegMaster_SatelliteMsgType;

#endif