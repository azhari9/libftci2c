/*++

Copyright (c) 2008  Future Technology Devices International Ltd.

Module Name:

    FT2232hcpp.h

Abstract:

    FT2232Hcpp And FT4232H Hi-Speed Dual Type Devices Base Class Declaration/Definition.

Environment:

    kernel & user mode

Revision History:

    07/07/08    kra     Created.
    01/08/08    kra     Removed ftcjtag.h include, modified FTC_GetHiSpeedDeviceNameLocationIDChannel and
                        FTC_GetHiSpeedDeviceType methods, to remove reference to FT2232H_DEVICE_TYPE and
                        FT2232H_DEVICE_TYPE enum values defined in DLL header file.
    15/08/08    kra     Modified FTC_IsDeviceNameLocationIDValid and FTC_InsertDeviceHandle methods to add the type
                        of hi-speed device to OpenedHiSpeedDevices ie array of FTC_HI_SPEED_DEVICE_DATA structures.
                        Add new FTC_IsDeviceHiSpeedType method.
	
--*/
#define WIO_DEFINED

#ifndef _GNU_SOURCE 
#define _GNU_SOURCE
#endif 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stdafx.h"
#include <unistd.h>
#include "FT2232h.h"
#include "FTD2XX.h"

INT iDeviceCntr = 0;
UINT uiNumOpenedHiSpeedDevices = 0;
FTC_HI_SPEED_DEVICE_DATA OpenedHiSpeedDevices[MAX_NUM_DEVICES]={0};

BOOL FTC_DeviceInUse(LPSTR lpDeviceName, DWORD dwLocationID)
{
  BOOL bDeviceInUse = 0;
  DWORD dwProcessId = 0;
  BOOL bLocationIDFound = 0;
  INT iDeviceCntr = 0;
  uiNumOpenedHiSpeedDevices = 0;
  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = getpid();
    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bLocationIDFound); iDeviceCntr++)
    {
      // Only check device name and location id not the current application
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId != dwProcessId)
      {
        if (strcmp(OpenedHiSpeedDevices[iDeviceCntr].szDeviceName, lpDeviceName) == 0)
        {
          if (OpenedHiSpeedDevices[iDeviceCntr].dwLocationID == dwLocationID)
            bLocationIDFound = 1;
        }
      }
    }

    if (bLocationIDFound)
      bDeviceInUse = 1;
  }
  return bDeviceInUse;
}

BOOL FTC_DeviceOpened(LPSTR lpDeviceName, DWORD dwLocationID, FTC_HANDLE *pftHandle)
{
  BOOL bDeviceOpen = 0;
  DWORD dwProcessId = 0;
  bool bLocationIDFound = 0;
  INT iDeviceCntr = 0;

  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = getpid();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bLocationIDFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (strcmp(OpenedHiSpeedDevices[iDeviceCntr].szDeviceName, lpDeviceName) == 0)
        {
          if (OpenedHiSpeedDevices[iDeviceCntr].dwLocationID == dwLocationID)
          {
            // Device has already been opened by this application, so just return the handle to the device
            *pftHandle = OpenedHiSpeedDevices[iDeviceCntr].hDevice;
            bLocationIDFound = 1;
          }
        }
      }
    }

    if (bLocationIDFound)
      bDeviceOpen = 1;
  }

  return bDeviceOpen;
}

