/*++

Copyright (c) 2005  Future Technology Devices International Ltd.

Module Name:

    ftci2c.cpp

Abstract:

    API DLL for FT2232H and FT4232H Hi-Speed Dual Device and FT2232D Dual Device setup to simulate the
    Inter-Integrated Circuit(I2C) synchronous serial protocol.
	  Defines the entry point for the DLL application.

Environment:

    kernel & user mode

Revision History:

    23/03/05    kra   Created.
    03/06/08    kra   Added two new functions ie I2C_SetGPIOs and I2C_GetGPIOs to control the output
                      states ie high(true) or low(false) of the 4 general purpose higher input/output
                      ACBUS 0-3/GPIOH 0-3) pins
    01/08/08    kra   Added new functions for FT2232H and FT4232H hi-speed devices.
    20/08/08    kra   Added new function I2C_CloseDevice.
	
--*/
#include <stdio.h>
#include <stdlib.h>
#include "stdafx.h"

#include "WinTypes.h"

#include "FTCI2C.h"

#include "FT2232hMpsseI2c.h"

//---------------------------------------------------------------------------


FTC_STATUS WINAPI I2C_GetNumDevices(LPDWORD lpdwNumDevices)
{
  return I2C_GetNumDevices_ssel(lpdwNumDevices);
}


FTC_STATUS WINAPI I2C_GetNumHiSpeedDevices(LPDWORD lpdwNumHiSpeedDevices)
{
  return I2C_GetNumHiSpeedDevices_ssel(lpdwNumHiSpeedDevices);
}


FTC_STATUS WINAPI I2C_GetDeviceNameLocID(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID)
{
  return I2C_GetDeviceNameLocationID(dwDeviceNameIndex, lpDeviceNameBuffer, dwBufferSize, lpdwLocationID);
}


FTC_STATUS WINAPI I2C_GetHiSpeedDeviceNameLocIDChannel(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID, LPSTR lpChannel, DWORD dwChannelBufferSize, LPDWORD lpdwHiSpeedDeviceType)
{
  return I2C_GetHiSpeedDeviceNameLocationIDChannel(dwDeviceNameIndex, lpDeviceNameBuffer, dwBufferSize, lpdwLocationID, lpChannel, dwChannelBufferSize, lpdwHiSpeedDeviceType);
}


FTC_STATUS WINAPI I2C_Open(FTC_HANDLE *pftHandle)
{
  return I2C_OpenDevice(pftHandle);
}


FTC_STATUS WINAPI I2C_OpenEx(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE *pftHandle)
{
  return I2C_OpenSpecifiedDevice(lpDeviceName, dwLocationID, pftHandle);
}


FTC_STATUS WINAPI I2C_OpenHiSpeedDevice(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, FTC_HANDLE *pftHandle)
{
  return I2C_OpenSpecifiedHiSpeedDevice(lpDeviceName, dwLocationID, lpChannel, pftHandle);
}


FTC_STATUS WINAPI I2C_GetHiSpeedDeviceType(FTC_HANDLE ftHandle, LPDWORD lpdwHiSpeedDeviceType)
{
  return I2C_GetHiSpeedDeviceType_ssel(ftHandle, lpdwHiSpeedDeviceType);
}


FTC_STATUS WINAPI I2C_Close(FTC_HANDLE ftHandle)
{
  return I2C_CloseDevice_ssel(ftHandle);
}


FTC_STATUS WINAPI I2C_CloseDevice(FTC_HANDLE ftHandle, PFTC_CLOSE_FINAL_STATE_PINS pCloseFinalStatePinsData)
{
  return I2C_CloseDevice3(ftHandle, pCloseFinalStatePinsData);
}


FTC_STATUS WINAPI I2C_InitDevice(FTC_HANDLE ftHandle, DWORD dwClockDivisor)
{
  return I2C_InitDevice_ssel(ftHandle, dwClockDivisor);
}


FTC_STATUS WINAPI I2C_TurnOnDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  return I2C_TurnOnDivideByFiveClockingHiSpeedDevice_ssel(ftHandle);
}


FTC_STATUS WINAPI I2C_TurnOffDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  return I2C_TurnOffDivideByFiveClockingHiSpeedDevice_ssel(ftHandle);
}


FTC_STATUS WINAPI I2C_TurnOnThreePhaseDataClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  return I2C_TurnOnThreePhaseDataClockingHiSpeedDevice_ssel(ftHandle);
}


FTC_STATUS WINAPI I2C_TurnOffThreePhaseDataClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  return I2C_TurnOffThreePhaseDataClockingHiSpeedDevice_ssel(ftHandle);
}


FTC_STATUS WINAPI I2C_SetDeviceLatencyTimer(FTC_HANDLE ftHandle, BYTE timerValue)
{
  return I2C_SetDeviceLatencyTimer_ssel(ftHandle, timerValue);
}


