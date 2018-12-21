/*++

Copyright (c) 2005  Future Technology Devices International Ltd.

Module Name:

    FT2232hMpsseI2c.cpp

Abstract:

    FT2232H and FT4232H Hi-Speed Dual Device and FT2232D Dual Device Class Declaration/Definition.

Environment:

    kernel & user mode

Revision History:

    23/03/05    kra     Created.
    03/06/08    kra     Version 1.3, added 3 new functions ie SetGeneralPurposeHighInputOutputPins, GetGeneralPurposeHighInputOutputPinsInputStates,
                        and GetGeneralPurposeHighInputOutputPins to control the output states ie high(1) or low(0)
                        of the 4 general purpose higher input/output ACBUS 0-3/GPIOH 0-3) pins
    01/08/08    kra     Renamed FT2232cMpsseI2c.cpp to FT2232cMpsseI2c.cpp for FT2232H and FT4232H hi-speed devices
    01/08/08    kra     Added new functions for FT2232H and FT4232H hi-speed devices.
    20/08/08    kra     Added new function SetTCKTDITMSPinsCloseState.
    03/09/08    kra     Added critical sections to every public method, to ensure that only one public method in the
                        DLL will be executed at a time, when a process/application has multiple threads running.

--*/

#define WIO_DEFINED

#include "stdafx.h"

//#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "FT2232hMpsseI2c.h"
#include <sys/time.h>
#include <pthread.h>

void begin (void) __attribute__((constructor));
void end (void) __attribute__((destructor));
pthread_mutex_t m;

DWORD dwDeviceIndex = 0;
DWORD dwNumOpenedDevices = 0;
FTC_I2C_DEVICE_DATA OpenedDevicesDataRecords[MAX_NUM_DEVICES] = {0};
DWORD dwSavedLowPinsValue = 0;
const   BYTE ACKNOWLEDGE_BIT = '\x01';
const char EN_Common_Errors[FTC_INSUFFICIENT_RESOURCES + 1][MAX_ERROR_MSG_SIZE] = {
    "",
    "Invalid device handle.",
    "Device not found.",
    "Device not opened.",
    "General device IO error.",
    "Insufficient resources available to execute function."};

const char EN_New_Errors[(FTC_INVALID_STATUS_CODE - FTC_FAILED_TO_COMPLETE_COMMAND) + 1][MAX_ERROR_MSG_SIZE] = {
    "Failed to complete command.",
    "Failed to synchronize the device MPSSE interface.",
    "Invalid device name index.",
    "Pointer to device name buffer is null.",
    "Buffer to contain device name is too small.",
    "Invalid device name.",
    "Invalid device location identifier.",
    "Device already in use by another application.",
    "More than one device detected.",

    "Pointer to channel buffer is null.",
    "Buffer to contain channel is too small.",
    "Invalid device channel. Valid values are A and B.",
    "Invalid latency timer value. Valid range is 2 - 255 milliseconds.",

    "External Device not found.",
    "Invalid clock divisor. Valid range is 0 - 65535.",
    "Pointer to input buffer is null.",
    "Pointer to input output buffer is null.",
    "Pointer to control data buffer is null.",
    "Invalid number of control bytes. Valid range is 1 - 255.",
    "Timeout expired waiting for acknowledgement, after control byte written to external device.",
    "Pointer to write data buffer is null.",
    "Invalid number of data bytes to be written. Valid range is 1 - 65535",
    "Timeout expired waiting for acknowledgement, after data byte written to external device.",
    "Invalid type of data write. Valid types are NONE, BYTE and PAGE.",
    "Not enough data bytes in write data buffer to perform specified page write.",
    "Pointer to page write buffer is null.",
    "Pointer to read data buffer is null.",
    "Invalid number of data bytes to be read. Valid range is 1 - 65535",
    "Invalid type of data read. Valid types are BYTE and BLOCK.",
    "Invalid communications mode. Valid modes are STANDARD, FAST and STRETCH DATA.",
    "Pointer to final state buffer is null.",
    "Pointer to dll version number buffer is null.",
    "Buffer to contain dll version number is too small.",
    "Pointer to language code buffer is null.",
    "Pointer to error message buffer is null.",
    "Buffer to contain error message is too small.",
    "Unsupported language code.",
    "Unknown status code = "};

FTC_STATUS CheckWriteDataToExternalDeviceParameters(PWriteControlByteBuffer pWriteControlBuffer, DWORD dwNumControlBytesToWrite,
                                                                     DWORD dwDataWriteTypes, PWriteDataByteBuffer pWriteDataBuffer, 
                                                                     DWORD dwNumDataBytesToWrite, PFTC_PAGE_WRITE_DATA pPageWriteData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumDataBytesToBeWritten = 0;

  if (pWriteControlBuffer != NULL)
  {
    if ((dwNumControlBytesToWrite >= MIN_NUM_CONTROL_BYTES) && (dwNumControlBytesToWrite <= MAX_NUM_CONTROL_BYTES))
    {
      if ((dwDataWriteTypes >= NO_WRITE_TYPE) && (dwDataWriteTypes <= PAGE_WRITE_TYPE))
      {
        if ((dwDataWriteTypes == BYTE_WRITE_TYPE) || (dwDataWriteTypes == PAGE_WRITE_TYPE))
        {
          if (pWriteDataBuffer != NULL)
          {
            if ((dwNumDataBytesToWrite >= MIN_NUM_WRITE_DATA_BYTES) && (dwNumDataBytesToWrite <= MAX_NUM_WRITE_DATA_BYTES))
            {
              if (dwDataWriteTypes == PAGE_WRITE_TYPE)
              {
                if (pPageWriteData == NULL)
                  Status = FTC_NULL_PAGE_WRITE_BUFFER_POINTER;
                else
                {
                  dwNumDataBytesToBeWritten = (pPageWriteData->dwNumPages * pPageWriteData->dwNumBytesPerPage);

                  if (dwNumDataBytesToWrite < dwNumDataBytesToBeWritten)
                    Status = FTC_NUMBER_BYTES_TOO_SMALL_PAGE_WRITE;
                }
              }
            }
            else
              Status = FTC_INVALID_NUMBER_DATA_BYTES_WRITE;
          }
          else
            Status = FTC_NULL_WRITE_DATA_BUFFER_POINTER;
        }
      }
      else
        Status = FTC_INVALID_WRITE_TYPE;
    }
    else
      Status = FTC_INVALID_NUMBER_CONTROL_BYTES;
  }
  else
    Status = FTC_NULL_CONTROL_DATA_BUFFER_POINTER;

  return Status;
}

FTC_STATUS CheckReadDataFromExternalDeviceParameters(PWriteControlByteBuffer pWriteControlBuffer, DWORD dwNumControlBytesToWrite,
                                                                      DWORD dwDataReadTypes, PReadDataByteBuffer pReadDataBuffer,
                                                                      DWORD dwNumDataBytesToRead)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((pWriteControlBuffer != NULL) && (pReadDataBuffer != NULL))
  {
    if ((dwNumControlBytesToWrite >= MIN_NUM_CONTROL_BYTES) && (dwNumControlBytesToWrite <= MAX_NUM_CONTROL_BYTES))
    {
      if ((dwNumDataBytesToRead >= MIN_NUM_READ_DATA_BYTES) && (dwNumDataBytesToRead <= MAX_NUM_READ_DATA_BYTES))
      {
        if ((dwDataReadTypes != BYTE_READ_TYPE) && (dwDataReadTypes != BLOCK_READ_TYPE))
          Status = FTC_INVALID_READ_TYPE;
      }
      else
        Status = FTC_INVALID_NUMBER_DATA_BYTES_READ;
    }
    else
      Status = FTC_INVALID_NUMBER_CONTROL_BYTES;
  }
  else
  {
    if (pWriteControlBuffer == NULL)
      Status = FTC_NULL_CONTROL_DATA_BUFFER_POINTER;
    else
      Status = FTC_NULL_READ_DATA_BUFFER_POINTER;
  }

  return Status;
}

FTC_STATUS SetTCKTDITMSPinsCloseState(FTC_HANDLE ftHandle, PFTC_CLOSE_FINAL_STATE_PINS pCloseFinalStatePinsData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((pCloseFinalStatePinsData->bTCKPinState != FALSE) ||  
      (pCloseFinalStatePinsData->bTDIPinState != FALSE) ||
      (pCloseFinalStatePinsData->bTMSPinState != FALSE)) {
    FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 1);

    if (pCloseFinalStatePinsData->bTCKPinState != FALSE) {
      if (pCloseFinalStatePinsData->bTCKPinActiveState != FALSE)
        dwSavedLowPinsValue = (dwSavedLowPinsValue | 0x01); // Set TCK pin high
      else
        dwSavedLowPinsValue = (dwSavedLowPinsValue & 0xFE); // Set TCK pin low

      dwSavedLowPinsDirection = (dwSavedLowPinsDirection | 0x01); // Ensure TCK pin is set to output
    }

    if (pCloseFinalStatePinsData->bTDIPinState != FALSE) {
      if (pCloseFinalStatePinsData->bTDIPinActiveState != FALSE)
        dwSavedLowPinsValue = (dwSavedLowPinsValue | 0x02); // Set TDI pin high
      else
        dwSavedLowPinsValue = (dwSavedLowPinsValue & 0xFD); // Set TDI pin low

      dwSavedLowPinsDirection = (dwSavedLowPinsDirection | 0x02); // Ensure TDI pin is set to output
    }

    if (pCloseFinalStatePinsData->bTMSPinState != FALSE) {
      if (pCloseFinalStatePinsData->bTMSPinActiveState != FALSE)
        dwSavedLowPinsValue = (dwSavedLowPinsValue | 0x08); // Set TMS pin high
      else
        dwSavedLowPinsValue = (dwSavedLowPinsValue & 0xF7); // Set TMS pin low

      dwSavedLowPinsDirection = (dwSavedLowPinsDirection | 0x08); // Ensure TMS pin is set to output
    }
    
    FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);
    FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0);

    Status = FTC_SendBytesToDevice(ftHandle);
  }

  return Status;
}