FTC_STATUS FTC_IsDeviceNameLocationIDValid(LPSTR lpDeviceName, DWORD dwLocationID, LPDWORD lpdwDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumHiSpeedDevices = 0;
  HiSpeedDeviceIndexes HiSpeedIndexes;
  DWORD dwFlags = 0;
  DWORD dwProductVendorID = 0;
  DWORD dwLocID = 0;
  SerialNumber szSerialNumber;
  char szDeviceNameBuffer[DEVICE_STRING_BUFF_SIZE + 1];
  FT_HANDLE ftHandle;
  bool bDeviceNameFound = 0;
  bool bLocationIDFound = 0;
  DWORD dwDeviceIndex = 0;

  // Get the number of hi-speed devices connected to the system
  if ((Status = FTC_GetNumHiSpeedDevices(&dwNumHiSpeedDevices, &HiSpeedIndexes)) == FTC_SUCCESS)
  {
    if (dwNumHiSpeedDevices > 0)
    {
      do
      {
        bDeviceNameFound = 0;

        // If the FT_GetDeviceInfoDetail function returns a location id of 0 then this device is already open
        Status = FT_GetDeviceInfoDetail(HiSpeedIndexes[dwDeviceIndex], &dwFlags, lpdwDeviceType, &dwProductVendorID,
                                        &dwLocID, szSerialNumber, szDeviceNameBuffer, &ftHandle);

        if (Status == FTC_SUCCESS)
        {
          if (strcmp(szDeviceNameBuffer, lpDeviceName) == 0)
          {
            bDeviceNameFound = 1;

            if (dwLocID == dwLocationID)
              bLocationIDFound = 1;
          }
        }
        dwDeviceIndex++;
      }
      while ((dwDeviceIndex < dwNumHiSpeedDevices) && (Status == FTC_SUCCESS) && (bDeviceNameFound == 0));

      if (bDeviceNameFound == 1)
      {
        if (dwLocID == 0) {
          Status = FTC_DEVICE_IN_USE;
        }
        else 
        {
          if (bLocationIDFound == 0)
            Status = FTC_INVALID_LOCATION_ID;
        }
      }
      else
        Status = FTC_INVALID_DEVICE_NAME;
    }
    else
      Status = FTC_DEVICE_NOT_FOUND;
  }

  return Status;
}

FTC_STATUS FTC_IsDeviceHiSpeedType1(FT_DEVICE_LIST_INFO_NODE devInfo, LPBOOL lpbHiSpeedDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  LPSTR pszStringSearch;

  *lpbHiSpeedDeviceType = 0;

  // The MPSSE hi-speed controller on both the FT2232H and FT4232H hi-speed dual devices, is available
  // through channel A and channel B
  if ((devInfo.Type == FT_DEVICE_2232H) || (devInfo.Type == FT_DEVICE_4232H))
  {
    // Search for the first occurrence of the channel string ie ' A' or ' B'. The Description field contains the device name
//    if (((pszStringSearch = strstr(strupr(devInfo.Description), DEVICE_NAME_CHANNEL_A)) != NULL) ||
  //      ((pszStringSearch = strstr(strupr(devInfo.Description), DEVICE_NAME_CHANNEL_B)) != NULL))
	if(((pszStringSearch = strcasestr(devInfo.Description,DEVICE_NAME_CHANNEL_A)) != NULL) ||
	    ((pszStringSearch = strcasestr(devInfo.Description,DEVICE_NAME_CHANNEL_B)) != NULL))
    {
      // Ensure the last two characters of the device name is ' A' ie channel A or ' B' ie channel B
      if (strlen(pszStringSearch) == 2)
        *lpbHiSpeedDeviceType = 1; 
    }
  }
  return Status;
}

void FTC_InsertDeviceHandle_h(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, DWORD dwDeviceType, FTC_HANDLE ftHandle)
{
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  bool bDeviceftHandleInserted = 0;
  if (uiNumOpenedHiSpeedDevices < MAX_NUM_DEVICES)
  {
    dwProcessId = getpid();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDeviceftHandleInserted); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == 0)
      {
        OpenedHiSpeedDevices[iDeviceCntr].dwProcessId = dwProcessId;
        strcpy(OpenedHiSpeedDevices[iDeviceCntr].szDeviceName, lpDeviceName);
        OpenedHiSpeedDevices[iDeviceCntr].dwLocationID = dwLocationID;
        strcpy(OpenedHiSpeedDevices[iDeviceCntr].szChannel, lpChannel);
        OpenedHiSpeedDevices[iDeviceCntr].bDivideByFiveClockingState = 1;
        OpenedHiSpeedDevices[iDeviceCntr].dwDeviceType = dwDeviceType;
        OpenedHiSpeedDevices[iDeviceCntr].hDevice = ftHandle;

        uiNumOpenedHiSpeedDevices = uiNumOpenedHiSpeedDevices + 1;

        bDeviceftHandleInserted = 1;
      }
    }
  }
}

void FTC_SetDeviceDivideByFiveState(FTC_HANDLE ftHandle, BOOL bDivideByFiveClockingState)
{
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  BOOL bDevicempHandleFound = 0;

  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = getpid();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDevicempHandleFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (OpenedHiSpeedDevices[iDeviceCntr].hDevice == ftHandle)
        {
          bDevicempHandleFound = 1;

          OpenedHiSpeedDevices[iDeviceCntr].bDivideByFiveClockingState = bDivideByFiveClockingState;
        }
      }
    }
  }
}

