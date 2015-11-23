#pragma once

#include "Common.h"

NTSTATUS LoopbackDeviceCreateIoQueue(_In_ WDFDEVICE device);

//
// Queue callbacks
//
EVT_WDF_IO_QUEUE_IO_READ LoopbackEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE LoopbackEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL LoopbackEvtIoDeviceControl;

#define LOOPBACK_IOCTL_ALTER_MAX_LENGTH 0x3245