FTC_STATUS InitDevice(FTC_HANDLE ftHandle, DWORD dwClockDivisor)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((dwClockDivisor >= MIN_CLOCK_DIVISOR) && (dwClockDivisor <= MAX_CLOCK_DIVISOR))
  {
      Status = FTC_ResetUSBDevicePurgeUSBInputBuffer(ftHandle);
      if (Status == FTC_SUCCESS)
        Status = FTC_SetDeviceUSBBufferSizes(ftHandle, USB_INPUT_BUFFER_SIZE, USB_OUTPUT_BUFFER_SIZE);
      if (Status == FTC_SUCCESS)
        Status = FTC_SetDeviceSpecialCharacters(ftHandle, 0, FT_EVENT_VALUE, 0, FT_ERROR_VALUE);
      if (Status == FTC_SUCCESS)
        Status = FTC_SetReadWriteDeviceTimeouts(ftHandle, DEVICE_READ_TIMEOUT_INFINITE, DEVICE_WRITE_TIMEOUT);
      if (Status == FTC_SUCCESS)
        Status = FTC_SetDeviceLatencyTimer(ftHandle, DEVICE_LATENCY_TIMER_VALUE);
      if (Status == FTC_SUCCESS)
        Status = FTC_ResetMPSSEInterface(ftHandle);
      if (Status == FTC_SUCCESS)
        Status = FTC_EnableMPSSEInterface(ftHandle);
      if (Status == FTC_SUCCESS)
        Status = FTC_SynchronizeMPSSEInterface(ftHandle);
      //if (Status == FTC_SUCCESS)
      //  Status = FTC_ResetUSBDevicePurgeUSBInputBuffer(ftHandle);

      if (Status == FTC_SUCCESS)
        usleep(50000); // wait for all the USB stuff to complete
      if (Status == FTC_SUCCESS)
        Status = InitDataInOutClockFrequency(ftHandle, dwClockDivisor);
      if (Status == FTC_SUCCESS)
        usleep(20000);
      if (Status == FTC_SUCCESS)
        Status = FTC_SetDeviceLoopbackState(ftHandle, 0);
      if (Status == FTC_SUCCESS)
        Status = EmptyDeviceInputBuffer(ftHandle);
      if (Status == FTC_SUCCESS)
        usleep(30000);
  }
  else
    Status = FTC_INVALID_CLOCK_DIVISOR;
  return Status;
}

FTC_STATUS SetDataInOutClockFrequency(FTC_HANDLE ftHandle, DWORD dwClockDivisor)
{
  FTC_STATUS Status = FTC_SUCCESS;
  
  // set clk divisor
  FTC_AddByteToOutputBuffer(SET_CLOCK_FREQUENCY_CMD, 1);
  FTC_AddByteToOutputBuffer((dwClockDivisor & 0xFF), 0);
  FTC_AddByteToOutputBuffer((dwClockDivisor >> 8), 0);

  Status = FTC_SendBytesToDevice(ftHandle);

  //sleep(0); // give up timeslice

  return Status;
}

FTC_STATUS InitDataInOutClockFrequency(FTC_HANDLE ftHandle, DWORD dwClockDivisor)
{
  FTC_STATUS Status = FTC_SUCCESS;

  // set SK,DO,CS as out
  FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 1);

  // SDA SCL WP high
  dwSavedLowPinsValue = 0x13;
  FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

  // inputs on GPIO12-14
  FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

  // outputs on GPIO21-24
  FTC_AddByteToOutputBuffer(SET_HIGH_BYTE_DATA_BITS_CMD, 0);
  FTC_AddByteToOutputBuffer(0x0F, 0);
  FTC_AddByteToOutputBuffer(0x0F, 0);

  Status = FTC_SendBytesToDevice(ftHandle);
  
  if (Status == FTC_SUCCESS)
  {
    //sleep(0); // give up timeslice

    SetDataInOutClockFrequency(ftHandle, dwClockDivisor);
  }

  return Status;
}

FTC_STATUS EmptyDeviceInputBuffer(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumBytesDeviceInputBuffer = 0;
  DWORD dwNumBytesRead = 0;

  // Get the number of bytes in the device input buffer
  Status = (FTC_STATUS)FT_GetQueueStatus((FT_HANDLE)ftHandle, &dwNumBytesDeviceInputBuffer);

  if ((Status == FTC_SUCCESS) && (dwNumBytesDeviceInputBuffer > 0))
     FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead);

  return Status;
}

FTC_STATUS SetGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, DWORD dwHighPinsDirection, DWORD dwHighPinsValue)
{
  FTC_STATUS Status = FTC_SUCCESS;

  // output on the general purpose I/O high pins 1-4
  FTC_AddByteToOutputBuffer(SET_HIGH_BYTE_DATA_BITS_CMD, 1);

  dwHighPinsValue = (dwHighPinsValue & 0x0F);
  FTC_AddByteToOutputBuffer(dwHighPinsValue, 0);

  dwHighPinsDirection = (dwHighPinsDirection & 0x0F);
  FTC_AddByteToOutputBuffer(dwHighPinsDirection, 0);

  Status = FTC_SendBytesToDevice(ftHandle);

  if (Status == FTC_SUCCESS) {
    dwSavedHighPinsDirection = dwHighPinsDirection;
    dwSavedHighPinsValue = dwHighPinsValue;
  }

  return Status;
}

FTC_STATUS SetHiSpeedDeviceGeneralPurposeLowInputOutputPins(FTC_HANDLE ftHandle, PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData)
{
  DWORD dwLowPinsDirection = 0;
  DWORD dwLowPinsValue = 0;

  // Only the 3 general purpose lower input/output pins (GPIOL2 – GPIOL4) are available, the GPIOL1 pin
  // cannot be used, it is reserved for I2C write protect and therefore is configured as an output
  dwLowPinsDirection = (dwLowPinsDirection | 0x01); // Set Write Protect pin(GPIOL1) high ie output
  if (pLowInputOutputPinsData->bPin2InputOutputState != FALSE)
    dwLowPinsDirection = (dwLowPinsDirection | 0x02);
  if (pLowInputOutputPinsData->bPin3InputOutputState != FALSE)
    dwLowPinsDirection = (dwLowPinsDirection | 0x04);
  if (pLowInputOutputPinsData->bPin4InputOutputState != FALSE)
    dwLowPinsDirection = (dwLowPinsDirection | 0x08);

  dwLowPinsValue = (dwLowPinsValue | 0x01); // Set Write Protect pin(GPIOL1) high ie enable write protect (default)
  if (pLowInputOutputPinsData->bPin2LowHighState != FALSE)
    dwLowPinsValue = (dwLowPinsValue | 0x02);
  if (pLowInputOutputPinsData->bPin3LowHighState != FALSE)
    dwLowPinsValue = (dwLowPinsValue | 0x04);
  if (pLowInputOutputPinsData->bPin4LowHighState != FALSE)
    dwLowPinsValue = (dwLowPinsValue | 0x08);

  // output on the general purpose I/O low pins 1-4
  FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, TRUE);

  // shift left by 4 bits ie move general purpose I/O low pins 1-4 from bits 0-3 to bits 4-7
  dwLowPinsValue = ((dwLowPinsValue & 0x0F) << 4);

  dwSavedLowPinsValue = (dwSavedLowPinsValue & 0x0F);
  dwSavedLowPinsValue = (dwSavedLowPinsValue | dwLowPinsValue);
  FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, FALSE);

  // shift left by 4 bits ie move general purpose I/O low pins 1-4 from bits 0-3 to bits 4-7
  dwLowPinsDirection = ((dwLowPinsDirection & 0x0F) << 4);

  dwSavedLowPinsDirection = (dwSavedLowPinsDirection & 0x0F);
  dwSavedLowPinsDirection = (dwSavedLowPinsDirection | dwLowPinsDirection); 
  FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, FALSE);

  return FTC_SendBytesToDevice(ftHandle);
}

FTC_STATUS SetHiSpeedDeviceGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, PFTH_INPUT_OUTPUT_PINS pHighInputOutputPinsData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  BOOL bHiSpeedFT2232HTDeviceype = FALSE;
  DWORD dwHighPinsDirection = 0;
  DWORD dwHighPinsValue = 0;

  if ((Status = FTC_IsDeviceHiSpeedFT2232HType(ftHandle, &bHiSpeedFT2232HTDeviceype)) == FTC_SUCCESS)
  {
    // If the device is a FT2232H hi-speed device
    if (bHiSpeedFT2232HTDeviceype == TRUE)
    {
	  if (pHighInputOutputPinsData->bPin1InputOutputState != FALSE)
        dwHighPinsDirection = (dwHighPinsDirection | 0x01);
      if (pHighInputOutputPinsData->bPin2InputOutputState != FALSE)
        dwHighPinsDirection = (dwHighPinsDirection | 0x02);
      if (pHighInputOutputPinsData->bPin3InputOutputState != FALSE)
        dwHighPinsDirection = (dwHighPinsDirection | 0x04);
      if (pHighInputOutputPinsData->bPin4InputOutputState != FALSE)
        dwHighPinsDirection = (dwHighPinsDirection | 0x08);
      if (pHighInputOutputPinsData->bPin5InputOutputState != FALSE)
        dwHighPinsDirection = (dwHighPinsDirection | 0x10);
      if (pHighInputOutputPinsData->bPin6InputOutputState != FALSE)
        dwHighPinsDirection = (dwHighPinsDirection | 0x20);
      if (pHighInputOutputPinsData->bPin7InputOutputState != FALSE)
        dwHighPinsDirection = (dwHighPinsDirection | 0x40);
      if (pHighInputOutputPinsData->bPin8InputOutputState != FALSE)
        dwHighPinsDirection = (dwHighPinsDirection | 0x80);

      if (pHighInputOutputPinsData->bPin1LowHighState != FALSE)
        dwHighPinsValue = (dwHighPinsValue | 0x01);
      if (pHighInputOutputPinsData->bPin2LowHighState != FALSE)
        dwHighPinsValue = (dwHighPinsValue | 0x02);
      if (pHighInputOutputPinsData->bPin3LowHighState != FALSE)
        dwHighPinsValue = (dwHighPinsValue | 0x04);
      if (pHighInputOutputPinsData->bPin4LowHighState != FALSE)
        dwHighPinsValue = (dwHighPinsValue | 0x08);
      if (pHighInputOutputPinsData->bPin5LowHighState != FALSE)
        dwHighPinsValue = (dwHighPinsValue | 0x10);
      if (pHighInputOutputPinsData->bPin6LowHighState != FALSE)
        dwHighPinsValue = (dwHighPinsValue | 0x20);
      if (pHighInputOutputPinsData->bPin7LowHighState != FALSE)
        dwHighPinsValue = (dwHighPinsValue | 0x40 );
      if (pHighInputOutputPinsData->bPin8LowHighState != FALSE)
        dwHighPinsValue = (dwHighPinsValue | 0x80);

      // output on the general purpose I/O high pins 1-8
      FTC_AddByteToOutputBuffer(SET_HIGH_BYTE_DATA_BITS_CMD, TRUE);

      dwHighPinsValue = (dwHighPinsValue & 0xFF);
      FTC_AddByteToOutputBuffer(dwHighPinsValue, FALSE);

      dwHighPinsDirection = (dwHighPinsDirection & 0XFF);
      FTC_AddByteToOutputBuffer(dwHighPinsDirection, FALSE);

      Status = FTC_SendBytesToDevice(ftHandle);

    }
  }

  return Status;
}