BOOL FTC_GetDeviceDivideByFiveState(FTC_HANDLE ftHandle)
{
  BOOL bDivideByFiveClockingState = 0;
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  BOOL bDevicempHandleFound = 0;

  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = getpid();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDevicempHandleFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (OpenedHiSpeedDevices[iDeviceCntr].hDevice == ftHandle)
        {
          bDevicempHandleFound = 1;

          bDivideByFiveClockingState = OpenedHiSpeedDevices[iDeviceCntr].bDivideByFiveClockingState;
        }
      }
    }
  }

  return bDivideByFiveClockingState;
}

FTC_STATUS FTC_GetNumHiSpeedDevices(LPDWORD lpdwNumHiSpeedDevices, HiSpeedDeviceIndexes *HiSpeedIndexes)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumOfDevices = 0;
  FT_DEVICE_LIST_INFO_NODE *pDevInfoList = NULL;
  FT_DEVICE_LIST_INFO_NODE devInfo;
  DWORD dwDeviceIndex = 0;
  BOOL bHiSpeedTypeDevice = 0;

  *lpdwNumHiSpeedDevices = 0;

  // Get the number of high speed devices(FT2232H and FT4232H) connected to the system
  if ((Status = FT_CreateDeviceInfoList(&dwNumOfDevices)) == FTC_SUCCESS)
  {
    if (dwNumOfDevices > 0)
    {
      // allocate storage for the device list based on dwNumOfDevices
	pDevInfoList = malloc(dwNumOfDevices * sizeof(FT_DEVICE_LIST_INFO_NODE));
	  //(pDevInfoList = new FT_DEVICE_LIST_INFO_NODE[dwNumOfDevices])
      if ( pDevInfoList != NULL )
      {
        if ((Status = FT_GetDeviceInfoList(pDevInfoList, &dwNumOfDevices)) == FTC_SUCCESS)
        {
          do
          {
            devInfo = pDevInfoList[dwDeviceIndex];

            if ((Status = FTC_IsDeviceHiSpeedType1(devInfo, &bHiSpeedTypeDevice)) == FTC_SUCCESS)
            {
              if (bHiSpeedTypeDevice == 1)
              {
                // The number of devices returned is, not opened devices ie channel A and channel B plus devices opened
                // by the calling application. Devices previously opened by another application are not included in this
                // number.
                (*HiSpeedIndexes)[*lpdwNumHiSpeedDevices] = dwDeviceIndex;

                *lpdwNumHiSpeedDevices = *lpdwNumHiSpeedDevices + 1;
              }
            }

            dwDeviceIndex++;
          }
          while ((dwDeviceIndex < dwNumOfDevices) && (Status == FTC_SUCCESS));
        }

        //delete pDevInfoList;
	free(pDevInfoList);
      }
      else
        Status = FTC_INSUFFICIENT_RESOURCES;
    }
  }

  return Status;
}

FTC_STATUS FTC_GetHiSpeedDeviceNameLocationIDChannel(DWORD dwDeviceIndex, LPSTR lpDeviceName, DWORD dwDeviceNameBufferSize, LPDWORD lpdwLocationID, LPSTR lpChannel, DWORD dwChannelBufferSize, LPDWORD lpdwDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumHiSpeedDevices = 0;
  HiSpeedDeviceIndexes HiSpeedIndexes;
  DWORD dwFlags = 0;
  DWORD dwProductVendorID = 0;
  SerialNumber szSerialNumber;
  char szDeviceNameBuffer[DEVICE_STRING_BUFF_SIZE + 1];
  FT_HANDLE ftHandle;
  LPSTR pszStringSearch;

  *lpdwDeviceType = 0;

  if ((lpDeviceName != NULL) && (lpChannel != NULL))
  {
    if ((Status = FTC_GetNumHiSpeedDevices(&dwNumHiSpeedDevices, &HiSpeedIndexes)) == FTC_SUCCESS)
    {
      if (dwNumHiSpeedDevices > 0)
      {
        if (dwDeviceIndex < dwNumHiSpeedDevices)
        {
          Status = FT_GetDeviceInfoDetail(HiSpeedIndexes[dwDeviceIndex], &dwFlags, lpdwDeviceType, &dwProductVendorID,
                                         lpdwLocationID, szSerialNumber, szDeviceNameBuffer, &ftHandle);

          if (Status == FTC_SUCCESS)
          {
            if (strlen(szDeviceNameBuffer) <= dwDeviceNameBufferSize)
            {
              strcpy(lpDeviceName, szDeviceNameBuffer);

              // Check for hi-speed device channel A or channel B
              if (((pszStringSearch = strcasestr(szDeviceNameBuffer, DEVICE_NAME_CHANNEL_A)) != NULL) ||
                  ((pszStringSearch = strcasestr(szDeviceNameBuffer, DEVICE_NAME_CHANNEL_B)) != NULL))
              {
                if (dwChannelBufferSize >= CHANNEL_STRING_MIN_BUFF_SIZE)
                {
                  if ((pszStringSearch = strcasestr(szDeviceNameBuffer, DEVICE_NAME_CHANNEL_A)) != NULL)
                    strcpy(lpChannel, CHANNEL_A);
                  else
                    strcpy(lpChannel, CHANNEL_B);
                }
                else
                  Status = FTC_CHANNEL_BUFFER_TOO_SMALL;
              }
              else
                Status = FTC_DEVICE_NOT_FOUND;
            }
            else
              Status = FTC_DEVICE_NAME_BUFFER_TOO_SMALL;
          }
        }
        else
          Status = FTC_INVALID_DEVICE_NAME_INDEX;
      }
      else
        Status = FTC_DEVICE_NOT_FOUND;
    }
  }
  else
  {
    if (lpDeviceName == NULL)
      Status = FTC_NULL_DEVICE_NAME_BUFFER_POINTER;
    else
      Status = FTC_NULL_CHANNEL_BUFFER_POINTER;
  }

  return Status;
}

