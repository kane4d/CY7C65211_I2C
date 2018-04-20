#pragma once
#include "cyusb_i2c.h"
#define MAX_DEVICE_NUM (16)
namespace cyusb {
	class i2c
	{
	private:
		UINT32 timeout = 5000;
	public:
		CY_HANDLE cyHandle;
		CY_I2C_CONFIG cyConfig;
		CY_I2C_DATA_CONFIG cyI2CDataConfig;
		CY_DATA_BUFFER cyDataBuffer;
		CY_DEVICE_INFO cyDeviceInfo, cyDeviceInfoList[MAX_DEVICE_NUM];
		uint8_t cyDeviceId[MAX_DEVICE_NUM];
		uint8_t cyNumDevices;
		CY_RETURN_STATUS retStat = CY_SUCCESS;
		i2c()
		{
#ifndef _WIN32
			retStat=CyLibraryInit();
#endif
		}
		~i2c()
		{
#ifndef _WIN32
			retStat = CyLibraryExit();
#endif
		}
		CY_RETURN_STATUS open()
		{
			return CyOpen(1, 0, &cyHandle);
		}
		CY_RETURN_STATUS read()
		{
			auto ret = CY_SUCCESS;
			ret = CyI2cRead(cyHandle, &cyI2CDataConfig, &cyDataBuffer, timeout);
			return ret;
		}
		CY_RETURN_STATUS write() 
		{
			auto ret = CY_SUCCESS;
			ret = CyI2cWrite(cyHandle, &cyI2CDataConfig, &cyDataBuffer, timeout);
			return ret;
		}
	};
}