void GetGeneralPurposeHighInputOutputPinsInputStates(DWORD dwInputStatesReturnedValue, PFTC_LOW_HIGH_PINS pHighPinsInputData)
{
  if ((dwInputStatesReturnedValue & PIN1_HIGH_VALUE) == PIN1_HIGH_VALUE)
    pHighPinsInputData->bPin1LowHighState = 1;

  if ((dwInputStatesReturnedValue & PIN2_HIGH_VALUE) == PIN2_HIGH_VALUE)
    pHighPinsInputData->bPin2LowHighState = 1;

  if ((dwInputStatesReturnedValue & PIN3_HIGH_VALUE) == PIN3_HIGH_VALUE)
    pHighPinsInputData->bPin3LowHighState = 1;

  if ((dwInputStatesReturnedValue & PIN4_HIGH_VALUE) == PIN4_HIGH_VALUE)
    pHighPinsInputData->bPin4LowHighState = 1;
}

FTC_STATUS GetGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, PFTC_LOW_HIGH_PINS pHighPinsInputData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  InputByteBuffer InputBuffer;
  DWORD dwNumBytesRead = 0;
  DWORD dwNumBytesDeviceInputBuffer;

  pHighPinsInputData->bPin1LowHighState = 0;
  pHighPinsInputData->bPin2LowHighState = 0;
  pHighPinsInputData->bPin3LowHighState = 0;
  pHighPinsInputData->bPin4LowHighState = 0;

  // Put in this small delay incase the application programmer does a get GPIOs immediately after a set GPIOs
  usleep(5000);

  // Get the number of bytes in the device input buffer
  Status = FT_GetQueueStatus((FT_HANDLE)ftHandle, &dwNumBytesDeviceInputBuffer);

  if (Status == FTC_SUCCESS)
  {
    if (dwNumBytesDeviceInputBuffer > 0)
      Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead);

    if (Status == FTC_SUCCESS)
    {
      // get the states of the general purpose I/O high pins 1-4
      FTC_AddByteToOutputBuffer(GET_HIGH_BYTE_DATA_BITS_CMD, 1);
      FTC_AddByteToOutputBuffer(SEND_ANSWER_BACK_IMMEDIATELY_CMD, 0);
      Status = FTC_SendBytesToDevice(ftHandle);

      if (Status == FTC_SUCCESS)
      {
        Status = FTC_GetNumberBytesFromDeviceInputBuffer(ftHandle, &dwNumBytesDeviceInputBuffer);

        if (Status == FTC_SUCCESS)
        {
          Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead);

          if (Status == FTC_SUCCESS)
            GetGeneralPurposeHighInputOutputPinsInputStates(InputBuffer[0], pHighPinsInputData);
        }
      }
    }
  }

  return Status;
}

void GetHiSpeedDeviceGeneralPurposeLowInputOutputPinsInputStates(DWORD dwInputStatesReturnedValue, PFTC_LOW_HIGH_PINS pLowPinsInputData)
{
  // Only the 3 general purpose lower input/output pins (GPIOL2 – GPIOL4) are available, the GPIOL1 pin
  // cannot be used, it is reserved for I2C write protect and therefore is configured as an output

  if ((dwInputStatesReturnedValue & PIN2_HIGH_VALUE) == PIN2_HIGH_VALUE)
    pLowPinsInputData->bPin2LowHighState = 1;

  if ((dwInputStatesReturnedValue & PIN3_HIGH_VALUE) == PIN3_HIGH_VALUE)
    pLowPinsInputData->bPin3LowHighState = 1;

  if ((dwInputStatesReturnedValue & PIN4_HIGH_VALUE) == PIN4_HIGH_VALUE)
    pLowPinsInputData->bPin4LowHighState = 1;
}

FTC_STATUS GetHiSpeedDeviceGeneralPurposeLowInputOutputPins(FTC_HANDLE ftHandle, PFTC_LOW_HIGH_PINS pLowPinsInputData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  InputByteBuffer InputBuffer;
  DWORD dwNumBytesRead = 0;
  DWORD dwNumBytesDeviceInputBuffer;

  pLowPinsInputData->bPin1LowHighState = FALSE;
  pLowPinsInputData->bPin2LowHighState = FALSE;
  pLowPinsInputData->bPin3LowHighState = FALSE;
  pLowPinsInputData->bPin4LowHighState = FALSE;

  // Get the number of bytes in the device input buffer
  if ((Status = FT_GetQueueStatus((FT_HANDLE)ftHandle, &dwNumBytesDeviceInputBuffer)) == FTC_SUCCESS)
  {
    if (dwNumBytesDeviceInputBuffer > 0)
      Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead);

    if (Status == FTC_SUCCESS)
    {
      // get the states of the general purpose I/O low pins 1-4
      FTC_AddByteToOutputBuffer(GET_LOW_BYTE_DATA_BITS_CMD, TRUE);
      FTC_AddByteToOutputBuffer(SEND_ANSWER_BACK_IMMEDIATELY_CMD, FALSE);
      Status = FTC_SendBytesToDevice(ftHandle);

      if (Status == FTC_SUCCESS)
      {
        if ((Status = FTC_GetNumberBytesFromDeviceInputBuffer(ftHandle, &dwNumBytesDeviceInputBuffer)) == FTC_SUCCESS)
        {
          if ((Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead)) == FTC_SUCCESS)
            // shift right by 4 bits ie move general purpose I/O low pins 1-4 from bits 4-7 to bits 0-3
            GetHiSpeedDeviceGeneralPurposeLowInputOutputPinsInputStates((InputBuffer[0] >> 4), pLowPinsInputData);
        }
      }
    }
  }

  return Status;
}

void  GetHiSpeedDeviceGeneralPurposeHighInputOutputPinsInputStates(DWORD dwInputStatesReturnedValue, PFTH_LOW_HIGH_PINS pPinsInputData)
{
  if ((dwInputStatesReturnedValue & PIN1_HIGH_VALUE) == PIN1_HIGH_VALUE)
    pPinsInputData->bPin1LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN2_HIGH_VALUE) == PIN2_HIGH_VALUE)
    pPinsInputData->bPin2LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN3_HIGH_VALUE) == PIN3_HIGH_VALUE)
    pPinsInputData->bPin3LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN4_HIGH_VALUE) == PIN4_HIGH_VALUE)
    pPinsInputData->bPin4LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN5_HIGH_VALUE) == PIN5_HIGH_VALUE)
    pPinsInputData->bPin5LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN6_HIGH_VALUE) == PIN6_HIGH_VALUE)
    pPinsInputData->bPin6LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN7_HIGH_VALUE) == PIN7_HIGH_VALUE)
    pPinsInputData->bPin7LowHighState = TRUE;

  if ((dwInputStatesReturnedValue & PIN8_HIGH_VALUE) == PIN8_HIGH_VALUE)
    pPinsInputData->bPin8LowHighState = TRUE;
}

FTC_STATUS GetHiSpeedDeviceGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, PFTH_LOW_HIGH_PINS pHighPinsInputData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  InputByteBuffer InputBuffer;
  DWORD dwNumBytesRead = 0;
  DWORD dwNumBytesDeviceInputBuffer;
  BOOL bHiSpeedFT2232HTDeviceype = FALSE;

  pHighPinsInputData->bPin1LowHighState = FALSE;
  pHighPinsInputData->bPin2LowHighState = FALSE;
  pHighPinsInputData->bPin3LowHighState = FALSE;
  pHighPinsInputData->bPin4LowHighState = FALSE;
  pHighPinsInputData->bPin5LowHighState = FALSE;
  pHighPinsInputData->bPin6LowHighState = FALSE;
  pHighPinsInputData->bPin7LowHighState = FALSE;
  pHighPinsInputData->bPin8LowHighState = FALSE;

  // Put in this small delay incase the application programmer does a get GPIOs immediately after a set GPIOs
  usleep(5000);

  if ((Status = FTC_IsDeviceHiSpeedFT2232HType(ftHandle, &bHiSpeedFT2232HTDeviceype)) == FTC_SUCCESS)
  {
    // If the device is a FT2232H hi-speed device
    if (bHiSpeedFT2232HTDeviceype == TRUE)
    {
      // Get the number of bytes in the device input buffer
      if ((Status = FT_GetQueueStatus((FT_HANDLE)ftHandle, &dwNumBytesDeviceInputBuffer)) == FTC_SUCCESS)
      {
        if (dwNumBytesDeviceInputBuffer > 0)
          Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead);

        if (Status == FTC_SUCCESS)
        {
          // get the states of the general purpose I/O high pins 1-4
          FTC_AddByteToOutputBuffer(GET_HIGH_BYTE_DATA_BITS_CMD, TRUE);
          FTC_AddByteToOutputBuffer(SEND_ANSWER_BACK_IMMEDIATELY_CMD, FALSE);
          Status = FTC_SendBytesToDevice(ftHandle);

          if (Status == FTC_SUCCESS)
          {
            if ((Status = FTC_GetNumberBytesFromDeviceInputBuffer(ftHandle, &dwNumBytesDeviceInputBuffer)) == FTC_SUCCESS)
            {
              if ((Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead)) == FTC_SUCCESS)
                GetHiSpeedDeviceGeneralPurposeHighInputOutputPinsInputStates(InputBuffer[0], pHighPinsInputData);
            }
          }
        }
      }
    }
  }

  return Status;
}

