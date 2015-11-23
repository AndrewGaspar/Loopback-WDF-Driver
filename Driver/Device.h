#pragma once

#include "Common.h"

//
// Loopback device specific settings are saved in the device context
//
typedef struct _LOOPBACK_DEVICE_CONTEXT
{
    // Store the maximum supported length for read and write requests
    size_t MaxLengthInBytesForRWTransfers;

    // number of open files
    size_t OpenFiles;
} LOOPBACK_DEVICE_CONTEXT, *PLOOPBACK_DEVICE_CONTEXT;

//
// This macro will generate an inline function called GetDeviceContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(LOOPBACK_DEVICE_CONTEXT, GetDeviceContext)

EVT_WDF_DRIVER_DEVICE_ADD LoopbackEvtDeviceAdd;
