#pragma once

#include <ntddk.h>
#include <wdf.h>
#include "trace.h"

//
// Unique device interface GUID
//
// 38553a6e-35a9-acee-bd18-3201f198a68b
DEFINE_GUID(GUID_DEVINTERFACE_LOOPBACK,
    0x38553a6e, 0x35a9, 0xacee, 0xbd, 0x18, 0x32, 0x01, 0xf1, 0x98, 0xa6, 0x8b);
