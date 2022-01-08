

#include <stdlib.h>

/*
 *	config.h needs to come first
 */
#include "config.h"

#include "fsl_misc_utilities.h"
#include "fsl_device_registers.h"
#include "fsl_i2c_master_driver.h"
#include "fsl_spi_master_driver.h"
#include "fsl_rtc_driver.h"
#include "fsl_clock_manager.h"
#include "fsl_power_manager.h"
#include "fsl_mcglite_hal.h"
#include "fsl_port_hal.h"

#include "gpio_pins.h"
#include "SEGGER_RTT.h"
#include "warp.h"


extern volatile WarpI2CDeviceState	deviceINA219State;
extern volatile uint32_t		gWarpI2cBaudRateKbps;
extern volatile uint32_t		gWarpI2cTimeoutMilliseconds;
extern volatile uint32_t		gWarpSupplySettlingDelayMilliseconds;



void
initINA219(const uint8_t i2cAddress, uint16_t operatingVoltageMillivolts)
{
	deviceINA219State.i2cAddress			= i2cAddress;
	deviceINA219State.operatingVoltageMillivolts	= operatingVoltageMillivolts;

	return;
}

WarpStatus
writeSensorRegisterINA219(uint8_t deviceRegister, uint8_t payload1, uint8_t payload2)
{
	uint8_t		commandByte[1];
	uint8_t		payloadSize = 2;
	uint8_t 	payloadByte[2];
	i2c_status_t	status;
	

	switch (deviceRegister)
	{
		case 0x00: case 0x05:
		{
			/* OK */
			break;
		}
		
		default:
		{
			return kWarpStatusBadDeviceCommand;
		}
	}

	i2c_device_t slave =
	{
		.address = 0x40,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);
	commandByte[0] = deviceRegister;
	payloadByte[0]=payload1;
	payloadByte[1]=payload2;
	warpEnableI2Cpins();

	status = I2C_DRV_MasterSendDataBlocking(
							0 /* I2C instance */,
							&slave,
							commandByte,
							1,
							payloadByte,
							2,
							gWarpI2cTimeoutMilliseconds);
	if (status != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}

WarpStatus
configureSensorINA219(uint8_t payload1, uint8_t payload2)
{
	WarpStatus	i2cWriteStatus1;


	warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);

	i2cWriteStatus1 = writeSensorRegisterINA219(kWarpSensorConfigurationRegisterINA219CONFIGURATION /* register address configuration */,
							payload1,payload2 /* payload: Default  */
							);

	return (i2cWriteStatus1);
}

WarpStatus
readSensorRegisterINA219(uint8_t deviceRegister, int numberOfBytes)
{
	uint8_t		cmdBuf[1] = {0xFF};
	i2c_status_t	status;


	USED(numberOfBytes);
	switch (deviceRegister)
	{
	
		case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05:
		{
			/* OK */
			break;
		}
		
		default:
		{
			return kWarpStatusBadDeviceCommand;
		}
	}

	i2c_device_t slave =
	{
		.address = deviceINA219State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);
	cmdBuf[0] = deviceRegister;
	warpEnableI2Cpins();

	status = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceINA219State.i2cBuffer,
							numberOfBytes,
							gWarpI2cTimeoutMilliseconds);

	if (status != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}

void
printSensorDataINA219(bool hexModeFlag)
{
	uint16_t        readSensorRegisterValueLSB;
        uint16_t        readSensorRegisterValueMSB;
        int16_t         readSensorRegisterValueCombined;
	WarpStatus	i2cReadStatus;


	warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);


	i2cReadStatus = readSensorRegisterINA219(kWarpSensorOutputRegisterINA219SHUNT_VOLTAGE, 2 /* numberOfBytes */);

	readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
        readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
        readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 8) | (readSensorRegisterValueLSB & 0xFF);

	if (i2cReadStatus != kWarpStatusOK)
	{
		warpPrint(" ----,");
	}
	else
	{
		if (hexModeFlag)
		{
		warpPrint(" 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB);
		}
		else
		{
			warpPrint(" %d,", readSensorRegisterValueCombined);
		}
	}

	i2cReadStatus = readSensorRegisterINA219(kWarpSensorOutputRegisterINA219BUS_VOLTAGE, 2 /* numberOfBytes */);
	readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
        readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
        readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 8) | (readSensorRegisterValueLSB & 0xFF);

	if (i2cReadStatus != kWarpStatusOK)
	{
		warpPrint(" ----,");
	}
	else
        {
                if (hexModeFlag)
                {
                warpPrint(" 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB);
                }
                else
                {
                        warpPrint(" %d,", readSensorRegisterValueCombined);
                }
        }


	i2cReadStatus = readSensorRegisterINA219(kWarpSensorOutputRegisterINA219POWER, 2 /* numberOfBytes */);
        readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
        readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
        readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 8) | (readSensorRegisterValueLSB & 0xFF);


        if (i2cReadStatus != kWarpStatusOK)
        {
                warpPrint(" ----,");
        }
        else
        {
                if (hexModeFlag)
                {
                warpPrint(" 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB);
                }
                else
                {
                        warpPrint(" %d,", readSensorRegisterValueCombined);
                }
        }
	
	i2cReadStatus = readSensorRegisterINA219(kWarpSensorOutputRegisterINA219CURRENT, 2 /* numberOfBytes */);
        readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
        readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
        readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 8) | (readSensorRegisterValueLSB & 0xFF);



        if (i2cReadStatus != kWarpStatusOK)
        {
                warpPrint(" ----,");
        }
        else
        {
                if (hexModeFlag)
                {
                warpPrint(" 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB);
                }
                else
                {
                        warpPrint(" %d,", readSensorRegisterValueCombined);
                }
        }

}