FTC_STATUS FTC_OpenSpecifiedHiSpeedDevice(LPSTR lpDeviceName, DWORD dwLocationID, LPSTR lpChannel, FTC_HANDLE *pftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwDeviceType = 0;
  FT_HANDLE ftHandle;

  if ((lpDeviceName != NULL) && (lpChannel != NULL))
  {
    if ((strcmp(lpChannel, CHANNEL_A) == 0) || (strcmp(lpChannel, CHANNEL_B) == 0))
    {
      if ((Status = FTC_IsDeviceNameLocationIDValid(lpDeviceName, dwLocationID, &dwDeviceType)) == FTC_SUCCESS)
      {
        if (!FTC_DeviceInUse(lpDeviceName, dwLocationID))
        {
          if (!FTC_DeviceOpened(lpDeviceName, dwLocationID, pftHandle))
          {
           // if ((Status = FT_OpenEx((PVOID)dwLocationID, FT_OPEN_BY_LOCATION, (FT_HANDLE*)&ftHandle)) == FTC_SUCCESS)
	     if ((Status = FT_OpenEx(lpDeviceName, FT_OPEN_BY_DESCRIPTION, &ftHandle)) == FTC_SUCCESS)
            {
               *pftHandle = (DWORD)ftHandle;
               FTC_InsertDeviceHandle_h(lpDeviceName, dwLocationID, lpChannel, dwDeviceType, *pftHandle);
            }
          }
        }
        else
          Status = FTC_DEVICE_IN_USE;
      }
    }
    else
      Status = FTC_INVALID_CHANNEL;
  }
  else
  {
    if (lpDeviceName == NULL)
      Status = FTC_NULL_DEVICE_NAME_BUFFER_POINTER;
    else
      Status = FTC_NULL_CHANNEL_BUFFER_POINTER;
  }
  return Status;
}

FTC_STATUS FTC_GetHiSpeedDeviceType(FTC_HANDLE ftHandle, LPBOOL lpbHiSpeedFT2232HTDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  BOOL bHiSpeedTypeDevice = 0;

  if ((Status = FTC_IsHiSpeedDeviceHandleValid( ftHandle)) == FTC_SUCCESS)
  {
    if ((Status = FTC_IsDeviceHiSpeedType2(ftHandle, &bHiSpeedTypeDevice)) == FTC_SUCCESS)
    {
      // Is the device a hi-speed device ie a FT2232H or FT4232H hi-speed device
      if (bHiSpeedTypeDevice == 1)
        Status = FTC_IsDeviceHiSpeedFT2232HType(ftHandle, lpbHiSpeedFT2232HTDeviceType);
      else
        Status = FTC_INVALID_HANDLE;
    }
  }
 
  return Status;
}

FTC_STATUS FTC_CloseDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_SUCCESS)
  {
    Status = FT_Close((FT_HANDLE)ftHandle);

    FTC_RemoveHiSpeedDeviceHandle(ftHandle);
  }

  if (Status == FTC_INVALID_HANDLE) {
    Status = FTC_CloseDevice_c(ftHandle);
  }

  return Status;
}

