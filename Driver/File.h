#pragma once

#include "Common.h"

VOID LoopbackDeviceInitSetFileObjectCallbacks(_In_ PWDFDEVICE_INIT deviceInit);

EVT_WDF_DEVICE_FILE_CREATE LoopbackEvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE LoopbackEvtFileClose;