void  SetI2CStartCondition(FTC_HANDLE ftHandle, BOOL bEnableDisableWriteProtect)
{
  DWORD dwLoopCntr = 0;
  BOOL bHiSpeedTypeDevice = FALSE;

  if (bEnableDisableWriteProtect == 0)
    dwSavedLowPinsValue = (dwSavedLowPinsValue & 0xEF); // Set Write Protect pin low ie disable write protect
  else
    dwSavedLowPinsValue = (dwSavedLowPinsValue | 0x10); // Set Write Protect pin high ie enable write protect

  FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
  dwSavedLowPinsValue = (dwSavedLowPinsValue | 0x03); //SCL SDA high
  FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

  dwSavedLowPinsDirection = (dwSavedLowPinsDirection & 0xE0);
  dwSavedLowPinsDirection = (dwSavedLowPinsDirection | 0x13); // set SCL,SDA,WP as out

  FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA,WP as out

  if (FTC_IsDeviceHiSpeedType3(ftHandle) == TRUE) {
    // For hi-speed device ie FT2232H or FT4232H, ensure the minimum period of the start hold time ie 600ns is achieved
    for (dwLoopCntr = 0; dwLoopCntr < START_HI_SPEED_DEVICE_HOLD_SETUP_TIME; dwLoopCntr++)
    {
      FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
      FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);
      FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA,WP as out
    }
  }

  FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
  dwSavedLowPinsValue = (dwSavedLowPinsValue & 0xFD); //SCL high SDA low
  FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

  FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA,WP as out

  if (FTC_IsDeviceHiSpeedType3(ftHandle) == TRUE) {
    // For hi-speed device ie FT2232H or FT4232H, ensure the minimum period of the start setup time ie 600ns is achieved
    for (dwLoopCntr = 0; dwLoopCntr < START_HI_SPEED_DEVICE_HOLD_SETUP_TIME; dwLoopCntr++)
    {
      FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
      FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);
      FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA,WP as out
    }
  }

  if ((GetCommunicationsMode(ftHandle)&STANDARD_MODE) == STANDARD_MODE)
  {
    // For Standard mode, repeat SDA low command to increase the Start Condition Hold Time to 4 microseconds
    for (dwLoopCntr = 0; dwLoopCntr < 3; dwLoopCntr++)
    {
      FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
      dwSavedLowPinsValue = (dwSavedLowPinsValue & 0xFD); //SCL high SDA low
      FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

      FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA,WP as out
    }
  }

  FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
  dwSavedLowPinsValue = (dwSavedLowPinsValue & 0xFC); //SCL low SDA low
  FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

  FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA,WP as out
}

FTC_STATUS SetI2CStopCondition(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwLoopCntr = 0;

  FTC_ClearOutputBuffer();

  FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
  dwSavedLowPinsValue = (dwSavedLowPinsValue | 0x01); //SCL high SDA low
  dwSavedLowPinsValue = (dwSavedLowPinsValue & 0xFD); //SCL high SDA low
  FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

  dwSavedLowPinsDirection = (dwSavedLowPinsDirection & 0xE0);
  dwSavedLowPinsDirection = (dwSavedLowPinsDirection | 0x13); // set SCL,SDA,WP as out
  FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA,WP as out

  if (FTC_IsDeviceHiSpeedType3(ftHandle) == TRUE) {
    // For hi-speed device ie FT2232H or FT4232H, ensure the minimum period of the stop setup time ie 600ns is achieved
    for (dwLoopCntr = 0; dwLoopCntr < STOP_HI_SPEED_DEVICE_SETUP_TIME; dwLoopCntr++)
    {
      FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
      FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);
      FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA,WP as out
    }
  }

  if ((GetCommunicationsMode(ftHandle)&STANDARD_MODE) == STANDARD_MODE)
  {
    // For Standard mode, repeat SDA low command to increase the Stop Condition Setup Time to 4 microseconds
    for (dwLoopCntr = 0; dwLoopCntr < 3; dwLoopCntr++)
    {
      FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
      FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);  //SCL high SDA low

      FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA,WP as out
    }
  }

  FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
  dwSavedLowPinsValue = (dwSavedLowPinsValue | 0x02); //SCL high SDA high
  FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

  FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA,WP as out

  if (FTC_IsDeviceHiSpeedType3(ftHandle) == TRUE) {
    // For hi-speed device ie FT2232H or FT4232H, ensure the bus is free for a time, before a new transmission can start
    for (dwLoopCntr = 0; dwLoopCntr < STOP_HI_SPEED_DEVICE_SETUP_TIME; dwLoopCntr++)
    {
      FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
      FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);
      FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA,WP as out
    }
  }

  // tristate SDA SCL
  FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
  FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

  dwSavedLowPinsDirection = (dwSavedLowPinsDirection & 0xE0);
  dwSavedLowPinsDirection = (dwSavedLowPinsDirection | 0x10); // set SCL,SDA as input,WP as out

  FTC_AddByteToOutputBuffer(dwSavedLowPinsDirection, 0); // set SCL,SDA as input,WP as out

  Status = FTC_SendBytesToDevice(ftHandle); // send off the command

  usleep(0); // give up timeslice

  return Status;
}

void SetI2CWriteData(FTC_HANDLE ftHandle, DWORD dwNumBitsToWrite, PI2CWriteByteBuffer pI2CWriteBuffer)
{
  DWORD dwDataBufferIndex = 0;
  DWORD dwNumDataBytes = 0;
  DWORD dwNumRemainingDataBits = 0;

  dwNumDataBytes = (dwNumBitsToWrite / 8);

  if (dwNumDataBytes > 0)
  {
    // Number of whole bytes
    dwNumDataBytes = (dwNumDataBytes - 1);

	if ((GetCommunicationsMode(ftHandle)&STRETCH_DATA_MODE) == STRETCH_DATA_MODE)
	{
		//change data after clock edge
		do
		{
			ClockByte((*pI2CWriteBuffer)[dwDataBufferIndex]);
			dwDataBufferIndex = (dwDataBufferIndex + 1);
		}
		while (dwDataBufferIndex < (dwNumDataBytes + 1));
	}
	else
	{
    // clk data bytes out on -ve clk MSB first
    FTC_AddByteToOutputBuffer(CLK_DATA_BYTES_OUT_ON_NEG_CLK_MSB_FIRST_CMD, 0);
    FTC_AddByteToOutputBuffer((dwNumDataBytes & 0xFF), 0);
    FTC_AddByteToOutputBuffer(((dwNumDataBytes / 256) & 0xFF), 0);

    // now add the data bytes to go out
    do
    {
      FTC_AddByteToOutputBuffer((*pI2CWriteBuffer)[dwDataBufferIndex], 0);
      dwDataBufferIndex = (dwDataBufferIndex + 1);
    }
    while (dwDataBufferIndex < (dwNumDataBytes + 1));
	}
  }

  dwNumRemainingDataBits = (dwNumBitsToWrite % 8);

  if (dwNumRemainingDataBits > 0)
  {
    // adjust for bit count of 1 less than no of bits
    dwNumRemainingDataBits = (dwNumRemainingDataBits - 1);

    // clk data bits out on -ve clk MSB first
    FTC_AddByteToOutputBuffer(CLK_DATA_BITS_OUT_ON_NEG_CLK_MSB_FIRST_CMD, 0);
    FTC_AddByteToOutputBuffer((dwNumRemainingDataBits & 0xFF), 0);
    FTC_AddByteToOutputBuffer((*pI2CWriteBuffer)[dwDataBufferIndex], 0);
  }
}

void  SetI2CReadData(DWORD dwNumBitsToRead)
{
  DWORD dwModNumBitsToRead = 0;
  DWORD dwNumDataBytes = 0;
  DWORD dwNumRemainingDataBits = 0;

  // adjust count value
  dwModNumBitsToRead = (dwNumBitsToRead -1);

  if (dwModNumBitsToRead == 0)
  {
    // clk data bits in -ve clk MSB
    FTC_AddByteToOutputBuffer(CLK_DATA_BITS_IN_ON_NEG_CLK_MSB_FIRST_CMD, 0);
    FTC_AddByteToOutputBuffer(0, 0);
  }
  else
  {
    //dwNumDataBytes = (dwModNumBitsToRead / 8);
    dwNumDataBytes = (dwNumBitsToRead / 8);

    if (dwNumDataBytes > 0)
    {
      // Number of whole bytes
      dwNumDataBytes = (dwNumDataBytes - 1);

      // clk data bytes in -ve clk MSB
      FTC_AddByteToOutputBuffer(CLK_DATA_BYTES_IN_ON_NEG_CLK_MSB_FIRST_CMD, 0);
      FTC_AddByteToOutputBuffer((dwNumDataBytes & 0xFF), 0);
      FTC_AddByteToOutputBuffer(((dwNumDataBytes / 256) & 0xFF), 0);
    }

    //dwNumRemainingDataBits = (dwModNumBitsToRead % 8);
    dwNumRemainingDataBits = (dwNumBitsToRead % 8);

    if (dwNumRemainingDataBits > 0)
    {
      // Do remaining bits
      // clk data bits in -ve clk MSB
      FTC_AddByteToOutputBuffer(CLK_DATA_BITS_IN_ON_NEG_CLK_MSB_FIRST_CMD, 0);
      FTC_AddByteToOutputBuffer((dwNumRemainingDataBits & 0xFF), 0);
    }
  }
}