FTC_STATUS FTC_IsDeviceHandleValid(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((Status = FTC_IsHiSpeedDeviceHandleValid(ftHandle)) == FTC_INVALID_HANDLE)
  {
    Status = FTC_IsDeviceHandleValid_c(ftHandle);
  }

  return Status;
}

FTC_STATUS FTC_IsHiSpeedDeviceHandleValid(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  bool bDevicempHandleFound = 0;

  if ((uiNumOpenedHiSpeedDevices > 0) && (ftHandle > 0))
  {
    dwProcessId = getpid();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDevicempHandleFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (OpenedHiSpeedDevices[iDeviceCntr].hDevice == ftHandle)
          bDevicempHandleFound = 1;
      }
    }

    if (!bDevicempHandleFound)
      Status = FTC_INVALID_HANDLE;
  }
  else
    Status = FTC_INVALID_HANDLE;

  return Status;
}

void FTC_RemoveHiSpeedDeviceHandle(FTC_HANDLE ftHandle)
{
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  bool bDevicempHandleFound = 0;

  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = getpid();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDevicempHandleFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (OpenedHiSpeedDevices[iDeviceCntr].hDevice == ftHandle)
        {
          OpenedHiSpeedDevices[iDeviceCntr].dwProcessId = 0;
          strcpy(OpenedHiSpeedDevices[iDeviceCntr].szDeviceName, "");
          OpenedHiSpeedDevices[iDeviceCntr].dwLocationID = 0;
          OpenedHiSpeedDevices[iDeviceCntr].hDevice = 0;

          uiNumOpenedHiSpeedDevices = uiNumOpenedHiSpeedDevices - 1;

          bDevicempHandleFound = 1;
        }
      }
    }
  }
}

FTC_STATUS FTC_InitHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((Status = FTC_TurnOffDivideByFiveClockingHiSpeedDevice(ftHandle)) == FTC_SUCCESS)
  {
    if ((Status = FTC_TurnOffAdaptiveClockingHiSpeedDevice(ftHandle)) == FTC_SUCCESS)
      Status = FTC_TurnOffThreePhaseDataClockingHiSpeedDevice(ftHandle);
  }

  return Status;
}

FTC_STATUS FTC_TurnOnDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((Status = FTC_IsHiSpeedDeviceHandleValid( ftHandle)) == FTC_SUCCESS)
  {
    FTC_SetDeviceDivideByFiveState(ftHandle, 1);

    FTC_AddByteToOutputBuffer(TURN_ON_DIVIDE_BY_FIVE_CLOCKING_CMD, 1);
    Status = FTC_SendBytesToDevice(ftHandle);
  }

  return Status;
}

FTC_STATUS FTC_TurnOffDivideByFiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((Status = FTC_IsHiSpeedDeviceHandleValid( ftHandle)) == FTC_SUCCESS)
  {
    FTC_SetDeviceDivideByFiveState(ftHandle, 0);

    FTC_AddByteToOutputBuffer(TURN_OFF_DIVIDE_BY_FIVE_CLOCKING_CMD, 1);
    Status = FTC_SendBytesToDevice(ftHandle);
  }

  return Status;
}

FTC_STATUS FTC_TurnOnAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_AddByteToOutputBuffer(TURN_ON_ADAPTIVE_CLOCKING_CMD, 1);

  return FTC_SendBytesToDevice(ftHandle);
}

FTC_STATUS FTC_TurnOffAdaptiveClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_AddByteToOutputBuffer(TURN_OFF_ADAPTIVE_CLOCKING_CMD, 1);

  return FTC_SendBytesToDevice(ftHandle);
}

FTC_STATUS FTC_TurnOnThreePhaseDataClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_AddByteToOutputBuffer(TURN_ON_THREE_PHASE_DATA_CLOCKING_CMD, 1);

  return FTC_SendBytesToDevice(ftHandle);
}

FTC_STATUS FTC_TurnOffThreePhaseDataClockingHiSpeedDevice(FTC_HANDLE ftHandle)
{
  FTC_AddByteToOutputBuffer(TURN_OFF_THREE_PHASE_DATA_CLOCKING_CMD, 1);

  return FTC_SendBytesToDevice(ftHandle);
}

