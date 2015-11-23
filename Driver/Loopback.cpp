
#define INITGUID

#include "Common.h"

#include "Driver.h"

EXTERN_C
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT     driverObject,
    _In_ PUNICODE_STRING    registryPath)
{
    return InitializeDriver(driverObject, registryPath);
}