FTC_STATUS ReadDataBytesFromExternalDevice(FTC_HANDLE ftHandle, PI2CReadByteBuffer pI2CReadBuffer, DWORD dwNumBitsToRead, LPDWORD lpdwNumDataBytesRead, BOOL bWaitForAllBytesRead)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumDataBytesToRead = 0;
  DWORD dwNumRemainingDataBits = 0;
  DWORD dwNumBytesDeviceInputBuffer = 0;
  DWORD dwNumBytesRead = 0;
  DWORD dwInputBufferIndex = 0;
  DWORD dwTotalNumBytesRead = 0;
  SYSTEMTIME StartTime;

  // This will work out the number of whole bytes to read
  dwNumDataBytesToRead = (dwNumBitsToRead / 8);

  dwNumRemainingDataBits = (dwNumBitsToRead % 8);

  // get remaining bits
  if (dwNumRemainingDataBits > 0)
    dwNumDataBytesToRead = dwNumDataBytesToRead + 1; // increment the number of whole bytes to read, if bits left over

  if (bWaitForAllBytesRead == 0)
  {
    // This path is used for getting an acknowledgement from an external device after a byte has been written to it

    // Get the number of bytes in the device input buffer
    Status = (FTC_STATUS)FT_GetQueueStatus((FT_HANDLE)ftHandle, &dwNumBytesDeviceInputBuffer);

    if ((Status == FTC_SUCCESS) && (dwNumBytesDeviceInputBuffer > 0))
    {
      //sleep(0);  // give up timeslice

      Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead);

      if ((Status == FTC_SUCCESS) && (dwNumBytesRead > 0))
      {
        for (dwInputBufferIndex = 0; dwInputBufferIndex < dwNumBytesRead; dwInputBufferIndex++)
          (*pI2CReadBuffer)[dwInputBufferIndex] = InputBuffer[dwInputBufferIndex];
      }

      *lpdwNumDataBytesRead = dwNumBytesRead;
    }
  }
  else
  {
    //GetLocalTime(&StartTime);
	gettimeofday(&StartTime,NULL);

    do
    {
      // Get the number of bytes in the device input buffer
      Status = (FTC_STATUS)FT_GetQueueStatus((FT_HANDLE)ftHandle, &dwNumBytesDeviceInputBuffer);

      if ((Status == FTC_SUCCESS) && (dwNumBytesDeviceInputBuffer > 0))
      {
        Status = FTC_ReadBytesFromDevice(ftHandle, &InputBuffer, dwNumBytesDeviceInputBuffer, &dwNumBytesRead);

        if ((Status == FTC_SUCCESS) && (dwNumBytesRead > 0))
        {
          dwInputBufferIndex = 0;

          do
          {
            (*pI2CReadBuffer)[dwTotalNumBytesRead++] = InputBuffer[dwInputBufferIndex++];
          }
          while ((dwInputBufferIndex < dwNumBytesRead) && (dwTotalNumBytesRead < dwNumDataBytesToRead));
        }
      }

      if ((dwTotalNumBytesRead < dwNumDataBytesToRead) && (Status == FTC_SUCCESS))
      {
        usleep(1000);  // give up timeslice
        if (FTC_Timeout(StartTime, MAX_COMMAND_TIMEOUT_PERIOD))
          Status = FTC_FAILED_TO_COMPLETE_COMMAND;
      }
    }
    while ((dwTotalNumBytesRead < dwNumDataBytesToRead) && (Status == FTC_SUCCESS));

    *lpdwNumDataBytesRead = dwTotalNumBytesRead;
  }
  return Status;
}

FTC_STATUS ReadDataAckFromExternalDevice(FTC_HANDLE ftHandle, PI2CReadByteBuffer pI2CReadBuffer, DWORD dwNumBitsToRead, int AckType, DWORD dwAckTimeoutmSecs)
{
  FTC_STATUS Status = FTC_SUCCESS;
  SYSTEMTIME StartTime;
  DWORD dwNumDataBytesRead = 0;
  BOOL bAckReceived = 0;
  DWORD dwAckValue = 0;

  //FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
  FTC_AddByteToOutputBuffer(SET_HIGH_BYTE_DATA_BITS_CMD, 0);
  FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

  //FTC_AddByteToOutputBuffer(CLK_DATA_BYTES_OUT_ON_NEG_CLK_MSB_FIRST_CMD, 0);  // set SCL,WP as out SDA as in
  FTC_AddByteToOutputBuffer(CLK_DATA_BYTES_OUT_ON_NEG_CLK_LSB_FIRST_CMD, 0);  // set SCL,WP as out SDA as in
  
  SetI2CReadData(dwNumBitsToRead);
  FTC_AddByteToOutputBuffer(SEND_ANSWER_BACK_IMMEDIATELY_CMD, 0);  // Send immediate

  Status = FTC_SendBytesToDevice(ftHandle); // send off the command

  //sleep(0); // give up timeslice
  if (Status == FTC_SUCCESS)
  {
    if (AckType == NoAck){
      Status = ReadDataBytesFromExternalDevice(ftHandle, pI2CReadBuffer, dwNumBitsToRead, &dwNumDataBytesRead, 1);
	}
    else
    {
      //GetLocalTime(&StartTime);
	  gettimeofday(&StartTime,NULL);

      do
      {
        Status = ReadDataBytesFromExternalDevice(ftHandle, pI2CReadBuffer, dwNumBitsToRead, &dwNumDataBytesRead, 0);
        if (Status == FTC_SUCCESS)
        {
          if (dwNumDataBytesRead > 0)
          {
            dwAckValue = ((*pI2CReadBuffer)[0] & ACKNOWLEDGE_BIT);

            if (dwAckValue == ACKNOWLEDGE_VALUE)
              bAckReceived = 1;
          }

          if (bAckReceived == 0)
          {
            //Sleep(0);  // give up timeslice
            if (FTC_Timeout(StartTime, dwAckTimeoutmSecs))
            {
              if (AckType == ControlAck)
                Status = FTC_CONTROL_ACKNOWLEDGE_TIMEOUT;
              else
                Status = FTC_DATA_ACKNOWLEDGE_TIMEOUT;
            }
          }
        }
      }
      while ((bAckReceived == 0) && (Status == FTC_SUCCESS));
    }
    if (Status == FTC_SUCCESS)
    {
      FTC_ClearOutputBuffer();

      //FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
      FTC_AddByteToOutputBuffer(SET_HIGH_BYTE_DATA_BITS_CMD, 0);
      FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

      //FTC_AddByteToOutputBuffer(CLK_DATA_BITS_OUT_ON_NEG_CLK_MSB_FIRST_CMD, 0); // set SCL,SDA,WP as out
      FTC_AddByteToOutputBuffer(CLK_DATA_BITS_OUT_ON_NEG_CLK_LSB_FIRST_CMD, 0); // set SCL,SDA,WP as out

      Status = FTC_SendBytesToDevice(ftHandle); // send off the command

      usleep(0);  // give up timeslice
    }
  }

  return Status;
}

FTC_STATUS ReadDataByteFromExternalDevice(FTC_HANDLE ftHandle, PI2CReadByteBuffer pI2CReadBuffer)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumDataBytesRead = 0;

  FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
  FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

  FTC_AddByteToOutputBuffer(CLK_DATA_BYTES_OUT_ON_NEG_CLK_MSB_FIRST_CMD, 0);  // set SCL,WP as out SDA as in

  SetI2CReadData(8);

  FTC_AddByteToOutputBuffer(SEND_ANSWER_BACK_IMMEDIATELY_CMD, 0);  // Send immediate

  Status = FTC_SendBytesToDevice(ftHandle); // send off the command

  //sleep(0); // give up timeslice

  if (Status == FTC_SUCCESS)
  {
    Status = ReadDataBytesFromExternalDevice(ftHandle, pI2CReadBuffer, 8, &dwNumDataBytesRead, 1);

    if (Status == FTC_SUCCESS)
    {
      FTC_ClearOutputBuffer();

      FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
      FTC_AddByteToOutputBuffer(dwSavedLowPinsValue, 0);

      FTC_AddByteToOutputBuffer(CLK_DATA_BITS_OUT_ON_NEG_CLK_MSB_FIRST_CMD, 0); // set SCL,SDA,WP as out

      Status = FTC_SendBytesToDevice(ftHandle); // send off the command
    }
  }

  return Status;
}

FTC_STATUS SendDataCheckAcknowledge(FTC_HANDLE ftHandle, BYTE DataByte, int AckType, DWORD dwAckTimeoutmSecs)
{
  FTC_STATUS Status = FTC_SUCCESS;
  I2CWriteByteBuffer I2CWriteBuffer;
  I2CReadByteBuffer I2CReadBuffer;
  BOOL bAckReceived = 0;

  I2CWriteBuffer[0] = DataByte;
  SetI2CWriteData(ftHandle, 8, &I2CWriteBuffer);

  if (AckType == NoAck)
  {
    Status = FTC_SendBytesToDevice(ftHandle); // send off the command
    //sleep(0); // give up timeslice
  }
  else{
    Status = ReadDataAckFromExternalDevice(ftHandle, &I2CReadBuffer, NUM_ACKNOWLEDGE_BITS, AckType, dwAckTimeoutmSecs);
  }
  return Status;
}

FTC_STATUS WriteAddressExternalDevice(FTC_HANDLE ftHandle, BYTE DeviceAddress, int ControlAckType,
                                                       DWORD dwControlAckTimeoutmSecs, BOOL bEnableDisableWriteProtect)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwControlAckCntr = 0;

  do
  {
    if (bEnableDisableWriteProtect == 0)
    {
      FTC_ClearOutputBuffer();

      SetI2CStartCondition(ftHandle, bEnableDisableWriteProtect);

      Status = SendDataCheckAcknowledge(ftHandle, DeviceAddress, ControlAckType, dwControlAckTimeoutmSecs);
    }
    else
    {
      FTC_ClearOutputBuffer();

      SetI2CStartCondition(ftHandle, bEnableDisableWriteProtect);

      Status = SendDataCheckAcknowledge(ftHandle, DeviceAddress, ControlAckType, dwControlAckTimeoutmSecs);
    }

    dwControlAckCntr++;
  }
  while ((Status == FTC_CONTROL_ACKNOWLEDGE_TIMEOUT) && (dwControlAckCntr < 100));
  return Status;
}