FTC_STATUS WINAPI I2C_GetDeviceLatencyTimer(FTC_HANDLE ftHandle, LPBYTE lpTimerValue)
{
  return I2C_GetDeviceLatencyTimer_ssel(ftHandle, lpTimerValue);
}


FTC_STATUS WINAPI I2C_GetClock(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  return I2C_GetClock_ssel(dwClockDivisor, lpdwClockFrequencyHz);
}


FTC_STATUS WINAPI I2C_GetHiSpeedDeviceClock(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  return I2C_GetHiSpeedDeviceClock_ssel(dwClockDivisor, lpdwClockFrequencyHz);
}


FTC_STATUS WINAPI I2C_SetClock(FTC_HANDLE ftHandle, DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  return I2C_SetClock_ssel(ftHandle, dwClockDivisor, lpdwClockFrequencyHz);
}


FTC_STATUS WINAPI I2C_SetLoopback(FTC_HANDLE ftHandle, BOOL bLoopbackState)
{
  return I2C_SetDeviceLoopbackState(ftHandle, bLoopbackState);
}


FTC_STATUS WINAPI I2C_SetMode(FTC_HANDLE ftHandle, DWORD dwCommsMode)
{
  return I2C_SetCommunicationsMode(ftHandle, dwCommsMode);
}


FTC_STATUS WINAPI I2C_SetGPIOs(FTC_HANDLE ftHandle, PFTC_INPUT_OUTPUT_PINS pHighInputOutputPinsData)
{
  return I2C_SetGeneralPurposeHighInputOutputPins(ftHandle, pHighInputOutputPinsData);
}


FTC_STATUS WINAPI I2C_SetHiSpeedDeviceGPIOs(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                            PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                            BOOL bControlHighInputOutputPins,
                                            PFTH_INPUT_OUTPUT_PINS pHighInputOutputPinsData)
{
  return I2C_SetHiSpeedDeviceGeneralPurposeInputOutputPins(ftHandle, bControlLowInputOutputPins,
                                                                             pLowInputOutputPinsData, bControlHighInputOutputPins,
                                                                             pHighInputOutputPinsData);
}


FTC_STATUS WINAPI I2C_GetGPIOs(FTC_HANDLE ftHandle, PFTC_LOW_HIGH_PINS pHighPinsInputData)
{
  return I2C_GetGeneralPurposeHighInputOutputPins(ftHandle, pHighPinsInputData);
}


FTC_STATUS WINAPI I2C_GetHiSpeedDeviceGPIOs(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                            PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                            BOOL bControlHighInputOutputPins,
                                            PFTH_LOW_HIGH_PINS pHighPinsInputData)
{
  return I2C_GetHiSpeedDeviceGeneralPurposeInputOutputPins(ftHandle, bControlLowInputOutputPins,
                                                                             pLowPinsInputData, bControlHighInputOutputPins,
                                                                             pHighPinsInputData);
}


FTC_STATUS WINAPI I2C_Write(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                            DWORD dwNumControlBytesToWrite, BOOL bControlAcknowledge, DWORD dwControlAckTimeoutmSecs,
                            BOOL bStopCondition, DWORD dwDataWriteTypes, PWriteDataByteBuffer pWriteDataBuffer, DWORD dwNumDataBytesToWrite,
                            BOOL bDataAcknowledge, DWORD dwDataAckTimeoutmSecs, PFTC_PAGE_WRITE_DATA pPageWriteData)
{
  return I2C_WriteDataToExternalDevice(ftHandle, pWriteControlBuffer, dwNumControlBytesToWrite,
                                                         bControlAcknowledge, dwControlAckTimeoutmSecs, bStopCondition,
                                                         dwDataWriteTypes, pWriteDataBuffer, dwNumDataBytesToWrite,
                                                         bDataAcknowledge, dwDataAckTimeoutmSecs, pPageWriteData);
}


FTC_STATUS WINAPI I2C_Read(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                           DWORD dwNumControlBytesToWrite, BOOL bControlAcknowledge, DWORD dwControlAckTimeoutmSecs,
                           DWORD dwDataReadTypes, PReadDataByteBuffer pReadDataBuffer, DWORD dwNumDataBytesToRead)
{
  return I2C_ReadDataFromExternalDevice(ftHandle, pWriteControlBuffer, dwNumControlBytesToWrite,
                                                          bControlAcknowledge, dwControlAckTimeoutmSecs, dwDataReadTypes,
                                                          pReadDataBuffer, dwNumDataBytesToRead);
}


FTC_STATUS WINAPI I2C_GetErrorCodeString(LPSTR lpLanguage, FTC_STATUS StatusCode, LPSTR lpErrorMessageBuffer, DWORD dwBufferSize)
{
  return I2C_GetErrorCodeString_ssel(lpLanguage, StatusCode, lpErrorMessageBuffer, dwBufferSize);
}
//---------------------------------------------------------------------------

