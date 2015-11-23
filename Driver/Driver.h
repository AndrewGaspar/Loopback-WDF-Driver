#pragma once

#include "Common.h"

NTSTATUS InitializeDriver(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath);

EVT_WDF_DRIVER_UNLOAD LoopbackEvtDriverUnload;
EVT_WDF_OBJECT_CONTEXT_CLEANUP LoopbackEvtDriverCleanup;