FTC_STATUS FTC_SetDeviceLatencyTimer_h(FTC_HANDLE ftHandle, BYTE LatencyTimermSec)
{
  FTC_STATUS Status = FTC_SUCCESS;

  if ((LatencyTimermSec >= MIN_LATENCY_TIMER_VALUE) && (LatencyTimermSec <= MAX_LATENCY_TIMER_VALUE))
    Status = FTC_SetDeviceLatencyTimer(ftHandle, LatencyTimermSec);
  else
    Status = FTC_INVALID_TIMER_VALUE;

  return Status;
}

void FTC_GetHiSpeedDeviceClockFrequencyValues3(FTC_HANDLE ftHandle, DWORD dwClockFrequencyValue, LPDWORD lpdwClockFrequencyHz)
{
  if (FTC_GetDeviceDivideByFiveState(ftHandle) == 0)
    // the state of the clock divide by five is turned off(0)
    *lpdwClockFrequencyHz = (BASE_CLOCK_FREQUENCY_60_MHZ / ((1 + dwClockFrequencyValue) * 2));
  else
    // the state of the clock divide by five is turned on(1)
    *lpdwClockFrequencyHz = (BASE_CLOCK_FREQUENCY_12_MHZ / ((1 + dwClockFrequencyValue) * 2));
}

void FTC_GetHiSpeedDeviceClockFrequencyValues(DWORD dwClockFrequencyValue, LPDWORD lpdwClockFrequencyHz)
{
  *lpdwClockFrequencyHz = (BASE_CLOCK_FREQUENCY_60_MHZ / ((1 + dwClockFrequencyValue) * 2));
}

FTC_STATUS FTC_IsDeviceHiSpeedType2(FTC_HANDLE ftHandle, LPBOOL lpbHiSpeedDeviceType)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwDeviceType = 0;
  DWORD dwDeviceID = 0;
  SerialNumber szSerialNumber;
  char szDeviceNameBuffer[DEVICE_STRING_BUFF_SIZE + 1];
  PVOID pvDummy = NULL;

  *lpbHiSpeedDeviceType = 0;

  if ((Status = FT_GetDeviceInfo((FT_HANDLE)ftHandle, &dwDeviceType, &dwDeviceID, szSerialNumber, szDeviceNameBuffer, pvDummy)) == FTC_SUCCESS)
  {
    if ((dwDeviceType == FT_DEVICE_2232H) || (dwDeviceType == FT_DEVICE_4232H))
    {
      *lpbHiSpeedDeviceType = 1;
    }
  }
  return Status;
}

FTC_STATUS FTC_IsDeviceHiSpeedFT2232HType(FTC_HANDLE ftHandle, LPBOOL lpbHiSpeedFT2232HTDeviceype)
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwDeviceType = 0;
  DWORD dwDeviceID = 0;
  SerialNumber szSerialNumber;
  char szDeviceNameBuffer[DEVICE_STRING_BUFF_SIZE + 1];
  PVOID pvDummy = NULL;

  *lpbHiSpeedFT2232HTDeviceype = 0;

  if ((Status = FT_GetDeviceInfo((FT_HANDLE)ftHandle, &dwDeviceType, &dwDeviceID, szSerialNumber, szDeviceNameBuffer, pvDummy)) == FTC_SUCCESS)
  {
    if (dwDeviceType == FT_DEVICE_2232H)
    {
      *lpbHiSpeedFT2232HTDeviceype = 1;
    }
  }

  return Status;
}

BOOL FTC_IsDeviceHiSpeedType3(FTC_HANDLE ftHandle)
{
  BOOL bHiSpeedDeviceType = 0;
  DWORD dwDeviceType = 0;
  DWORD dwProcessId = 0;
  INT iDeviceCntr = 0;
  BOOL bDevicempHandleFound = 0;

  if (uiNumOpenedHiSpeedDevices > 0)
  {
    dwProcessId = getpid();

    for (iDeviceCntr = 0; ((iDeviceCntr < MAX_NUM_DEVICES) && !bDevicempHandleFound); iDeviceCntr++)
    {
      if (OpenedHiSpeedDevices[iDeviceCntr].dwProcessId == dwProcessId)
      {
        if (OpenedHiSpeedDevices[iDeviceCntr].hDevice == ftHandle)
        {
          bDevicempHandleFound = 1;

          dwDeviceType = OpenedHiSpeedDevices[iDeviceCntr].dwDeviceType;

          if ((dwDeviceType == FT_DEVICE_2232H) || (dwDeviceType == FT_DEVICE_4232H))
          {
            bHiSpeedDeviceType = 1;
          }
        }
      }
    }
  }
  return bHiSpeedDeviceType;
}
