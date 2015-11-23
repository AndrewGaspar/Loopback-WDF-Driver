#pragma once

#include "Common.h"

//
// Registration
//
VOID LoopbackDeviceInitSetPnpPowerCallbacks(_In_ PWDFDEVICE_INIT deviceInit);

//
// Prepare hardware for power the first time
//
EVT_WDF_DEVICE_PREPARE_HARDWARE LoopbackEvtDevicePrepareHardware;

//
// Called when device is being removed or resources are being rebalanced
//
EVT_WDF_DEVICE_RELEASE_HARDWARE LoopbackEvtDeviceReleaseHardware;

//
// Entry/exit from D0 state
//
EVT_WDF_DEVICE_D0_ENTRY LoopbackEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT LoopbackEvtDeviceD0Exit;
