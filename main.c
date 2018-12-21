#include "FTCI2C.h"
#include <stdio.h>
#include "FTD2XX.h"
#include "WinTypes.h"
int main()
{
    int ftdi_status;
    FT_HANDLE ftHandle;
    DWORD num_hi_speed_devices;
    DWORD dev_idx;
    char dev_name[200];
    DWORD loc_id;
    char channel[8];
    DWORD hi_speed_device_type;
    DWORD clock_freq;
	
	DWORD BytesWritten; 
	char TxBuffer[256]; // Contains data to write to device
	TxBuffer[0] = 0x8a;
	
    printf("start reading I2C_GetNumHiSpeedDevices\n");
    ftdi_status = I2C_GetNumHiSpeedDevices(&num_hi_speed_devices);
    if (0 != ftdi_status) {
	  printf("I2C_GetNumHiSpeedDevices error");
	  return ftdi_status;
   }

    printf("Dbg (HiCom): NumHiSpeedDevices = %lu\n", num_hi_speed_devices);

    ftdi_status = I2C_GetHiSpeedDeviceNameLocIDChannel(dev_idx, dev_name,
                200, &loc_id, channel, 8, &hi_speed_device_type);

    printf("Dbg (HiCom): DevIdx=%lu, DevName=%s, LocID=%lu, Channel=%s, HiSpeedDeviceType=%lu\n",
               dev_idx, dev_name, loc_id, channel,hi_speed_device_type);

	//ftdi_status = I2C_OpenHiSpeedDevice(dev_name, loc_id , channel, &ftHandle);
	//printf("I2C_OpenHiSpeedDevice %d\n",ftdi_status);
	
    ftdi_status = FT_OpenEx(dev_name,FT_OPEN_BY_DESCRIPTION,&ftHandle);
	printf("FT_OpenEx %d\n",ftdi_status);
	
	ftdi_status = FT_Write(ftHandle, TxBuffer, 1, &BytesWritten);
	printf("FT_Write sts(%d) BytesWritten = %d\n",ftdi_status,BytesWritten);
    //ftdi_status = I2C_Open(ftHandle);
    //printf("I2C_Open %d\n",ftdi_status);

   return ftdi_status;
 //do something
}
