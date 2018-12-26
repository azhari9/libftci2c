/*++

Copyright (c) 2008  Future Technology Devices International Ltd.

Module Name:

    FT2232cMpsseI2c.h

Abstract:

    FT2232H and FT4232H Hi-Speed Dual Device and FT2232D Dual Device Class Declaration/Definition.

Environment:

    kernel & user mode

Revision History:

    23/03/05    kra     Created.
    XX/XX/XX    ana		  Version 1.14
    23/05/08    kra     Version 1.2 - Modified FT2232.cpp to fix two bugs, if more than one device
                        is connected with the same device name, an invalid location id error is reported and changed
                        Sleep(0) to Sleep(1) to solve intermittent problem with FTC_Timeout failure
    03/06/08    kra     Version 1.3, added 3 new functions ie SetGeneralPurposeHighInputOutputPins, GetGeneralPurposeHighInputOutputPinsInputStates,
                        and GetGeneralPurposeHighInputOutputPins to control the output states ie high(true) or low(false)
                        of the 4 general purpose higher input/output ACBUS 0-3/GPIOH 0-3) pins
    01/08/08    kra     Renamed FT2232cMpsseI2c.h to FT2232cMpsseI2c.h for FT2232H and FT4232H hi-speed devices
    01/08/08    kra     Version 2.0 - Added new functions for FT2232H and FT4232H hi-speed devices.
    20/08/08    kra     Added new function SetTCKTDITMSPinsCloseState.
    03/09/08    kra     Added CRITICAL_SECTION object.
	
--*/

#ifndef FT2232hMpsseI2c_H
#define FT2232hMpsseI2c_H

#include "WinTypes.h"

#include "FTCI2C.h"

#include "FT2232h.h"
#include "stdafx.h"
#define DEVICE_CHANNEL_A " A"
#define DEVICE_CHANNEL_B " B"


#define USB_INPUT_BUFFER_SIZE 65536  // 64K
#define USB_OUTPUT_BUFFER_SIZE 65536  // 64K

#define FT_EVENT_VALUE  0
#define FT_ERROR_VALUE  0

#define DEVICE_READ_TIMEOUT_INFINITE 0
#define DEVICE_WRITE_TIMEOUT 5000 // 5 seconds

#define MIN_CLOCK_DIVISOR 0     // equivalent to 6MHz
#define MAX_CLOCK_DIVISOR 65535 // equivalent to 91Hz

#define PIN1_HIGH_VALUE  1
#define PIN2_HIGH_VALUE  2
#define PIN3_HIGH_VALUE  4
#define PIN4_HIGH_VALUE  8
#define PIN5_HIGH_VALUE  16
#define PIN6_HIGH_VALUE  32
#define PIN7_HIGH_VALUE  64
#define PIN8_HIGH_VALUE  128

#define MIN_NUM_CONTROL_BYTES 1          // specifies the minimum number of control bytes that can be written to an external device
#define MAX_NUM_CONTROL_BYTES 255        // specifies the maximum number of control bytes that can be written to an external device
#define MIN_NUM_WRITE_DATA_BYTES 1       // specifies the minimum number of data bytes that can be written to an external device
#define MAX_NUM_WRITE_DATA_BYTES 65535   // specifies the maximum number of data bytes that can be written to an external device
#define MIN_NUM_READ_DATA_BYTES 1        // specifies the minimum number of data bytes that can be read from an external device
#define MAX_NUM_READ_DATA_BYTES 65535    // specifies the maximum number of data bytes that can be read from an external device

#define I2C_WRITE_BUFFER_SIZE 8  // Changed to 8 was 64K (ANA)

typedef BYTE I2CWriteByteBuffer[I2C_WRITE_BUFFER_SIZE];
typedef I2CWriteByteBuffer *PI2CWriteByteBuffer;

#define I2C_READ_BUFFER_SIZE 8  // Changed to 8 was 64K (ANA)

typedef BYTE I2CReadByteBuffer[I2C_READ_BUFFER_SIZE];
typedef I2CReadByteBuffer *PI2CReadByteBuffer;