FTC_STATUS WriteControlToExternalDevice(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                                                         DWORD dwNumControlBytesToWrite, int ControlAckType,
                                                         DWORD dwControlAckTimeoutmSecs)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwControlByteIndex = 1;

  Status = WriteAddressExternalDevice(ftHandle, (*pWriteControlBuffer)[0], ControlAckType,
                                      dwControlAckTimeoutmSecs, 0);
  if (Status == FTC_SUCCESS)
  {
    if (dwNumControlBytesToWrite > 1)
    {
      do
      {
        Status = SendDataCheckAcknowledge(ftHandle, (*pWriteControlBuffer)[dwControlByteIndex], ControlAckType, dwControlAckTimeoutmSecs);
        dwControlByteIndex++;
      }
      while ((dwControlByteIndex < dwNumControlBytesToWrite) && (Status == FTC_SUCCESS)); 
    }
  }

  return Status;
}

FTC_STATUS WriteDataToExternalDevice(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                                                      DWORD dwNumControlBytesToWrite, BOOL bControlAcknowledge,
                                                      DWORD dwControlAckTimeoutmSecs, BOOL bStopCondition, 
                                                      DWORD dwDataWriteTypes, PWriteDataByteBuffer pWriteDataBuffer,
                                                      DWORD dwNumDataBytesToWrite, BOOL bDataAcknowledge,
                                                      DWORD dwDataAckTimeoutmSecs, PFTC_PAGE_WRITE_DATA pPageWriteData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwWriteDataBufferIndex = 0;
  int ControlAckType = NoAck;
  int DataAckType = NoAck;

  if (bControlAcknowledge != FALSE)
    ControlAckType = ControlAck;

  if (bDataAcknowledge != FALSE)
    DataAckType = DataAck;

  Status = WriteControlToExternalDevice(ftHandle, pWriteControlBuffer, dwNumControlBytesToWrite,
                                        ControlAckType, dwControlAckTimeoutmSecs);

  if (Status == FTC_SUCCESS)
  {
    switch(dwDataWriteTypes) 
    {
      case NO_WRITE_TYPE:
        break;
      case BYTE_WRITE_TYPE:
        Status = SendDataCheckAcknowledge(ftHandle, (*pWriteDataBuffer)[0], DataAckType, dwDataAckTimeoutmSecs);
        break;
      case PAGE_WRITE_TYPE:
        do
        {
          Status = SendDataCheckAcknowledge(ftHandle, (*pWriteDataBuffer)[dwWriteDataBufferIndex], DataAck, dwDataAckTimeoutmSecs);

          dwWriteDataBufferIndex++;
        }
        while ((dwWriteDataBufferIndex < dwNumDataBytesToWrite) && (Status == FTC_SUCCESS));
        break;
    }

    if ((Status == FTC_SUCCESS) && (bStopCondition != 0))
    {
      Status = SetI2CStopCondition(ftHandle);

      if (Status == FTC_SUCCESS)
        //Status = WriteAddressExternalDevice(ftHandle, (*pWriteControlBuffer)[0], ControlAckType,
        //                                    dwControlAckTimeoutmSecs, 1);  // Enable write protect
		WriteProtectEnable(ftHandle, 1);
    }
  }

  return Status;
}

FTC_STATUS ReadDataFromExternalDevice(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                                                       DWORD dwNumControlBytesToWrite, BOOL bControlAcknowledge,
                                                       DWORD dwControlAckTimeoutmSecs, DWORD dwDataReadTypes,
                                                       PReadDataByteBuffer pReadDataBuffer, DWORD dwNumDataBytesToRead)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwReadDataBufferIndex = 0;
  I2CReadByteBuffer I2CReadBuffer;
  I2CWriteByteBuffer I2CWriteBuffer;
  int ControlAckType = NoAck;

  if (bControlAcknowledge != 0)
    ControlAckType = ControlAck;

  if (dwDataReadTypes == BYTE_READ_TYPE)
  {
    // Set Read/Write bit to write ie 0
    (*pWriteControlBuffer)[0] = ((*pWriteControlBuffer)[0] & 0xFE);

    Status = WriteControlToExternalDevice(ftHandle, pWriteControlBuffer, dwNumControlBytesToWrite,
                                          ControlAckType, dwControlAckTimeoutmSecs);
    if (Status == FTC_SUCCESS)
    {
      // Set Read/Write bit to read ie 1
      (*pWriteControlBuffer)[0] = ((*pWriteControlBuffer)[0] | 0x01);

      Status = WriteAddressExternalDevice(ftHandle, (*pWriteControlBuffer)[0], ControlAckType,
                                          dwControlAckTimeoutmSecs, 1);
      if (Status == FTC_SUCCESS)
      {
        Status = ReadDataAckFromExternalDevice(ftHandle, &I2CReadBuffer, 8, NoAck, 0);

        if (Status == FTC_SUCCESS)
        {
          (*pReadDataBuffer)[dwReadDataBufferIndex] = I2CReadBuffer[0];

          Status = SetI2CStopCondition(ftHandle);
        }
      }
    }
  }
  else
  {
    // Set Read/Write bit to write ie 0
    (*pWriteControlBuffer)[0] = ((*pWriteControlBuffer)[0] & 0xFE);

    Status = WriteControlToExternalDevice(ftHandle, pWriteControlBuffer, dwNumControlBytesToWrite,
                                          ControlAckType, dwControlAckTimeoutmSecs);
    if (Status == FTC_SUCCESS)
    {
      // Set Read/Write bit to read ie 1
      (*pWriteControlBuffer)[0] = ((*pWriteControlBuffer)[0] | 0x01);

      Status = WriteAddressExternalDevice(ftHandle, (*pWriteControlBuffer)[0], ControlAckType,
                                            dwControlAckTimeoutmSecs, 1);
      if (Status == FTC_SUCCESS)
      {
        FTC_ClearOutputBuffer();

        // Write Ack to external device
        I2CWriteBuffer[0] = 0;

        do
        {
          Status = ReadDataByteFromExternalDevice(ftHandle, &I2CReadBuffer);
          if (Status == FTC_SUCCESS)
          {
            (*pReadDataBuffer)[dwReadDataBufferIndex] = I2CReadBuffer[0];

            dwReadDataBufferIndex++;

            if (dwReadDataBufferIndex < dwNumDataBytesToRead)
            {
              // Write Ack to external device
              SetI2CWriteData(ftHandle, 1, &I2CWriteBuffer);

              Status = FTC_SendBytesToDevice(ftHandle); // send off the command
            }
          }
        }
        while ((dwReadDataBufferIndex < dwNumDataBytesToRead) && (Status == FTC_SUCCESS));
      }
    }

    if (Status == FTC_SUCCESS)
      Status = SetI2CStopCondition(ftHandle);
  }

  return Status;
}

void CreateDeviceDataRecord(FTC_HANDLE ftHandle)
{
  DWORD dwDeviceIndex = 0;
  BOOL bDeviceDataRecordCreated = 0;

  for (dwDeviceIndex = 0; ((dwDeviceIndex < MAX_NUM_DEVICES) && !bDeviceDataRecordCreated); dwDeviceIndex++)
  {
    if (OpenedDevicesDataRecords[dwDeviceIndex].hDevice == 0)
    {
      bDeviceDataRecordCreated = 1;

      OpenedDevicesDataRecords[dwDeviceIndex].hDevice = ftHandle;
      OpenedDevicesDataRecords[dwDeviceIndex].dwCommsMode = FAST_MODE;
    }
  }

  if (bDeviceDataRecordCreated == TRUE)
    dwNumOpenedDevices = dwNumOpenedDevices + 1; 
}

INT GetDeviceDataRecordIndex(FTC_HANDLE ftHandle)
{
  DWORD dwDeviceIndex = 0;
  BOOLEAN bDeviceHandleFound = 0;
  INT iDeviceDataRecordIndex = -1;

  if (ftHandle != 0)
  {
    for (dwDeviceIndex = 0; ((dwDeviceIndex < MAX_NUM_DEVICES) && !bDeviceHandleFound); dwDeviceIndex++)
    {
      if (OpenedDevicesDataRecords[dwDeviceIndex].hDevice == ftHandle)
      {
        bDeviceHandleFound = 1;

        iDeviceDataRecordIndex = dwDeviceIndex;
      }
    }
  }
  else
  {
    // This code is executed if there is only one device connected to the system, this code is here just in case
    // that a device was unplugged from the system, while the system was still running
    for (dwDeviceIndex = 0; ((dwDeviceIndex < MAX_NUM_DEVICES) && !bDeviceHandleFound); dwDeviceIndex++)
    {
      if (OpenedDevicesDataRecords[dwDeviceIndex].hDevice != 0)
      {
        bDeviceHandleFound = 1;

        iDeviceDataRecordIndex = dwDeviceIndex;
      }
    }
  }

  return iDeviceDataRecordIndex;
}

void DeleteDeviceDataRecord(FTC_HANDLE ftHandle)
{
  DWORD dwDeviceIndex = 0;
  BOOLEAN bDeviceHandleFound = 0;

  for (dwDeviceIndex = 0; ((dwDeviceIndex < MAX_NUM_DEVICES) && !bDeviceHandleFound); dwDeviceIndex++)
  {
    if (OpenedDevicesDataRecords[dwDeviceIndex].hDevice == ftHandle)
    {
      bDeviceHandleFound = 1;

      OpenedDevicesDataRecords[dwDeviceIndex].hDevice = 0;
    }
  }

  if ((dwNumOpenedDevices > 0) && bDeviceHandleFound)
    dwNumOpenedDevices = dwNumOpenedDevices - 1;
}

DWORD GetCommunicationsMode(FTC_HANDLE ftHandle)
{
  DWORD dwCommsMode = FAST_MODE;
  INT iDeviceDataRecordIndex;

  iDeviceDataRecordIndex = GetDeviceDataRecordIndex(ftHandle);

  if (iDeviceDataRecordIndex != -1)
    dwCommsMode = OpenedDevicesDataRecords[iDeviceDataRecordIndex].dwCommsMode;

  return dwCommsMode;
}

void begin(void)
{
	//create mutex attribute variable
pthread_mutexattr_t mAttr;

// setup recursive mutex for mutex attribute
pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_RECURSIVE_NP);

// Use the mutex attribute to create the mutex
pthread_mutex_init(&m, &mAttr);

// Mutex attribute can be destroy after initializing the mutex variable
pthread_mutexattr_destroy(&mAttr);

  
  //InitializeCriticalSection(&threadAccess);
}

