
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "applibs/i2c.h"
#include "mt3620_avnet_dev.h"
#include <applibs/log.h>

#include "SatelliteIntf.h"

int i2cFd = -1;

#define procErr(err) \
    if (err != 0) { \
        Log_Debug("ERROR: %s(): errno=%d (%s)\n", __func__, errno, strerror(errno)); \
    }


int Satellite_Init(void) {

	// Begin MT3620 I2C init 

	i2cFd = I2CMaster_Open(MT3620_RDB_HEADER4_ISU2_I2C);
	if (i2cFd < 0) {
		Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	int result = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
	if (result != 0) {
		Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	result = I2CMaster_SetTimeout(i2cFd, 100);
	if (result != 0) {
		Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

    return 0;
}



int Satellite_GpioRead(I2C_DeviceAddress address, uint8_t pinId, bool* data) {
    Satellite_MsgId cmd = Satellite_MsgId_GpioRead;
    Satellite_MsgType msg;
    Satellite_MsgType reply;
    int err = EINVAL;

    if (data != NULL) {
        msg.id = cmd;
        msg.data.gpio.id = pinId;
        msg.msg_trm = 0x04FF;

        err = 0 != I2CMaster_WriteThenRead(i2cFd, address, (uint8_t*)&msg, sizeof(msg), (uint8_t*)&reply, sizeof(reply)) ? 0 : err;
        *data = reply.data.gpio.state;
    }

    procErr(err)
    return(err);
}

int Satellite_GpioSet(I2C_DeviceAddress address, uint8_t pinId, bool* data, uint16_t holdTime) {
    Satellite_MsgId cmd = Satellite_MsgId_GpioSet;
    Satellite_MsgType msg;
    int err = EINVAL;

    if (data != NULL) {
        msg.id = cmd;
        msg.data.gpio.id = pinId;
        msg.data.gpio.state = *data;
        msg.data.gpio.holdTime = holdTime;
        msg.msg_trm = 0x04FF;
        err = 0 != I2CMaster_Write(i2cFd, address, (uint8_t*)&msg, sizeof(msg)) ? 0 : err;
    }

    procErr(err)
    return(err);
}

int Satellite_GpioSetDflt(I2C_DeviceAddress address, uint8_t pinId, bool* data) {
    #define DFLT_HOLD_TIME 1000; /* This does nothing atm but is here in case I change my mind in the future */
    Satellite_MsgId cmd = Satellite_MsgId_GpioSetDflt;
    Satellite_MsgType msg;
    int err = EINVAL;

    if (data != NULL) {
        msg.id = cmd;
        msg.data.gpio.id = pinId;
        msg.data.gpio.state = *data;
        msg.data.gpio.holdTime = DFLT_HOLD_TIME;
        msg.msg_trm = 0x04FF;
        err = 0 != I2CMaster_Write(i2cFd, address, (uint8_t*)&msg, sizeof(msg)) ? 0 : err;
    }

    procErr(err)
    return(err);
}

int Satellite_InterruptRead(I2C_DeviceAddress address, uint8_t pinId, uint32_t* data) {
    Satellite_MsgId cmd = Satellite_MsgId_InterruptRead;
    Satellite_MsgType msg;
    Satellite_MsgType reply;
    int err = EINVAL;

    if (data != NULL) {
        msg.id = cmd;
        msg.data.intrpt.id = pinId;
        msg.msg_trm = 0x04FF;
        err = 0 != I2CMaster_WriteThenRead(i2cFd, address, (uint8_t*)&msg, sizeof(msg), (uint8_t*)&reply, sizeof(reply)) ? 0 : err;
        *data = reply.data.intrpt.count;
    }

    procErr(err)
    return(err);
}

int Satellite_InterruptReset(I2C_DeviceAddress address, uint8_t pinId) {
    Satellite_MsgId cmd = Satellite_MsgId_InterruptReset;
    Satellite_MsgType msg;
    int err = EINVAL;

    msg.id = cmd;
    msg.data.intrpt.id = pinId;
    msg.msg_trm = 0x04FF;
    err = 0 != I2CMaster_Write(i2cFd, address, (uint8_t*)&msg, sizeof(msg)) ? 0 : err;

    procErr(err)
    return(err);
}

int Satellite_ADCRead(I2C_DeviceAddress address, uint8_t pinId, uint32_t* data) {
    Satellite_MsgId cmd = Satellite_MsgId_ADCRead;
    Satellite_MsgType msg;
    Satellite_MsgType reply;
    int err = EINVAL;

    if (data != NULL) {
        msg.id = cmd;
        msg.data.adc.id = pinId;
        msg.msg_trm = 0x04FF;

        *data = reply.data.adc.value;
        err = 0 != I2CMaster_WriteThenRead(i2cFd, address, (uint8_t*)&msg, sizeof(msg), (uint8_t*)&reply, sizeof(reply)) ? 0 : err;
        *data = reply.data.adc.value;
    }

    procErr(err)
    return(err);
}