enum AckTypes {NoAck, ControlAck, DataAck};

// These two literals define the number of times commands are repeated to ensure that the minimum periods
// for the start hold and setup times ie 600ns and stop setup time ie 600ns is achieved to ensure correct
// communication with I2C devices
#define START_HI_SPEED_DEVICE_HOLD_SETUP_TIME 3
#define STOP_HI_SPEED_DEVICE_SETUP_TIME       3

#define PIN1_HIGH_VALUE  1
#define PIN2_HIGH_VALUE  2
#define PIN3_HIGH_VALUE  4
#define PIN4_HIGH_VALUE  8

typedef struct Ft_I2C_Device_Data{
  DWORD hDevice;                                    // handle to the opened and initialized FT2232C dual type device
  DWORD dwCommsMode;
}FTC_I2C_DEVICE_DATA, *PFTC_I2C_DEVICE_DATA;


//----------------------------------------------------------------------------
  DWORD dwSavedLowPinsDirection;
  DWORD dwSavedHighPinsDirection;
  DWORD dwSavedHighPinsValue;
  InputByteBuffer InputBuffer;

  FTC_STATUS CheckWriteDataToExternalDeviceParameters(PWriteControlByteBuffer pWriteControlBuffer, DWORD dwNumControlBytesToWrite,
                                                      DWORD dwDataWriteTypes, PWriteDataByteBuffer pWriteDataBuffer,
                                                      DWORD dwNumDataBytesToWrite, PFTC_PAGE_WRITE_DATA pPageWriteData);
  FTC_STATUS CheckReadDataFromExternalDeviceParameters(PWriteControlByteBuffer pWriteControlBuffer, DWORD dwNumControlBytesToWrite,
                                                       DWORD dwDataReadTypes, PReadDataByteBuffer pReadDataBuffer,
                                                       DWORD dwNumDataBytesToRead);

  FTC_STATUS SetTCKTDITMSPinsCloseState(FTC_HANDLE ftHandle, PFTC_CLOSE_FINAL_STATE_PINS pCloseFinalStatePinsData);
  FTC_STATUS InitDevice(FTC_HANDLE ftHandle, DWORD dwClockDivisor);
  FTC_STATUS SetDataInOutClockFrequency(FTC_HANDLE ftHandle, DWORD dwClockDivisor);
  FTC_STATUS InitDataInOutClockFrequency(FTC_HANDLE ftHandle, DWORD dwClockDivisor);
  FTC_STATUS EmptyDeviceInputBuffer(FTC_HANDLE ftHandle);

  FTC_STATUS SetGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, DWORD dwHighPinsDirection, DWORD dwHighPinsValue);
  FTC_STATUS SetHiSpeedDeviceGeneralPurposeLowInputOutputPins(FTC_HANDLE ftHandle, PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData);
  FTC_STATUS SetHiSpeedDeviceGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, PFTH_INPUT_OUTPUT_PINS pHighInputOutputPinsData);
  void       GetGeneralPurposeHighInputOutputPinsInputStates(DWORD dwInputStatesReturnedValue, PFTC_LOW_HIGH_PINS pPinsInputData);
  FTC_STATUS GetGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, PFTC_LOW_HIGH_PINS pHighPinsInputData);
  void       GetHiSpeedDeviceGeneralPurposeLowInputOutputPinsInputStates(DWORD dwInputStatesReturnedValue, PFTC_LOW_HIGH_PINS pLowPinsInputData);
  FTC_STATUS GetHiSpeedDeviceGeneralPurposeLowInputOutputPins(FTC_HANDLE ftHandle, PFTC_LOW_HIGH_PINS pLowPinsInputData);
  void       GetHiSpeedDeviceGeneralPurposeHighInputOutputPinsInputStates(DWORD dwInputStatesReturnedValue, PFTH_LOW_HIGH_PINS pPinsInputData);
  FTC_STATUS GetHiSpeedDeviceGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, PFTH_LOW_HIGH_PINS pHighPinsInputData);

  void       SetI2CStartCondition(FTC_HANDLE ftHandle, BOOL bEnableDisableWriteProtect);
  FTC_STATUS SetI2CStopCondition(FTC_HANDLE ftHandle);
  void       SetI2CWriteData(FTC_HANDLE ftHandle, DWORD dwNumBitsToWrite, PI2CWriteByteBuffer pI2CWriteBuffer);
  void       SetI2CReadData(DWORD dwNumBitsToRead);
  FTC_STATUS ReadDataBytesFromExternalDevice(FTC_HANDLE ftHandle, PI2CReadByteBuffer pI2CReadBuffer, DWORD dwNumBitsToRead, LPDWORD lpdwNumDataBytesRead, BOOL bWaitForAllBytesRead);
  FTC_STATUS ReadDataAckFromExternalDevice(FTC_HANDLE ftHandle, PI2CReadByteBuffer pI2CReadBuffer, DWORD dwNumBitsToRead, int AckType, DWORD dwAckTimeoutmSecs);
  FTC_STATUS ReadDataByteFromExternalDevice(FTC_HANDLE ftHandle, PI2CReadByteBuffer pI2CReadBuffer);
  FTC_STATUS SendDataCheckAcknowledge(FTC_HANDLE ftHandle, BYTE DataByte, int AckType, DWORD dwAckTimeoutmSecs);
  FTC_STATUS WriteAddressExternalDevice(FTC_HANDLE ftHandle, BYTE DeviceAddress, int ControlAckType,
                                        DWORD dwControlAckTimeoutmSecs, BOOL bEnableDisableWriteProtect);
  FTC_STATUS WriteControlToExternalDevice(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                                          DWORD dwNumControlBytesToWrite, int AckType, DWORD dwControlAckTimeoutmSecs);
  FTC_STATUS WriteDataToExternalDevice(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                                       DWORD dwNumControlBytesToWrite, BOOL bControlAcknowledge, DWORD dwControlAckTimeoutmSecs,
                                       BOOL bStopCondition, DWORD dwDataWriteTypes, PWriteDataByteBuffer pWriteDataBuffer,
                                       DWORD dwNumDataBytesToWrite, BOOL bDataAcknowledge, DWORD dwDataAckTimeoutmSecs,
                                       PFTC_PAGE_WRITE_DATA pPageWriteData);
  FTC_STATUS ReadDataFromExternalDevice(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                                        DWORD dwNumControlBytesToWrite, BOOL bControlAcknowledge,
                                        DWORD dwControlAckTimeoutmSecs, DWORD dwDataReadTypes,
                                        PReadDataByteBuffer pReadDataBuffer, DWORD dwNumDataBytesToRead);

  void       CreateDeviceDataRecord(FTC_HANDLE ftHandle);
  INT        GetDeviceDataRecordIndex(FTC_HANDLE ftHandle);
  void       DeleteDeviceDataRecord(FTC_HANDLE ftHandle);
  DWORD      GetCommunicationsMode(FTC_HANDLE ftHandle);
  void		 ClockByte(unsigned char byte);
  void       RepeatBlock(char byte, DWORD num);
  void       WriteProtectEnable(FTC_HANDLE ftHandle, bool b);
  
  FTC_STATUS WINAPI I2C_GetNumDevices_ssel(LPDWORD lpdwNumDevices);
  FTC_STATUS WINAPI I2C_GetNumHiSpeedDevices_ssel(LPDWORD lpdwNumHiSpeedDevices);
  FTC_STATUS WINAPI I2C_GetDeviceNameLocationID(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID);
  FTC_STATUS WINAPI I2C_GetHiSpeedDeviceNameLocationIDChannel(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID, LPSTR lpChannel, DWORD dwChannelBufferSize, LPDWORD lpdwHiSpeedDeviceType);
  FTC_STATUS WINAPI I2C_OpenDevice(FTC_HANDLE *pftHandle);
  FTC_STATUS WINAPI I2C_OpenSpecifiedDevice(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE *pftHandle);
  FTC_STATUS WINAPI I2C_OpenSpecifiedHiSpeedDevice(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, FTC_HANDLE *pftHandle);
  FTC_STATUS WINAPI I2C_GetHiSpeedDeviceType_ssel(FTC_HANDLE ftHandle, LPDWORD lpdwHiSpeedDeviceType);
  FTC_STATUS WINAPI I2C_CloseDevice_ssel(FTC_HANDLE ftHandle);
  FTC_STATUS WINAPI I2C_CloseDevice3(FTC_HANDLE ftHandle, PFTC_CLOSE_FINAL_STATE_PINS pCloseFinalStatePinsData);
  FTC_STATUS WINAPI I2C_InitDevice_ssel(FTC_HANDLE ftHandle, DWORD dwClockFrequencyValue);
  FTC_STATUS WINAPI I2C_TurnOnDivideByFiveClockingHiSpeedDevice_ssel(FTC_HANDLE ftHandle);
  FTC_STATUS WINAPI I2C_TurnOffDivideByFiveClockingHiSpeedDevice_ssel(FTC_HANDLE ftHandle);
  FTC_STATUS WINAPI I2C_TurnOnThreePhaseDataClockingHiSpeedDevice_ssel(FTC_HANDLE ftHandle);
  FTC_STATUS WINAPI I2C_TurnOffThreePhaseDataClockingHiSpeedDevice_ssel(FTC_HANDLE ftHandle);
  FTC_STATUS WINAPI I2C_SetDeviceLatencyTimer_ssel(FTC_HANDLE ftHandle, BYTE LatencyTimermSec);
  FTC_STATUS WINAPI I2C_GetDeviceLatencyTimer_ssel(FTC_HANDLE ftHandle, LPBYTE lpLatencyTimermSec);
  FTC_STATUS WINAPI I2C_GetClock_ssel(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz);
  FTC_STATUS WINAPI I2C_GetHiSpeedDeviceClock_ssel(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz);
  FTC_STATUS WINAPI I2C_SetClock_ssel(FTC_HANDLE ftHandle, DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz);
  FTC_STATUS WINAPI I2C_SetDeviceLoopbackState(FTC_HANDLE ftHandle, BOOL bLoopbackState);
  FTC_STATUS WINAPI I2C_SetCommunicationsMode(FTC_HANDLE ftHandle, DWORD dwCommsMode);
  FTC_STATUS WINAPI I2C_SetGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, PFTC_INPUT_OUTPUT_PINS pHighInputOutputPinsData);
  FTC_STATUS WINAPI I2C_SetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                      PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                                                      BOOL bControlHighInputOutputPins,
                                                                      PFTH_INPUT_OUTPUT_PINS pHighInputOutputPinsData);
  FTC_STATUS WINAPI I2C_GetGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, PFTC_LOW_HIGH_PINS pHighPinsInputData);
  FTC_STATUS WINAPI I2C_GetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                      PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                                                      BOOL bControlHighInputOutputPins,
                                                                      PFTH_LOW_HIGH_PINS pHighPinsInputData);
  FTC_STATUS WINAPI I2C_WriteDataToExternalDevice(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                                                  DWORD dwNumControlBytesToWrite, BOOL bControlAcknowledge,
                                                  DWORD dwControlAckTimeoutmSecs, BOOL bStopCondition, 
                                                  DWORD dwDataWriteTypes, PWriteDataByteBuffer pWriteDataBuffer,
                                                  DWORD dwNumDataBytesToWrite, BOOL bDataAcknowledge,
                                                  DWORD dwDataAckTimeoutmSecs, PFTC_PAGE_WRITE_DATA pPageWriteData);
  FTC_STATUS WINAPI I2C_ReadDataFromExternalDevice(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                                                   DWORD dwNumControlBytesToWrite, BOOL bControlAcknowledge,
                                                   DWORD dwControlAckTimeoutmSecs, DWORD dwDataReadTypes,
                                                   PReadDataByteBuffer pReadDataBuffer, DWORD dwNumDataBytesToRead);
  FTC_STATUS WINAPI I2C_GetErrorCodeString_ssel(LPSTR lpLanguage, FTC_STATUS StatusCode,
                                           LPSTR lpErrorMessageBuffer, DWORD dwBufferSize);


#endif  /* FT2232hMpsseI2c_H */