void end(void)
{
	pthread_mutex_destroy (&m);
  //DeleteCriticalSection(&threadAccess);
}

FTC_STATUS I2C_GetNumDevices_ssel(LPDWORD lpdwNumDevices)
{
  FTC_STATUS Status = FTC_SUCCESS;
  FT2232CDeviceIndexes FT2232CIndexes;

  pthread_mutex_lock (&m);

  *lpdwNumDevices = 0;

  Status = FTC_GetNumNotOpenedDevices(lpdwNumDevices, &FT2232CIndexes);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_GetNumHiSpeedDevices_ssel(LPDWORD lpdwNumHiSpeedDevices)
{
  FTC_STATUS Status = FTC_SUCCESS;
  HiSpeedDeviceIndexes HiSpeedIndexes;

  pthread_mutex_lock (&m);

  *lpdwNumHiSpeedDevices = 0;

  Status = FTC_GetNumHiSpeedDevices(lpdwNumHiSpeedDevices, &HiSpeedIndexes);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_GetDeviceNameLocationID(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_GetDeviceNameLocationID(dwDeviceNameIndex, lpDeviceNameBuffer, dwBufferSize, lpdwLocationID);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_GetHiSpeedDeviceNameLocationIDChannel(DWORD dwDeviceNameIndex, LPSTR lpDeviceNameBuffer, DWORD dwBufferSize, LPDWORD lpdwLocationID, LPSTR lpChannel, DWORD dwChannelBufferSize, LPDWORD lpdwHiSpeedDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwDeviceType = 0;

  pthread_mutex_lock (&m);

  *lpdwHiSpeedDeviceType = 0;

  Status = FTC_GetHiSpeedDeviceNameLocationIDChannel(dwDeviceNameIndex, lpDeviceNameBuffer, dwBufferSize, lpdwLocationID, lpChannel, dwChannelBufferSize, &dwDeviceType);

  if (Status == FTC_SUCCESS)
  {
    if ((dwDeviceType == FT_DEVICE_2232H) || (dwDeviceType == FT_DEVICE_4232H))
    {
      if (dwDeviceType == FT_DEVICE_2232H)
        *lpdwHiSpeedDeviceType = FT2232H_DEVICE_TYPE;
      else
        *lpdwHiSpeedDeviceType = FT4232H_DEVICE_TYPE;
    }
    else
      Status = FTC_DEVICE_NOT_FOUND;
  }

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_OpenDevice(FTC_HANDLE *pftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_OpenDevice(pftHandle);
  
  if (Status == FTC_SUCCESS)
    CreateDeviceDataRecord(*pftHandle);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_OpenSpecifiedDevice(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE *pftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_OpenSpecifiedDevice(lpDeviceName, dwLocationID, pftHandle);
  
  if (Status == FTC_SUCCESS)
    CreateDeviceDataRecord(*pftHandle);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_OpenSpecifiedHiSpeedDevice(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, FTC_HANDLE *pftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_OpenSpecifiedHiSpeedDevice(lpDeviceName, dwLocationID, lpChannel, pftHandle);
  if (Status == FTC_SUCCESS)
    CreateDeviceDataRecord(*pftHandle);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_GetHiSpeedDeviceType_ssel(FTC_HANDLE ftHandle, LPDWORD lpdwHiSpeedDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  BOOL bHiSpeedFT2232HTDeviceType = FALSE;

  pthread_mutex_lock (&m);

  *lpdwHiSpeedDeviceType = 0;

  Status = FTC_GetHiSpeedDeviceType(ftHandle, &bHiSpeedFT2232HTDeviceType);

  if (Status == FTC_SUCCESS)
  {
    // Is the device a FT2232H hi-speed device
    if (bHiSpeedFT2232HTDeviceType == TRUE)
      *lpdwHiSpeedDeviceType = FT2232H_DEVICE_TYPE;
    else
      *lpdwHiSpeedDeviceType = FT4232H_DEVICE_TYPE;
  }

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_CloseDevice_ssel(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_CloseDevice_c(ftHandle);

  DeleteDeviceDataRecord(ftHandle);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS WINAPI I2C_CloseDevice3(FTC_HANDLE ftHandle, PFTC_CLOSE_FINAL_STATE_PINS pCloseFinalStatePinsData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  if ((Status = FTC_IsDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    if (pCloseFinalStatePinsData != NULL)
    {
      if ((Status = SetTCKTDITMSPinsCloseState(ftHandle, pCloseFinalStatePinsData)) == FTC_SUCCESS)
        Status = I2C_CloseDevice_ssel(ftHandle);
    }
    else
      Status = FTC_NULL_CLOSE_FINAL_STATE_BUFFER_POINTER;
  }

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_InitDevice_ssel(FTC_HANDLE ftHandle, DWORD dwClockDivisor)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);
  if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    if ((Status = FTC_InitHiSpeedDevice(ftHandle)) == FTC_SUCCESS)
    {
      Status = InitDevice(ftHandle, dwClockDivisor);
    }
  }
  if (Status == FTC_INVALID_HANDLE)
  {
    if ((Status = FTC_IsDeviceHandleValid_c(ftHandle)) == FTC_SUCCESS)
    {
      Status = InitDevice(ftHandle, dwClockDivisor);
    }
  }
  pthread_mutex_unlock (&m);									  
  return Status;
}

FTC_STATUS I2C_TurnOnDivideByFiveClockingHiSpeedDevice_ssel(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  

  Status = FTC_TurnOnDivideByFiveClockingHiSpeedDevice(ftHandle);

  

  return Status;
}

FTC_STATUS I2C_TurnOffDivideByFiveClockingHiSpeedDevice_ssel(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_TurnOffDivideByFiveClockingHiSpeedDevice(ftHandle);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_TurnOnThreePhaseDataClockingHiSpeedDevice_ssel(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_TurnOnAdaptiveClockingHiSpeedDevice(ftHandle);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_TurnOffThreePhaseDataClockingHiSpeedDevice_ssel(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_TurnOffAdaptiveClockingHiSpeedDevice(ftHandle);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_SetDeviceLatencyTimer_ssel(FTC_HANDLE ftHandle, BYTE LatencyTimermSec)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_SetDeviceLatencyTimer(ftHandle, LatencyTimermSec);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_GetDeviceLatencyTimer_ssel(FTC_HANDLE ftHandle, LPBYTE lpLatencyTimermSec)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_GetDeviceLatencyTimer(ftHandle, lpLatencyTimermSec);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_GetClock_ssel(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  if ((dwClockDivisor >= MIN_CLOCK_DIVISOR) && (dwClockDivisor <= MAX_CLOCK_DIVISOR))
    FTC_GetClockFrequencyValues(dwClockDivisor, lpdwClockFrequencyHz);
  else
    Status = FTC_INVALID_CLOCK_DIVISOR;

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_GetHiSpeedDeviceClock_ssel(DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  if ((dwClockDivisor >= MIN_CLOCK_DIVISOR) && (dwClockDivisor <= MAX_CLOCK_DIVISOR))
    FTC_GetHiSpeedDeviceClockFrequencyValues(dwClockDivisor, lpdwClockFrequencyHz);
  else
    Status = FTC_INVALID_CLOCK_DIVISOR;

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_SetClock_ssel(FTC_HANDLE ftHandle, DWORD dwClockDivisor, LPDWORD lpdwClockFrequencyHz)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  if ((dwClockDivisor >= MIN_CLOCK_DIVISOR) && (dwClockDivisor <= MAX_CLOCK_DIVISOR))
  {
    if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
    {
      FTC_GetHiSpeedDeviceClockFrequencyValues3(ftHandle, dwClockDivisor, lpdwClockFrequencyHz);
    }

    if (Status == FTC_INVALID_HANDLE)
    {
      if ((Status = FTC_IsDeviceHandleValid_c(ftHandle)) == FTC_SUCCESS)
      {
        FTC_GetClockFrequencyValues(dwClockDivisor, lpdwClockFrequencyHz);
      }
    }

    if (Status == FTC_SUCCESS)
    {
      Status = SetDataInOutClockFrequency(ftHandle, dwClockDivisor);
    }
  }
  else
    Status = FTC_INVALID_CLOCK_DIVISOR;

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_SetDeviceLoopbackState(FTC_HANDLE ftHandle, BOOL bLoopbackState)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  if ((Status = FTC_IsDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
    Status = FTC_SetDeviceLoopbackState(ftHandle, bLoopbackState);

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS WINAPI I2C_SetCommunicationsMode(FTC_HANDLE ftHandle, DWORD dwCommsMode)
{
  FTC_STATUS Status = FTC_SUCCESS;
  INT iDeviceDataRecordIndex;

  pthread_mutex_lock (&m);

  if ((Status = FTC_IsDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    if ((dwCommsMode == STANDARD_MODE) || (dwCommsMode == FAST_MODE) || (dwCommsMode == STRETCH_DATA_MODE))
    {
      iDeviceDataRecordIndex = GetDeviceDataRecordIndex(ftHandle);

      if (iDeviceDataRecordIndex != -1)
        OpenedDevicesDataRecords[iDeviceDataRecordIndex].dwCommsMode = dwCommsMode; 
    }
    else
      Status = FTC_INVALID_COMMS_MODE;
  }

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_SetGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, PFTC_INPUT_OUTPUT_PINS pHighInputOutputPinsData)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwHighPinsDirection = 0;
  DWORD dwHighPinsValue = 0;

  pthread_mutex_lock (&m);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
  {
    if (pHighInputOutputPinsData != NULL)
    {
      if (pHighInputOutputPinsData->bPin1InputOutputState != 0)
        dwHighPinsDirection = (dwHighPinsDirection | 0x01);
      if (pHighInputOutputPinsData->bPin2InputOutputState != 0)
        dwHighPinsDirection = (dwHighPinsDirection | 0x02);
      if (pHighInputOutputPinsData->bPin3InputOutputState != 0)
        dwHighPinsDirection = (dwHighPinsDirection | 0x04);
      if (pHighInputOutputPinsData->bPin4InputOutputState != 0)
        dwHighPinsDirection = (dwHighPinsDirection | 0x08);
      if (pHighInputOutputPinsData->bPin1LowHighState != 0)
        dwHighPinsValue = (dwHighPinsValue | 0x01);
      if (pHighInputOutputPinsData->bPin2LowHighState != 0)
        dwHighPinsValue = (dwHighPinsValue | 0x02);
      if (pHighInputOutputPinsData->bPin3LowHighState != 0)
        dwHighPinsValue = (dwHighPinsValue | 0x04);
      if (pHighInputOutputPinsData->bPin4LowHighState != 0)
        dwHighPinsValue = (dwHighPinsValue | 0x08);

      Status = SetGeneralPurposeHighInputOutputPins(ftHandle, dwHighPinsDirection, dwHighPinsValue);
    }
    else
    {
      Status = FTC_NULL_INPUT_OUTPUT_BUFFER_POINTER;
    }
  }

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_SetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                              PFTC_INPUT_OUTPUT_PINS pLowInputOutputPinsData,
                                                                              BOOL bControlHighInputOutputPins,
                                                                              PFTH_INPUT_OUTPUT_PINS pHighInputOutputPinsData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    if ((pLowInputOutputPinsData != NULL) && (pHighInputOutputPinsData != NULL))
    {
      if (bControlLowInputOutputPins != FALSE)
        Status = SetHiSpeedDeviceGeneralPurposeLowInputOutputPins(ftHandle, pLowInputOutputPinsData);
      
      if (bControlHighInputOutputPins != FALSE)
        Status = SetHiSpeedDeviceGeneralPurposeHighInputOutputPins(ftHandle, pHighInputOutputPinsData);
    }
    else
      Status = FTC_NULL_INPUT_OUTPUT_BUFFER_POINTER;
  }

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_GetGeneralPurposeHighInputOutputPins(FTC_HANDLE ftHandle, PFTC_LOW_HIGH_PINS pHighPinsInputData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
  {
    if (pHighPinsInputData != NULL)
      Status = GetGeneralPurposeHighInputOutputPins(ftHandle, pHighPinsInputData);
    else
      Status = FTC_NULL_INPUT_BUFFER_POINTER;
  }

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_GetHiSpeedDeviceGeneralPurposeInputOutputPins(FTC_HANDLE ftHandle, BOOL bControlLowInputOutputPins,
                                                                              PFTC_LOW_HIGH_PINS pLowPinsInputData,
                                                                              BOOL bControlHighInputOutputPins,
                                                                              PFTH_LOW_HIGH_PINS pHighPinsInputData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    if ((pLowPinsInputData != NULL) && (pHighPinsInputData != NULL))
    {
      // Put in this small delay incase the application programmer does a get GPIOs immediately after a set GPIOs
      usleep(5000);

      if (bControlLowInputOutputPins != FALSE)
        Status = GetHiSpeedDeviceGeneralPurposeLowInputOutputPins(ftHandle, pLowPinsInputData);

      if (bControlHighInputOutputPins != FALSE)
        Status = GetHiSpeedDeviceGeneralPurposeHighInputOutputPins(ftHandle, pHighPinsInputData);
    }
    else
      Status = FTC_NULL_INPUT_OUTPUT_BUFFER_POINTER;
  }

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_WriteDataToExternalDevice(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                                                          DWORD dwNumControlBytesToWrite, BOOL bControlAcknowledge,
                                                          DWORD dwControlAckTimeoutmSecs, BOOL bStopCondition, 
                                                          DWORD dwDataWriteTypes, PWriteDataByteBuffer pWriteDataBuffer,
                                                          DWORD dwNumDataBytesToWrite, BOOL bDataAcknowledge,
                                                          DWORD dwDataAckTimeoutmSecs, PFTC_PAGE_WRITE_DATA pPageWriteData)
{
  FTC_STATUS Status = FTC_SUCCESS;

  pthread_mutex_lock (&m);

  Status = FTC_IsDeviceHandleValid(ftHandle);

  if (Status == FTC_SUCCESS)
  {
    Status = CheckWriteDataToExternalDeviceParameters(pWriteControlBuffer, dwNumControlBytesToWrite, dwDataWriteTypes, 
                                                      pWriteDataBuffer, dwNumDataBytesToWrite, pPageWriteData);

    if (Status == FTC_SUCCESS)
      Status = WriteDataToExternalDevice(ftHandle, pWriteControlBuffer, dwNumControlBytesToWrite,
                                         bControlAcknowledge, dwControlAckTimeoutmSecs, bStopCondition,
                                         dwDataWriteTypes, pWriteDataBuffer, dwNumDataBytesToWrite,
                                         bDataAcknowledge, dwDataAckTimeoutmSecs, pPageWriteData);
  }

  pthread_mutex_unlock (&m);

  return Status;
}

FTC_STATUS I2C_ReadDataFromExternalDevice(FTC_HANDLE ftHandle, PWriteControlByteBuffer pWriteControlBuffer,
                                                           DWORD dwNumControlBytesToWrite, BOOL bControlAcknowledge,
                                                           DWORD dwControlAckTimeoutmSecs, DWORD dwDataReadTypes,
                                                           PReadDataByteBuffer pReadDataBuffer, DWORD dwNumDataBytesToRead)
{
  pthread_mutex_lock (&m);

  FTC_STATUS Status = FTC_SUCCESS;

  Status = FTC_IsDeviceHandleValid(ftHandle);
  if (Status == FTC_SUCCESS)
  {
    Status = CheckReadDataFromExternalDeviceParameters(pWriteControlBuffer, dwNumControlBytesToWrite, dwDataReadTypes, 
                                                       pReadDataBuffer, dwNumDataBytesToRead);
    if (Status == FTC_SUCCESS)
      Status = ReadDataFromExternalDevice(ftHandle, pWriteControlBuffer, dwNumControlBytesToWrite,
                                          bControlAcknowledge, dwControlAckTimeoutmSecs, dwDataReadTypes,
                                          pReadDataBuffer, dwNumDataBytesToRead);
  }

  pthread_mutex_unlock (&m);

  return Status;
}


FTC_STATUS I2C_GetErrorCodeString_ssel(LPSTR lpLanguage, FTC_STATUS StatusCode,
                                                     LPSTR lpErrorMessageBuffer, DWORD dwBufferSize)
{
  FTC_STATUS Status = FTC_SUCCESS;
  char szErrorMsg[MAX_ERROR_MSG_SIZE];
  INT iCharCntr = 0;

	pthread_mutex_lock (&m);

  if ((lpLanguage != NULL) && (lpErrorMessageBuffer != NULL))
  {
    for (iCharCntr = 0; (iCharCntr < MAX_ERROR_MSG_SIZE); iCharCntr++)
      szErrorMsg[iCharCntr] = '\0';
    if (((StatusCode >= FTC_SUCCESS) && (StatusCode <= FTC_INSUFFICIENT_RESOURCES)) ||
        ((StatusCode >= FTC_FAILED_TO_COMPLETE_COMMAND) && (StatusCode <= FTC_INVALID_STATUS_CODE)))
    {
      if (strcmp(lpLanguage, ENGLISH) == 0)
      {
        if ((StatusCode >= FTC_SUCCESS) && (StatusCode <= FTC_INSUFFICIENT_RESOURCES))
          strcpy(szErrorMsg, EN_Common_Errors[StatusCode]);
        else
          strcpy(szErrorMsg, EN_New_Errors[(StatusCode - FTC_FAILED_TO_COMPLETE_COMMAND)]);
      }
      else
      {
        strcpy(szErrorMsg, EN_New_Errors[FTC_INVALID_LANGUAGE_CODE - FTC_FAILED_TO_COMPLETE_COMMAND]);

        Status = FTC_INVALID_LANGUAGE_CODE;
      }
    }
    else
    {
      sprintf(szErrorMsg, "%s%d", EN_New_Errors[FTC_INVALID_STATUS_CODE - FTC_FAILED_TO_COMPLETE_COMMAND], StatusCode);

      Status = FTC_INVALID_STATUS_CODE;
    }

    if (dwBufferSize > strlen(szErrorMsg))
      strcpy(lpErrorMessageBuffer, szErrorMsg);
    else
      Status = FTC_ERROR_MESSAGE_BUFFER_TOO_SMALL;
  }
  else
  {
    if (lpLanguage == NULL)
      Status = FTC_NULL_LANGUAGE_CODE_BUFFER_POINTER;
    else
      Status = FTC_NULL_ERROR_MESSAGE_BUFFER_POINTER;
  }

  pthread_mutex_unlock (&m);

  return Status;
}

// fix - Andrew 2/11/05

void RepeatBlock(char byte, DWORD num)
{
	DWORD i=0;

	for(i=0; i<num; i++)
	{
		FTC_AddByteToOutputBuffer((DWORD)0x80, 0);
		FTC_AddByteToOutputBuffer((DWORD)byte, 0); 
		FTC_AddByteToOutputBuffer((DWORD)0x03, 0); 
	}
}

void ClockByte(unsigned char byte)
{
	int i=0;

	//option 2
	for(i=0; i<8; i++)
	{
		FTC_AddByteToOutputBuffer((DWORD)0x13, 0); //latch data on -ve edge msb first no read
		FTC_AddByteToOutputBuffer((DWORD)0x00, 0);

		if(byte&0x80)
			FTC_AddByteToOutputBuffer((DWORD)0xff, 0); //write address
		else
			FTC_AddByteToOutputBuffer((DWORD)0x00, 0);
		
		RepeatBlock((byte>>6)&0x02, 1);
		byte <<= 1;
	}
}

void WriteProtectEnable(FTC_HANDLE ftHandle, bool b)
{
	FTC_STATUS Status;

	if(b)
	{
		// tristate SDA SCL
		FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
		FTC_AddByteToOutputBuffer(dwSavedLowPinsValue & 0xEF , 0); //WP off

		FTC_AddByteToOutputBuffer(0x13, 0); // set SCL,SDA as input,WP as out

	}else
	{
		// tristate SDA SCL
		FTC_AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, 0);
		FTC_AddByteToOutputBuffer(dwSavedLowPinsValue | 0x10, 0); //WP On

		FTC_AddByteToOutputBuffer(0x10, 0); // set SCL,SDA as input,WP as out
	}

	Status = FTC_SendBytesToDevice(ftHandle); // send off the command
	usleep(0); // give up timeslice
}
